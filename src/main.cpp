#include <unordered_map>
#include <string>
#include <functional>

#include <nise/stub.h>
#include <memscan.h>
#include <inlinehook.h>

#include <miniAPI.h>

class OreUIConfig {
public:
    void* mUnknown1;
    void* mUnknown2;
    std::function<bool()> mUnknown3;
    std::function<bool()> mUnknown4;
};

class OreUi {
public:
    std::unordered_map<std::string, OreUIConfig> mConfigs;
};

void (*original_h2)(
    void*, void*, void*, void*, void*,
    void*, void*, void*, void*, OreUi&, void*
);

nlohmann::json outputJson;
std::string filePath = "/sdcard/Android/media/io.kitsuri.mayape/modules_config/ForceCloseOreUI/config.json";
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

void hooked_h2(
    void* a1, void* a2, void* a3, void* a4, void* a5,
    void* a6, void* a7, void* a8, void* a9,
    OreUi& a10, void* a11
) {

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
      if (!scanner) continue;
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
