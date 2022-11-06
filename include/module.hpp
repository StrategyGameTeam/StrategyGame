#pragma once
#include <sol/sol.hpp>
#include <filesystem>
#include <iostream>
#include <concepts>
#include <unordered_map>
#include <variant>
#include <string>

struct Module;

// ===== Content declaration kinds =====
#pragma region Declarations
struct BaseDeclaration {
    const Module& mod;
};

struct HexDeclaration : public BaseDeclaration {
    std::string name;
    std::filesystem::path model;
};

struct WorldgenDeclaration : public BaseDeclaration {
    std::string name;
    sol::protected_function generator;
};

using Declaration = std::variant<HexDeclaration, WorldgenDeclaration>;
#pragma endregion Declarations

// ===== Issues with modules =====
#pragma region Issues
struct BaseIssue {
    bool is_critical;
};

struct InvalidPathIssue : public BaseIssue { std::string path; };

using ModuleIssues = std::variant<InvalidPathIssue>;
#pragma endregion Issues

struct Module {
    std::string name;
    sol::state state;
    std::vector<Declaration> declarations;
    std::optional<std::reference_wrapper<std::vector<ModuleIssues>>> current_issues;

    Module(std::string name) : name(name) {}

    void AddIssue(auto issue);

    void ModuleInfo(sol::table t);
    void DeclareHex(sol::table t);
};


template <typename T>
concept ModuleExtention = requires(T ext, sol::state ss) {
    { ext.InjectSymbols(ss) };
};

struct ModuleLoader {
    // some notes on that - the sol object sol::state belongs to the Module, but the pointer that is actually
    // at the heart od sol::state does not (that's the lua_State*). So, when destroying this variable,
    // the lua_State*s are destroyed by the destructor of the Module
    std::unordered_map<lua_State*, Module> m_loaded_modules;
    std::function<void(sol::string_view)> m_logger_fn;

    void LoadLuaTRec(sol::state &lua) {}
    template <ModuleExtention T, ModuleExtention ...TS>
    void LoadLuaTRec(sol::state &lua, T& m, TS&... ms) {
        m.InjectSymbols(lua);
        LoadLuaTRec(lua, ms...);
    }

    template <ModuleExtention ...ExtentionTS>
    auto LoadModules(const std::vector<std::filesystem::path>& module_paths, ExtentionTS&... extentions) {
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
                    }
                }
                return {{}, modpath.stem().string()};
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
            lua.open_libraries(sol::lib::base, sol::lib::jit, sol::lib::string, sol::lib::package, sol::lib::math);

            LoadLuaTRec(lua, extentions...);
            InjectSymbols(insert_it->second, lua);

            if (entry_point != modpath) {
                std::string package_path = lua["package"]["path"];
                lua["package"]["path"] = package_path + (!package_path.empty() ? ";" : "") + modpath.string() + "/?.lua";
            }
            try {
                auto res = lua.script_file(entry_point.value().string());
                if (!res.valid()) {
                    std::cout << "Module " << name << " had an error when running - unregistering\n";
                    m_loaded_modules.erase(lua.lua_state());
                    continue;
                }
            } catch (std::exception& e) {
                std::cerr << e.what() << '\n';
                m_loaded_modules.erase(lua.lua_state());
                continue;
            }

            insert_it->second.state = std::move(lua); // fucky ownership
        }
        std::cout << " === MODULE LOADING END === \n";
    }

    void DefaultLogger(sol::this_state ts, sol::string_view sv);
    std::vector<std::filesystem::path> ListCandidateModules (std::filesystem::path load_path);
    void InjectSymbols(Module &mod, sol::state& lua);
};

