#pragma once
#include <sol/sol.hpp>
#include <filesystem>
#include <iostream>
#include <concepts>
#include <unordered_map>

struct Module {
    std::string name;
    sol::state state;

    Module(std::string name) : name(name) {}
};

struct ModuleLoader {
    // some notes on that - the sol object sol::state belongs to the Module, but the pointer that is actually
    // at the heart od sol::state does not (that's the lua_State*). So, when destroying this variable,
    // the lua_State*s are destroyed by the destructor of the Module
    std::unordered_map<lua_State*, Module> m_loaded_modules;
    std::function<void(sol::string_view)> m_logger_fn;

    void DefaultLogger(sol::this_state ts, sol::string_view sv) {
        const auto name = ([&]{
            if (auto res = m_loaded_modules.find(ts.lua_state()); res != m_loaded_modules.end()) {
                return res->second.name;
            } else {
                return std::string("???");
            } 
        })();
        std::cout << "LUA [" << name << "]: " << sv << '\n';
    }

    auto ListCandidateModules () {
        auto cwd = std::filesystem::current_path();
        cwd.append("resources/modules");
        auto iter = std::filesystem::directory_iterator(cwd);
        std::vector<std::filesystem::path> paths;
        for(const auto &file : iter) {
            if (file.is_directory() || (file.is_regular_file() && file.path().extension() == ".lua")) {
                paths.push_back(file);
            }
        }
        return paths;
    }

    auto LoadModules(const std::vector<std::filesystem::path>& module_paths) {
        std::cout << " === MODULE LOADING START === \n";
        for(const auto& modpath : module_paths) {
            std::cout << "Starting to load " << modpath.string() << '\n';
            // find the entry point
            const auto [entry_point, name] = ([&] () -> std::pair<std::optional<std::filesystem::path>, std::string> {
                if (std::filesystem::is_regular_file(modpath) && modpath.extension() == ".lua") {
                    return {modpath, modpath.stem().string()};
                } else if (std::filesystem::is_directory(modpath)) {
                    auto entrypath = modpath;
                    entrypath.append("mod.lua");
                    if (std::filesystem::exists(entrypath) && std::filesystem::is_regular_file(entrypath)) {
                        return {entrypath, modpath.stem().string()};
                    } else {
                        return {{}, modpath.stem().string()};
                    }
                } else {
                    return {{}, modpath.stem().string()};
                };
            })();

            if (!entry_point.has_value()) {
                std::cout << "Module " << modpath.filename().string() << " was skipped, as we cannot load it\n";
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
            InjectSymbols(lua);
            if (entry_point != modpath) {
                std::string package_path = lua["package"]["path"];
                lua["package"]["path"] = package_path + (!package_path.empty() ? ";" : "") + modpath.string() + "/?.lua";
            }
            auto res = lua.script_file(entry_point.value().string());
            if (!res.valid()) {
                std::cout << "Module " << name << " had an error when running - unregistering\n";
                m_loaded_modules.erase(lua.lua_state());
            }

            insert_it->second.state = std::move(lua); // fucky ownership
        }
        std::cout << " === MODULE LOADING END === \n";
    }

    void InjectSymbols(sol::state& lua) {
        lua.set_function("log", &ModuleLoader::DefaultLogger, this);
    }
};

