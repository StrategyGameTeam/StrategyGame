#include "module.hpp"

void ModuleLoader::InjectSymbols(Module &mod, sol::state& lua) {
    using sol::as_function;

    lua.set_function("log", &ModuleLoader::DefaultLogger, this);
    lua.create_named_table("MOD",
        "ModuleInfo", as_function(&Module::ModuleInfo, &mod),
        "DeclareHex", as_function(&Module::DeclareHex, &mod)
    );
}

// ===== Lua API =====

void ModuleLoader::DefaultLogger(sol::this_state ts, sol::string_view sv) {
    const auto res = m_loaded_modules.find(ts.lua_state());
    const auto name = res != m_loaded_modules.end() ? res->second.name : "???";
    std::cout << "LUA [" << name << "]: " << sv << '\n';
}

void Module::ModuleInfo (sol::table t) {
    const sol::optional<std::string_view> name = t["name"];
    if (name) {
        std::cout << "Module called " << name.value() << " called ModuleInfo!\n";
    } else {
        std::cout << "Module called ModuleInfo with no name!\n";
    }
}

void Module::DeclareHex (sol::table t) {
    sol::optional<sol::string_view> name = t["name"];
    sol::optional<sol::string_view> model = t["model"];

    std::cout << "DECLHEX: " << name.value_or("???") << " " << model.value_or("???") << '\n';
}

// ===== Loading stuff =====

std::vector<std::filesystem::path> ModuleLoader::ListCandidateModules(std::filesystem::path load_path) {
    auto iter = std::filesystem::directory_iterator(std::filesystem::path(load_path));
    std::vector<std::filesystem::path> paths;
    for(const auto &file : iter) {
        if (file.is_directory()) {
            paths.push_back(file);
        }
    }
    return paths;
}
