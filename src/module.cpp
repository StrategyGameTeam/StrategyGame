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


std::vector<ModuleIssues> ModuleLoader::LoadModules(const std::vector<std::filesystem::path>& module_paths) {
    std::vector<ModuleIssues> issues;
    
    std::cout << " === MODULE LOADING START === \n";
    for(const auto& modpath : module_paths) {
        std::cout << "Starting to load " << modpath.string() << '\n';
        // find the entry point
        std::filesystem::path entry_point;
        std::string name;
        if (std::filesystem::is_directory(modpath)) {
            auto entrypath = modpath;
            entrypath.append("mod.lua");
            if (std::filesystem::exists(entrypath) && std::filesystem::is_regular_file(entrypath)) {
                entry_point = entrypath;
                name = modpath.stem().string();
            }
        } else {
            issues.push_back(InvalidPath{.path = modpath.string()});
            continue;
        }
        
        // create and save the module to the registry
        sol::state lua;
        auto [insert_it, insert_ok] = m_loaded_modules.emplace(lua.lua_state(), name);
        if (!insert_ok) {
            std::cout << "Could not contruct module - this should not happen, and should be reported to the developers\n";
            continue;
        }

        // load everything
        lua.open_libraries(sol::lib::base, sol::lib::jit, sol::lib::string, sol::lib::package);
        InjectSymbols(insert_it->second, lua);
        if (entry_point != modpath) {
            std::string package_path = lua["package"]["path"];
            lua["package"]["path"] = package_path + (!package_path.empty() ? ";" : "") + modpath.string() + "/?.lua";
        }
        auto res = lua.script_file(entry_point.string());
        if (!res.valid()) {
            std::cout << "Module " << name << " had an error when running - unregistering\n";
            m_loaded_modules.erase(lua.lua_state());
        }

        insert_it->second.state = std::move(lua); // fucky ownership
    }
    std::cout << " === MODULE LOADING END, DECLARATION APPLICATION START === \n";
    

    return issues;
}