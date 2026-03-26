#include <unordered_map>
#include <string>
#include <functional>

#include <nise/stub.h>
#include <memscan.h>
#include <inlinehook.h>

#include "miniAPI.h"

using miniAPI::Config;

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

static Config cfg("ForceCloseOreUI");

void (*original_h2)(
    void*, void*, void*, void*, void*,
    void*, void*, void*, void*, OreUi&, void*
);

void hooked_h2(
    void* a1, void* a2, void* a3, void* a4, void* a5,
    void* a6, void* a7, void* a8, void* a9,
    OreUi& ui, void* a11
) {
    cfg.load("config.json");

    for (auto& [key, value] : ui.mConfigs) {
        bool state;

        if (cfg.has(key)) {
            state = cfg.get<bool>(key, false);
        } else {
            state = false;
            cfg.set(key, false);
        }

        value.mUnknown3 = [state]() { return state; };
        value.mUnknown4 = [state]() { return state; };
    }

    original_h2(a1, a2, a3, a4, a5, a6, a7, a8, a9, ui, a11);
}

hook_handle* g_hook = nullptr;

void hookOreUI() {
    sigscan_handle* scanner = nullptr;

    const char* patterns[] = {
        "?? ?? ?? A9 ?? ?? ?? A9 ?? ?? ?? A9 ?? ?? ?? A9 "
        "?? ?? ?? A9 ?? ?? ?? A9 FD 03 00 91 ?? ?? ?? D1 "
        "?? ?? ?? D5 FB 03 00 AA F5 03 07 AA",

        "?? ?? ?? D1 ?? ?? ?? A9 ?? ?? ?? A9 ?? ?? ?? A9 "
        "?? ?? ?? A9 ?? ?? ?? A9 ?? ?? ?? A9 ?? ?? ?? 91 "
        "?? ?? ?? F9 ?? ?? ?? D5 FB 03 00 AA ?? ?? ?? F9 "
        "F5 03 07 AA",

        "FD 7B BA A9 FC 6F 01 A9 FA 67 02 A9 F8 5F 03 A9 "
        "F6 57 04 A9 F4 4F 05 A9 FD 03 00 91 FF C3 18 D1 "
        "59 D0 3B D5 FA 03 00 AA F5 03 07 AA"
    };

    for (const char* pattern : patterns) {
        scanner = sigscan_setup(pattern, "libminecraftpe.so", GPWN_SIGSCAN_XMEM);
        if (scanner) break;
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
