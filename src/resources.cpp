#include "resources.hpp"
#include "utils.hpp"

ResourceStore::~ResourceStore() {
    log::debug("UNLOADING RESOURCES");

    log::debug("UNLOADING TEXTURES");
    for(auto& def : m_product_table) {
        UnloadTexture(def.texture);
        UnloadImage(def.image);
    }

    log::debug("UNLOADING MODELS");
    for(auto& def : m_hex_table) {
        UnloadModel(def.model);
    }

    log::debug("UNLOADING DONE");
}

std::vector<ResourceIssues> ResourceStore::LoadModuleResources(ModuleLoader& ml) {
    std::vector<ResourceIssues> issues;

    // Loading products
    for(const auto& [_, mod] : ml.m_loaded_modules) {
        LoadProducts(mod, issues);
        LoadHexes(mod, issues);
        LoadWorldGen(mod, issues);
    }

    return issues;
}

void ResourceStore::LoadProducts(const Module &mod, std::vector<ResourceIssues> &issues) {
    sol::optional<sol::table> prods = mod.module_root_object["declarations"]["products"];
    if (!prods.has_value()) return; // nothing to do, no products defined
    for(const auto& [_, rtab] : prods.value()) {
        ProductKind def;
        if (rtab.get_type() != sol::type::table) {
            issues.push_back(issues::InvalidType{
                .what_module = mod.name_unsafe(),
                .what_def = "Products",
                .what_field = "N/A (root table of the product)",
                .what_type_wanted = sol::type::table,
                .what_type_provided = rtab.get_type()
            });
            continue;
        }
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
        if (rtab.get_type() != sol::type::table) {
            issues.push_back(issues::InvalidType{
                .what_module = mod.name_unsafe(),
                .what_def = "Hexes",
                .what_field = "N/A (root table of the hex)",
                .what_type_wanted = sol::type::table,
                .what_type_provided = rtab.get_type()
            });
            continue;
        }
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

        m_hex_table.push_back(def);
    }
}

void ResourceStore::LoadWorldGen(const Module &mod, std::vector<ResourceIssues> &issues) {
    using namespace sol;
    
    optional<table> wgens = mod.module_root_object["declarations"]["world_generators"];
    if (!wgens.has_value()) return;
    for(const auto& [_, rtab] : wgens.value()) {
        WorldGen def;
        if (rtab.get_type() != type::table) {
            issues.push_back(issues::InvalidType{
                .what_module = mod.name_unsafe(),
                .what_def = "WorldGenerator",
                .what_field = "N/A (root table of the world gen)",
                .what_type_wanted = type::table,
                .what_type_provided = rtab.get_type()
            });
            continue;
        }
        table gen = rtab;
        optional<string_view> name = gen["name"];
        optional<table> options = gen["options"];
        optional<protected_function> generator = gen["generator"];

        if (!name.has_value()) {
            issues.push_back(issues::MissingField{.what_module = mod.name_unsafe(), .what_def = "world generators", .fieldname = "name"});
            continue;
        }
        // options are optional
        if (!generator.has_value()) {
            issues.push_back(issues::MissingField{.what_module = mod.name_unsafe(), .what_def = "world generators", .fieldname = "generator"});
            continue;
        }

        def.name = std::string(name.value());
        def.generator = generator.value();
        if (options.has_value()) {
            // ! TODO: Finish this
            for(const auto& [key, value] : options.value()) {
                try {
                    // this is rushed, but the whole system needs some good utils
                    const string_view name = key.as<string_view>();
                    table deets = value;
                    const string_view typestring = deets["type"].get<string_view>();
                    const string_view description = deets["description"].get<string_view>();
                    if (typestring == "range") {
                        std::string description;
                        if (deets["description"].valid()) {
                            description = std::string(deets["description"].get<string_view>());
                        }

                        def.options.emplace_back(WorldGen::Option{
                            .name = std::string(name), 
                            .value = WorldGen::RangeOption{
                                .from = deets["from"].get<double>(),
                                .to = deets["to"].get<double>(),
                                .step = deets["to"].get_or(1.0),
                                .default_value = deets["default_value"].get_or(deets["from"].get<double>()),
                                .description = description
                            }
                        });
                    } else if (typestring == "selection") {
                        
                    } else if (typestring == "toggle") {

                    } else if (typestring == "seed") {

                    }
                } catch (std::exception& e) {
                    continue;
                }
            }
        }
    
        m_worldgens.push_back(def);
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

int ResourceStore::FindHexIndex(std::string name) {
    const auto it = std::ranges::find_if(m_hex_table, [&](const HexKind& hex) {
        return hex.name == name;
    });

    if (it == m_hex_table.end()) return -1;
    return std::distance(m_hex_table.begin(), it);
}

int ResourceStore::FindGeneratorIndex(std::string name) {
    const auto it = std::ranges::find_if(m_worldgens, [&](const WorldGen& gen) {
        return gen.name == name;
    });

    if (it == m_worldgens.end()) return -1;
    return std::distance(m_worldgens.begin(), it);
}

void ResourceStore::InjectSymbols(sol::state &lua) {
    using sol::as_function;

    lua.create_named_table("Defs",
        "getHex", as_function(&ResourceStore::FindHexIndex, this),
        "getProduct", as_function(&ResourceStore::FindProductIndex, this)
    );
}