#include "resource_store.hpp"

std::vector<ResourceIssues> ResourceStore::LoadModuleResources(ModuleLoader& ml) {
    std::vector<ResourceIssues> issues;
    for(auto it = ml.iter_start(); it != ml.iter_end(); it++) {
        auto& modtable = it->second.module_root_object;
        sol::optional<sol::string_view> name = modtable["name"];
        std::cout << "Loading Module Resources from " << name.value_or("???") << '\n';
    }
    return issues;
}