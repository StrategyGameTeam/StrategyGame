#pragma once
#include <sol/sol.hpp>
#include <raylib.h>
#include "module.hpp"

namespace issues {
    struct MissingField { std::string what_module; std::string what_def; std::string fieldname; };
    struct InvalidFile { std::string what_module; std::string filepath; };
    struct InvalidKey { std::string what_module; std::string what_def; };
}

using ResourceIssues = std::variant<issues::MissingField, issues::InvalidFile, issues::InvalidKey>;

struct ProductKind {
    std::string name;
    Image image;
    Texture texture;
};

struct HexKind {
    std::string name;
    std::string description;
    std::vector<std::pair<int, int>> produces;
    Model model;
};

struct WorldGen {
    std::string name;
    sol::protected_function generator;
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
    std::vector<WorldGen> m_worldgens;

    // Resource file utils
    std::optional<std::filesystem::path> ResolveModuleFile(const Module& m, std::filesystem::path relpath) const;

    // Process loaded modules, load their stuff into memory, apply alterations, and optimize
    std::vector<ResourceIssues> LoadModuleResources(ModuleLoader& modl);
    void LoadProducts(const Module& mod, std::vector<ResourceIssues>& issues);
    void LoadHexes(const Module& mod, std::vector<ResourceIssues>& issues);

    int FindProductIndex (std::string name);
};
