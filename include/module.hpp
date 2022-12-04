#pragma once
#include <sol/sol.hpp>
#include <filesystem>
#include <iostream>
#include <concepts>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <string>
#include "utils.hpp"

struct Module;

// ===== Issues with modules =====
namespace issues {
    // We cannot read the given path as valid lua
    struct InvalidPath { std::string path; };
    // Something happened, no clue what
    struct UnknownError { std::string message; };
    // Lua done goofed
    struct LuaError { std::string message; };
    // Running a module did not return a table
    struct NotATable { std::string mod; };
    // A module that is needed is not available
    struct RequiredModuleNotFound { std::string requiree; std::string required; };
    // A module did not declare its name
    struct UnnamedModule { std::string path; };
    // Something extra was present in the configuration
    struct ExtraneousElement { std::string mod; std::string in; };

    using ModuleIssues = std::variant<InvalidPath, UnknownError, LuaError, NotATable, RequiredModuleNotFound, UnnamedModule, ExtraneousElement>;
}

struct Module {
    std::string entry_point;
    sol::state state;
    sol::table module_root_object;

    Module(std::string entry) : entry_point(entry) {}

    void ModuleInfo(sol::table t);
    sol::optional<std::string_view> name() const;
    std::string name_unsafe () const;
};

template <typename T>
concept ModuleExtention = requires(T ext, sol::state ss) {
    { ext.InjectSymbols(ss) };
};

struct ModuleLoader {
    // some notes on that - the sol object sol::state belongs to the Module, but the pointer that is actually
    // at the heart od sol::state does not (that's the lua_State*). So, when destroying this variable,
    // the lua_State*s are destroyed by the destructor of the Module.
    // Also, iterators to unordered_map can get invalidated, but not references to things behind that
    // (in most implementations, it's a linked list. but that's a standard guaranteed prop)
    std::unordered_map<lua_State*, Module> m_loaded_modules;

    // module loader should not me mobile in memory - make one, and reference to it
    ModuleLoader() = default;
    ~ModuleLoader();
    ModuleLoader(const ModuleLoader&) = delete;
    ModuleLoader(ModuleLoader&&) = delete;
    ModuleLoader& operator= (const ModuleLoader&) = delete;
    ModuleLoader& operator= (ModuleLoader&&) = delete;

    // This function is used to give lua a way to log stuff
    void DefaultLogger(sol::this_state ts, sol::string_view sv);

    // Goes through a directory to find modules that one could attempt to load
    std::vector<std::filesystem::path> ListCandidateModules (std::filesystem::path load_path);
    
    // Gives lua needed symbols (which is a bad name - think functions and variables)
    void InjectSymbols(sol::state& lua);
    // Tests if simple declared needs are met
    void BasicValidation(std::vector<issues::ModuleIssues> &issues);

    // Used to call the method InjecSymbols on a bunch of objects of different types
    static void LoadLuaTRec(sol::state &lua) {(void)lua;}
    template <ModuleExtention T, ModuleExtention ...TS>
    static void LoadLuaTRec(sol::state &lua, T& m, TS&... ms) {
        m.InjectSymbols(lua);
        LoadLuaTRec(lua, ms...);
    }

    // Start running lua modules
    template <ModuleExtention ...ExtentionTS>
    std::vector<issues::ModuleIssues> LoadModules(const std::vector<std::filesystem::path>& module_paths, ExtentionTS&... extentions) {
        std::vector<issues::ModuleIssues> issues;

        // Load and run Lua
        for(const auto& modpath : module_paths) {
            try {
                logger::info("Starting to load ", modpath.string());
                const auto entry_point = modpath / "mod.lua";
                if (!std::filesystem::is_regular_file(entry_point)) {
                    issues.push_back(issues::InvalidPath{.path = entry_point.string() });
                    continue;
                }

                sol::state lua;
            
                // load parts of standard lua library
                lua.open_libraries(sol::lib::base, sol::lib::jit, sol::lib::string, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::os);
                // load custom stuff
                LoadLuaTRec(lua, *this, extentions...);
                
                // allow lua to import from the module directory
                std::string package_path = lua["package"]["path"];
                lua["package"]["path"] = package_path + (!package_path.empty() ? ";" : "") + std::filesystem::absolute(modpath).string() + "/?.lua";

                // run the module code
                auto res = lua.safe_script_file(entry_point.string());
                if (!res.valid()) {
                    sol::error err = res;
                    issues.push_back(issues::LuaError{.message = err.what() });
                    continue;
                }
                if (res.get_type() != sol::type::table) {
                    issues.push_back(issues::NotATable{.mod = entry_point.string() });
                    continue;
                }

                // create and save the module to the registry
                auto [insert_it, insert_ok] = m_loaded_modules.emplace(lua.lua_state(), entry_point.string());
                if (!insert_ok) {
                    issues.push_back(issues::UnknownError{.message = "Could not contruct module - this should not happen, and should be reported to the developers" });
                    continue;
                }

                insert_it->second.module_root_object = res.get<sol::table>();
                insert_it->second.state = std::move(lua); // fucky ownership
            } catch (std::exception& exc) {
                issues.push_back(issues::UnknownError{.message = exc.what() });
            }
        }
        
        // Do basic validation and processing
        BasicValidation(issues);        

        return issues;
    }    
};

