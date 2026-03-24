#include <filesystem>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>

#include "api/memory/Hook.h"
#include <cstdio>

namespace fs = std::filesystem;

#include "jni.h"
#include <android/log.h>

JNIEnv *env = nullptr;

#define LOGI(...)                                                              \
  __android_log_print(ANDROID_LOG_INFO, "LeviLogger", __VA_ARGS__)

jobject getGlobalContext(JNIEnv *env) {
  jclass activity_thread = env->FindClass("android/app/ActivityThread");
  jmethodID current_activity_thread =
      env->GetStaticMethodID(activity_thread, "currentActivityThread",
                             "()Landroid/app/ActivityThread;");
  jobject at =
      env->CallStaticObjectMethod(activity_thread, current_activity_thread);
  jmethodID get_application = env->GetMethodID(
      activity_thread, "getApplication", "()Landroid/app/Application;");
  jobject context = env->CallObjectMethod(at, get_application);
  if (env->ExceptionCheck())
    env->ExceptionClear();
  return context;
}

std::string getAbsolutePath(JNIEnv *env, jobject file) {
  jclass file_class = env->GetObjectClass(file);
  jmethodID get_abs_path =
      env->GetMethodID(file_class, "getAbsolutePath", "()Ljava/lang/String;");
  auto jstr = (jstring)env->CallObjectMethod(file, get_abs_path);
  if (env->ExceptionCheck())
    env->ExceptionClear();
  const char *cstr = env->GetStringUTFChars(jstr, nullptr);
  std::string result(cstr);
  env->ReleaseStringUTFChars(jstr, cstr);
  return result;
}

std::string getPackageName(JNIEnv *env, jobject context) {
  jclass context_class = env->GetObjectClass(context);
  jmethodID get_pkg_name =
      env->GetMethodID(context_class, "getPackageName", "()Ljava/lang/String;");
  auto jstr = (jstring)env->CallObjectMethod(context, get_pkg_name);
  if (env->ExceptionCheck())
    env->ExceptionClear();
  const char *cstr = env->GetStringUTFChars(jstr, nullptr);
  std::string result(cstr);
  env->ReleaseStringUTFChars(jstr, cstr);
  return result;
}

std::string getInternalStoragePath(JNIEnv *env) {
  jclass env_class = env->FindClass("android/os/Environment");
  jmethodID get_storage_dir = env->GetStaticMethodID(
      env_class, "getExternalStorageDirectory", "()Ljava/io/File;");
  jobject storage_dir = env->CallStaticObjectMethod(env_class, get_storage_dir);
  return getAbsolutePath(env, storage_dir);
}

std::string GetModsFilesPath(JNIEnv *env) {
  jobject app_context = getGlobalContext(env);
  if (!app_context) {
    return "";
  }
  auto package_name = getPackageName(env, app_context);
  for (auto &c : package_name)
    c = tolower(c);

  return (fs::path(getInternalStoragePath(env)) / "Android" / "media" /
          package_name / "modules_config");
}

SKY_AUTO_STATIC_HOOK(
    Hook1, memory::HookPriority::Normal,
    std::initializer_list<const char *>(
        {"? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? "
         "91 ? ? ? D5 ? ? ? F9 ? ? ? F8 ? ? ? 39 ? ? ? 34 ? ? ? 12"}),
    int, void *_this, JavaVM *vm) {

  vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4);
  return origin(_this, vm);
}

class OreUIConfig {
public:
  void *mUnknown1;
  void *mUnknown2;
  std::function<bool()> mUnknown3;
  std::function<bool()> mUnknown4;
};

class OreUi {
public:
  std::unordered_map<std::string, OreUIConfig> mConfigs;
};

#define OREUI_PATTERN                                                                     \
     std::initializer_list<const char *>({                                                \
      "? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 FD 03 00 91 ? ? ? D1 ? ? ? D5 FB 03 00 AA F5 03 07 AA", \
      "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 ? ? ? F9 ? ? ? D5 FB 03 00 AA ? ? ? F9 F5 03 07 AA", \
      "? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 FD 03 00 91 ? ? ? D1 ? ? ? D5 FA 03 00 AA F6 03 07 AA" \
  })                                                                                                                    \

namespace {

bool testDirWritable(const std::string &dir) {
  std::error_code _;
  std::filesystem::create_directories(dir, _);
  std::string testFile = dir + "._perm_test";
  std::ofstream ofs(testFile);
  bool ok = ofs.is_open();
  ofs.close();
  if (ok)
    std::filesystem::remove(testFile, _);
  return ok;
}

std::string getConfigDir() {
  std::string primary = "/sdcard/games/modules_config";
  if (!env)
    return primary;
  std::string base = GetModsFilesPath(env);
  if (!base.empty()) {
    base += "/ForceCloseOreUI/";
    if (testDirWritable(base))
      return base;
  }
  if (!primary.empty()) {
    primary += "/ForceCloseOreUI/";
    if (testDirWritable(primary))
      return primary;
  }
  return primary;
}
nlohmann::json outputJson;
std::string dirPath = "";
std::string filePath = dirPath + "config.json";
bool updated = false;

void saveJson(const std::string &path, const nlohmann::json &j) {
  std::filesystem::create_directories(
      std::filesystem::path(path).parent_path());
  FILE *f = std::fopen(path.c_str(), "w");
  if (!f) {
    throw std::runtime_error(path);
  }
  std::string jsonStr = j.dump(4);
  std::fwrite(jsonStr.data(), 1, jsonStr.size(), f);
  std::fclose(f);
}

SKY_AUTO_STATIC_HOOK(Hook2, memory::HookPriority::Normal, OREUI_PATTERN, void,
                     void *a1, void *a2, void *a3, void *a4, void *a5, void *a6,
                     void *a7, void *a8, void *a9, OreUi &a10, void *a11) {
  dirPath = getConfigDir();
  filePath = dirPath + "config.json";

  if (std::filesystem::exists(filePath)) {
    std::ifstream inFile(filePath);
    inFile >> outputJson;
    inFile.close();
  }

  for (auto &data : a10.mConfigs) {

    bool value = false;
    if (outputJson.contains(data.first) &&
        outputJson[data.first].is_boolean()) {
      value = outputJson[data.first];
    } else {

      outputJson[data.first] = false;
      updated = true;
    }
    data.second.mUnknown3 = [value]() { return value; };
    data.second.mUnknown4 = [value]() { return value; };
  }

  if (updated || !std::filesystem::exists(filePath)) {
    saveJson(filePath, outputJson);
  }

  origin(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

} // namespace
