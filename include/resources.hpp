#pragma once
#include <sol/sol.hpp>
#include <raylib.h>
#include "module.hpp"
#include "utils.hpp"

namespace issues {
    struct MissingField { std::string what_module; std::string what_def; std::string fieldname; };
    struct InvalidFile { std::string what_module; std::string filepath; };
    struct InvalidKey { std::string what_module; std::string what_def; };
    struct InvalidType { std::string what_module; std::string what_def; std::string what_field; sol::type what_type_wanted; sol::type what_type_provided; };
}

using ResourceIssues = std::variant<issues::MissingField, issues::InvalidFile, issues::InvalidKey, issues::InvalidType>;

struct ProductKind {
    std::string name;
    Image image;
    Texture texture;

    ~ProductKind() {
        log::debug(__func__);
    }
};

struct HexKind {
    std::string name;
    std::string description;
    std::vector<std::pair<int, int>> produces;
    Model model;

    ~HexKind() {
        log::debug(__func__);
    }
};

struct WorldGen {
    struct SeedOption { bool provided; size_t value; };
    struct RangeOption { double from; double to; double step; double default_value; std::string description; };
    struct SelectionOption { std::vector<std::string> options; size_t default_selection; std::string description; };
    struct ToggleOption { std::string description; bool default_value; };
    struct Option {
        std::string name;
        std::variant<SeedOption, RangeOption, SelectionOption, ToggleOption> value;
    };

    std::string name;
    std::vector<Option> options;
    sol::protected_function generator;

    // World gens should not be movable, as they store VM references
    WorldGen() = default;
    WorldGen(const WorldGen&) = delete;
    WorldGen(WorldGen&&) = delete;

    ~WorldGen() {
        generator.abandon(); // module loader might be already dead, so no luck trying to unregister from lua vms
        log::debug(__func__);
    }
};

struct ResourceStore {
    ResourceStore() = default;
    ~ResourceStore();

    // Resource store should be immobile
    ResourceStore(const ResourceStore&) = delete;
    ResourceStore(ResourceStore&&) = delete;
    ResourceStore& operator= (const ResourceStore&) = delete;
    ResourceStore& operator= (ResourceStore&&) = delete;

    // Type tables
    std::vector<ProductKind> m_product_table;
    std::vector<HexKind> m_hex_table;
    std::vector<std::unique_ptr<WorldGen>> m_worldgens;

    // Resource file utils
    std::optional<std::filesystem::path> ResolveModuleFile(const Module& m, std::filesystem::path relpath) const;

    // Process loaded modules, load their stuff into memory, apply alterations, and optimize
    std::vector<ResourceIssues> LoadModuleResources(ModuleLoader& modl);
    
    void LoadProducts(const Module& mod, std::vector<ResourceIssues>& issues);
    void LoadHexes(const Module& mod, std::vector<ResourceIssues>& issues);
    void LoadWorldGen(const Module& mod, std::vector<ResourceIssues>& issues);

    int FindProductIndex (std::string name);
    int FindHexIndex (std::string name);
    int FindGeneratorIndex (std::string name);

    // Inject things to get definitions
    void InjectSymbols(sol::state& lua);
};
