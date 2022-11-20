#include "resources.hpp"

ResourceStore::~ResourceStore() {
    for(auto& def : m_product_table) {
        UnloadTexture(def.texture);
    }

    for(auto& def : m_hex_table) {
        UnloadModel(def.model);
    }
}

std::vector<ResourceIssues> ResourceStore::LoadModuleResources(ModuleLoader& ml) {
    std::vector<ResourceIssues> issues;

    // Loading products
    for(const auto& [_, mod] : ml.m_loaded_modules) {
        LoadProducts(mod, issues);
        LoadHexes(mod, issues);
    }

    return issues;
}

void ResourceStore::LoadProducts(const Module &mod, std::vector<ResourceIssues> &issues) {
    sol::optional<sol::table> prods = mod.module_root_object["declarations"]["products"];
    if (!prods.has_value()) return; // nothing to do, no products defined
    for(const auto& [_, rtab] : prods.value()) {
        ProductKind def;
        sol::table tab = rtab; 
        sol::optional<sol::string_view> name = tab["name"];
        sol::optional<sol::string_view> icon = tab["icon"];
        if (!name.has_value()) {
            issues.push_back(issues::MissingField{.what_module = mod.name_unsafe(), .what_def = "products", .fieldname = "name"});
            continue;
        }
        if (!icon.has_value()) {
            issues.push_back(issues::MissingField{.what_module = mod.name_unsafe(), .what_def = "products", .fieldname = "icon"});
            continue;
        }
        const auto icon_path = ResolveModuleFile(mod, std::filesystem::path(icon.value()));
        if (!icon_path.has_value()) {
            issues.push_back(issues::InvalidFile{.what_module = mod.name_unsafe(), .filepath = std::string(icon.value())});
            continue;
        }

        def.name = std::string(name.value());
        // filesystem can have wrong char formats, so we have to go through string to fix it
        def.image = LoadImage(icon_path.value().string().c_str());
        def.texture = LoadTextureFromImage(def.image); 
        m_product_table.push_back(def);
    }
}

void ResourceStore::LoadHexes(const Module &mod, std::vector<ResourceIssues> &issues) {
    sol::optional<sol::table> hexes = mod.module_root_object["declarations"]["hexes"];
    if (!hexes.has_value()) return; // nothing to do, no products defined
    for(const auto& [_, rtab] : hexes.value()) {
        HexKind def;
        sol::table tab = rtab; 
        sol::optional<sol::string_view> name = tab["name"];
        sol::optional<sol::string_view> model = tab["model"];
        sol::optional<sol::string_view> description = tab["description"];
        sol::optional<sol::table> products = tab["products"];

        if (!name.has_value()) {
            issues.push_back(issues::MissingField{.what_module = mod.name_unsafe(), .what_def = "products", .fieldname = "name"});
            continue;
        }
        if (!model.has_value()) {
            issues.push_back(issues::MissingField{.what_module = mod.name_unsafe(), .what_def = "products", .fieldname = "icon"});
            continue;
        }
        // description is optional
        // products are optional

        const auto model_path = ResolveModuleFile(mod, std::filesystem::path(model.value()));
        if (!model_path.has_value()) {
            issues.push_back(issues::InvalidFile{.what_module = mod.name_unsafe(), .filepath = std::string(model.value())});
            continue;
        }
        
        def.name = name.value();
        def.model = LoadModel(model_path.value().string().c_str());
        if (description.has_value()) def.description = description.value();
        if (products.has_value()) {
            for(auto& [key, value] : products.value()) {
                if (key.get_type() != sol::type::string) {
                    issues.push_back(issues::InvalidKey{.what_module = mod.name_unsafe(), .what_def = "hexes"});
                    continue;
                }
                def.produces.push_back({FindProductIndex(std::string(key.as<sol::string_view>())), value.as<int>()});
            }
        }
    }
}

std::optional<std::filesystem::path> ResourceStore::ResolveModuleFile(const Module &m, std::filesystem::path relpath) const {
    auto entry = std::filesystem::path(m.entry_point);
    // security here should not be an issue - lua modules should not get a read into our memory, so we can should be ok to load anything
    const auto target = entry.remove_filename()/relpath;
    if (std::filesystem::is_regular_file(target)) {
        return target;
    } else {
        return {};
    }
}

int ResourceStore::FindProductIndex(std::string name) {
    const auto it = std::ranges::find_if(m_product_table, [&](const ProductKind& prod) {
        return prod.name == name;
    });

    if (it == m_product_table.end()) return -1;
    return std::distance(m_product_table.begin(), it);    
}