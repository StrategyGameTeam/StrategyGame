#pragma once
#include <sol/sol.hpp>
#include "module.hpp"

using ResourceIssues = std::variant<LuaErrorIssue>;

struct ResourceStore {
    ResourceStore() = default;

    // Resource store should be immobile
    ResourceStore(const ResourceStore&) = delete;
    ResourceStore(ResourceStore&&) = delete;
    ResourceStore& operator= (const ResourceStore&) = delete;
    ResourceStore& operator= (ResourceStore&&) = delete;


    // Process loaded modules, load their stuff into memory, apply alterations, and optimize
    std::vector<ResourceIssues> LoadModuleResources(ModuleLoader& modl);
};