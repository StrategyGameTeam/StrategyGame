#pragma once
#include <sol/sol.hpp>
#include <raylib.h>
#include "module.hpp"
#include "utils.hpp"
#include "issues.hpp"

struct ProductKind {
    std::string name;
    Image image;
    Texture texture;

    ~ProductKind() {
        logger::debug(__func__);
    }
};

struct HexKind {
    std::string name;
    std::string description;
    std::vector<std::pair<int, int>> produces;
    int vision_cost = 1;
    int movement_cost = 1;
    Model model;

    ~HexKind() {
        logger::debug(__func__);
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
        logger::debug(__func__);
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

    // Type tables (optionals for possible non-initialized content)
    std::vector<ProductKind> m_product_table;
    std::vector<HexKind> m_hex_table;
    std::vector<std::unique_ptr<WorldGen>> m_worldgens;

    // Resource file utils
    std::optional<std::filesystem::path> ResolveModuleFile(const Module& m, std::filesystem::path relpath) const;
    std::filesystem::path ResolveModuleFileThrows(const Module& m, std::filesystem::path relpath) const;

    // Process loaded modules, load their stuff into memory, apply alterations, and optimize
    std::vector<issues::AnyIssue> LoadModuleResources(ModuleLoader& modl);
    
    void LoadProducts(ModuleLoader& modl, const Module& mod, std::vector<issues::AnyIssue>& issues);
    void LoadHexes(ModuleLoader& modl, const Module& mod, std::vector<issues::AnyIssue>& issues);
    void LoadWorldGen(ModuleLoader& modl, const Module& mod, std::vector<issues::AnyIssue>& issues);

    int FindProductIndex (std::string name);
    int FindHexIndex (std::string name);
    int FindGeneratorIndex (std::string name);

    const ProductKind& GetProduct(int idx);
    const HexKind& GetHex(int idx);
    const WorldGen& GetGenerator(int idx);

    // Inject things to get definitions
    void InjectSymbols(sol::state& lua);
};
