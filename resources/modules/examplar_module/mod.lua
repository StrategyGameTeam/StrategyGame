local proper_module_root = {
    name = "name of the module",

    required_modules = {
        "a", "list", "of", "modules", "that", "this", "one", "needs"
    },
    altered_modules = {
        "a list of modules", "that this one modifies"
    },

    declarations = {
        products = {
            { name = "Testanium", icon = "/path/to/icon/relative/to/module/root" }
        },
        hexes = {
            { 
                name = "Test hex", 
                model = "/path/to/model/relative/to/module/root",
                description = "This place contains ancient doom, only know to the most ancient of devs",
                products = {
                    Testanium = 3
                }
            }
        },
        world_generators = {
            {
                name = "Test generator",
                options = {
                    width = { type = "range", from = 50, to = 1000 },
                    height = { type = "range", from = 50, to = 1000 },
                    complexity = { type = "range", from = 0.1, to = 2.0, step = 0.1},
                    temperature = { type = "selection", options = {"Hot", "Warm", "Mild", "Cold", "Freezing"} },
                    generate_rivers = { type = "toggle" }
                },
                generator = function (Map, Options)
                    Map.setSize(Options.width, Options.height)
                    Map.setTileCoords(Map.OFFSET)
                    -- or Map.setTileCoords(Map.AXIAL)
                    Map.setTileAt(0, 0, Defs.getHex("Test hex"))
                    -- and so on
                end
            }
        }
    }
}

return {
    name = "examplar_pls_ignore"
}