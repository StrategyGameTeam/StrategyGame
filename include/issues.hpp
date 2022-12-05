#pragma once
#include <string>
#include <variant>
#include <sol/sol.hpp>

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

    struct MissingField { std::string what_module; std::string what_def; std::string fieldname; };
    struct InvalidFile { std::string what_module; std::string filepath; };
    struct InvalidKey { std::string what_module; std::string what_def; };
    struct InvalidType { std::string what_module; std::string what_def; std::string what_field; sol::type what_type_wanted; sol::type what_type_provided; };

    using AnyIssue = std::variant<InvalidPath, UnknownError, LuaError, NotATable, RequiredModuleNotFound, UnnamedModule, ExtraneousElement, issues::MissingField, issues::InvalidFile, issues::InvalidKey, issues::InvalidType>;
};
