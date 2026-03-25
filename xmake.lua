set_project("ForceCloseOreUI")
set_version("1.0.0")

set_languages("cxx23")

add_rules("mode.release")

add_repositories(
    "xmake-repo https://github.com/xmake-io/xmake-repo.git",
    "liteldev-repo https://github.com/LiteLDev/xmake-repo.git"
)

add_requires("nlohmann_json v3.11.3")

target("ForceCloseOreUI")
    set_kind("shared")
    add_packages("nlohmann_json")
    add_linkdirs("niseAPI/libs/arm64-v8a")
    add_linkdirs("miniAPI/libs/arm64-v8a")
    add_links("nise", "log", "miniAPI", "GlossHook")

    add_files("src/*.cpp", "niseAPI/deps/gamepwnage/src/*.c", "niseAPI/deps/gamepwnage/src/**/*.c")

    add_includedirs("miniAPI/include", "niseAPI/include", "niseAPI/deps/gamepwnage/includes", {public = true})

    add_cxflags("-O2", "-fvisibility=hidden", "-ffunction-sections", "-fdata-sections", "-w")
    add_cflags("-O2", "-fvisibility=hidden", "-ffunction-sections", "-fdata-sections", "-w")
    add_ldflags("-Wl,--gc-sections,--strip-all", "-s")
