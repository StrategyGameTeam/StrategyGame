#include "module.hpp"

void ModuleLoader::InjectSymbols(sol::state& lua) {
    using sol::as_function;
    lua.set_function("log", &ModuleLoader::DefaultLogger, this);
    lua.set_function("pow", sol::overload(powf, powl));
}

// ===== Lua API =====

void ModuleLoader::DefaultLogger(sol::this_state ts, sol::string_view sv) {
    const auto res = m_loaded_modules.find(ts.lua_state());
    const auto name = res != m_loaded_modules.end() ? res->second.name().value_or(res->second.entry_point) : "???";
    std::cout << "LUA [" << name << "]: " << sv << '\n';
}

// ===== Module functions =====

std::optional<std::string_view> Module::name() const {
    return module_root_object["name"];
}

// ===== Loading stuff =====

std::vector<std::filesystem::path> ModuleLoader::ListCandidateModules(std::filesystem::path load_path) {
    auto iter = std::filesystem::directory_iterator(load_path);
    std::vector<std::filesystem::path> paths;
    for(const auto &file : iter) {
        if (!file.is_directory()) { continue; } 
        if (!std::filesystem::is_regular_file(file.path() / "mod.lua")) { continue; }
        paths.push_back(file);
    }
    return paths;
}
