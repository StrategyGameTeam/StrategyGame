#pragma once
#include <sol/sol.hpp>
#include <filesystem>
#include <iostream>
#include <concepts>
#include <unordered_map>
#include <variant>
#include <string>

struct Module;

// ===== Issues with modules =====
#pragma region Issues
// We cannot read the given path as valid lua
struct InvalidPathIssue { std::string path; };
// Something happened, no clue what
struct UnknownErrorIssue { std::string message; };
// Lua done goofed
struct LuaErrorIssue { std::string message; };
// Running a module did not return a table
struct NotATableIssue { std::string mod; };
using ModuleIssues = std::variant<InvalidPathIssue, UnknownErrorIssue, LuaErrorIssue, NotATableIssue>;
#pragma endregion Issues

struct Module {
    std::string entry_point;
    sol::state state;
    sol::table module_root_object;

    Module(std::string entry) : entry_point(entry) {}

    void ModuleInfo(sol::table t);
    std::optional<std::string_view> name() const;
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
    ModuleLoader(const ModuleLoader&) = delete;
    ModuleLoader(ModuleLoader&&) = delete;
    ModuleLoader& operator= (const ModuleLoader&) = delete;
    ModuleLoader& operator= (ModuleLoader&&) = delete;

    // This function is used to give lua a way to log stuff
    void DefaultLogger(sol::this_state ts, sol::string_view sv);

    // Goes through a directory to find modules that one could attempt to load
    std::vector<std::filesystem::path> ListCandidateModules (std::filesystem::path load_path);
    
    // Gets an forward_iterator through modules
    auto iter_start() { return m_loaded_modules.begin(); }
    auto iter_end() { return m_loaded_modules.end(); }

    // Gives lua needed symbols (which is a bad name - think functions and variables)
    void InjectSymbols(sol::state& lua);

    // Used to call the method InjecSymbols on a bunch of objects of different types
    void LoadLuaTRec(sol::state &lua) {(void)lua;}
    template <ModuleExtention T, ModuleExtention ...TS>
    void LoadLuaTRec(sol::state &lua, T& m, TS&... ms) {
        m.InjectSymbols(lua);
        LoadLuaTRec(lua, ms...);
    }

    // Start running lua modules
    template <ModuleExtention ...ExtentionTS>
    std::vector<ModuleIssues> LoadModules(const std::vector<std::filesystem::path>& module_paths, ExtentionTS&... extentions) {
        std::cout << " === MODULE LOADING START === \n";
        std::vector<ModuleIssues> issues;

        for(const auto& modpath : module_paths) {
            try {
                std::cout << "Starting to load " << modpath.string() << '\n';
                const auto entry_point = modpath / "mod.lua";
                if (!std::filesystem::is_regular_file(entry_point)) {
                    issues.push_back(InvalidPathIssue{.path = entry_point.string() });
                    continue;
                }

                sol::state lua;
            
                // load parts of standard lua library
                lua.open_libraries(sol::lib::base, sol::lib::jit, sol::lib::string, sol::lib::package, sol::lib::math, sol::lib::table);
                // load custom stuff
                LoadLuaTRec(lua, *this, extentions...);
                
                // allow lua to import from the module directory
                std::string package_path = lua["package"]["path"];
                lua["package"]["path"] = package_path + (!package_path.empty() ? ";" : "") + std::filesystem::absolute(modpath).string() + "/?.lua";

                // run the module code
                auto res = lua.safe_script_file(entry_point.string());
                if (!res.valid()) {
                    sol::error err = res;
                    issues.push_back(LuaErrorIssue{.message = err.what() });
                    continue;
                }
                if (res.get_type() != sol::type::table) {
                    issues.push_back(NotATableIssue{.mod = entry_point.string() });
                    continue;
                }

                // create and save the module to the registry
                auto [insert_it, insert_ok] = m_loaded_modules.emplace(lua.lua_state(), entry_point.string());
                if (!insert_ok) {
                    issues.push_back(UnknownErrorIssue{.message = "Could not contruct module - this should not happen, and should be reported to the developers" });
                    continue;
                }

                insert_it->second.module_root_object = res.get<sol::table>();
                insert_it->second.state = std::move(lua); // fucky ownership
            } catch (std::exception& exc) {
                issues.push_back(UnknownErrorIssue{.message = exc.what() });
            }
        }
        std::cout << " === MODULE LOADING END === \n";
        return issues;
    }    
};

