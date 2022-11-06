#pragma once
#include <sol/sol.hpp>
#include <filesystem>
#include <iostream>
#include <concepts>
#include <unordered_map>
#include <variant>
#include <string>

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

struct ModuleLoader {
    // some notes on that - the sol object sol::state belongs to the Module, but the pointer that is actually
    // at the heart od sol::state does not (that's the lua_State*). So, when destroying this variable,
    // the lua_State*s are destroyed by the destructor of the Module
    std::unordered_map<lua_State*, Module> m_loaded_modules;
    std::function<void(sol::string_view)> m_logger_fn;

    void DefaultLogger(sol::this_state ts, sol::string_view sv);
    std::vector<std::filesystem::path> ListCandidateModules (std::filesystem::path load_path);
    std::vector<ModuleIssues> LoadModules(const std::vector<std::filesystem::path>& module_paths);
    void InjectSymbols(Module &mod, sol::state& lua);
};

