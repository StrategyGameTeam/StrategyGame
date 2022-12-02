#include "module.hpp"
#include "utils.hpp"

ModuleLoader::~ModuleLoader() {
    log::debug("MODULE LOADER DESCTUCTOR");
}

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

sol::optional<sol::string_view> Module::name() const {
    return module_root_object["name"];
}

std::string Module::name_unsafe() const {
    return std::string(this->name().value_or("???"));
}

std::optional<std::reference_wrapper<Module>> ModuleLoader::GetModule(lua_State *ptr) {
    auto v = m_loaded_modules.find(ptr);
    if (v != m_loaded_modules.end()) {
        return v->second;
    } else {
        return {};
    }
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

void ModuleLoader::BasicValidation(std::vector<issues::AnyIssue> &issues) {
    // get a hashset of names of modules
    std::unordered_set<std::string> module_names;
    for(const auto& [_, mod] : m_loaded_modules) { 
        const auto name = mod.name();
        if (!name.has_value()) {
            issues.push_back(issues::UnnamedModule{.path = mod.entry_point});
            continue;
        }
        module_names.emplace(name.value());
    }

    // test required modules
    for(const auto& [_, mod] : m_loaded_modules) {
        sol::optional<sol::table> required_modules = mod.module_root_object["required_modules"];
        if (!required_modules.has_value()) continue; // safe, module does not require stuff
        for (auto& [key, value] : required_modules.value()) {
            if (!value.is<sol::string_view>()) {
                std::string location = "root -> required_modules -> ";
                location += key.as<sol::string_view>();
                const auto name = mod.name().map_or([](sol::string_view sv){ return std::string(sv); }, mod.entry_point);
                issues.push_back(issues::ExtraneousElement{.mod = name, .in = location });
                continue;
            }
            const auto required_name = value.as<sol::string_view>();
            if (!module_names.contains(std::string(required_name))) {
                const auto name = mod.name().map_or([](sol::string_view sv){ return std::string(sv); }, mod.entry_point);
                issues.push_back(issues::RequiredModuleNotFound{.requiree = name, .required = std::string(required_name)});
                continue;
            }
        }
    }
}