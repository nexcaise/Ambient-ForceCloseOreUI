#include <filesystem>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>

#include <cstdio>

#include <memscan.h>
#include <inlinehook.h>

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

std::string getConfigDir() {
  std::string primary = "/sdcard/games";
  std::string base = "/sdcard/Android/media/io.kitsuri.mayape.dev/modules";
  std::string base_real = "/sdcard/Android/media/io.kitsuri.mayape/modules";
  if (!base.empty()) {
    base += "/ForceCloseOreUI/";
    if (testDirWritable(base))
      return base;
  }
  if (!base_real.empty()) {
    base_real += "/ForceCloseOreUI/";
    if (testDirWritable(base_real))
      return base_real;
  }
  if (!primary.empty()) {
    primary += "/ForceCloseOreUI/";
    if (testDirWritable(primary))
      return primary;
  }
  return primary;
}

static bool founded = false;

void (*original_h2)(
    void*, void*, void*, void*, void*,
    void*, void*, void*, void*, OreUi&, void*
);

void hooked_h2(
    void* a1, void* a2, void* a3, void* a4, void* a5,
    void* a6, void* a7, void* a8, void* a9,
    OreUi& a10, void* a11
) {

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

  original_h2(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

hook_handle* g_hook = nullptr;

void hookOreUI() {
    sigscan_handle* scanner = nullptr;

    const char* patterns[] = {
      "? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 FD 03 00 91 ? ? ? D1 ? ? ? D5 FA 03 00 AA F5 03 07 AA",
      "? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 FD 03 00 91 ? ? ? D1 ? ? ? D5 FB 03 00 AA F5 03 07 AA",
      "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 ? ? ? F9 ? ? ? D5 FB 03 00 AA ? ? ? F9 F5 03 07 AA",
      "? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 FD 03 00 91 ? ? ? D1 ? ? ? D5 FA 03 00 AA F6 03 07 AA"
    };

    for (const char* pattern : patterns) {
      scanner = sigscan_setup(pattern, "libminecraftpe.so", GPWN_SIGSCAN_XMEM);
      if (founded) continue;
      if (!scanner) continue;
      founded = true;
    }

    if (!scanner) return;

    void* func = get_sigscan_result(scanner);
    sigscan_cleanup(scanner);

    if (func == (void*)-1) return;

    g_hook = hook_addr(
      func,
      (void*)hooked_h2,
      (void**)&original_h2,
      GPWN_AARCH64_MICROHOOK
    );
}

__attribute__((constructor))
void StartUp() {
    hookOreUI();
}

__attribute__((destructor))
void Shutdown() {
    if (g_hook) {
      rm_hook(g_hook);
    }
}
