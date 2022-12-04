return {
    name = "basic_hexes",

    altered_modules = {},
    required_modules = {},

    declarations = {
        hexes = {
            require("hexes/water"),
            require("hexes/grass"),
            require("hexes/dirt"),
            require("hexes/sand"),
            require("hexes/sand_rocks"),
            require("hexes/stone"),
            require("hexes/mountain")
        }
    }
}