#pragma once
#include "app_state.hpp"
#include "input.hpp"
#include "hex.hpp"
#include "resources.hpp"
#include "units.hpp"
#include <memory>

struct GameState {
    std::shared_ptr<AppState> app_state;
    CylinderHexWorld<HexData> world;
    UnitStore units;

    GameState(std::shared_ptr<AppState> as) : app_state(as) {}

    GameState(const GameState&) = delete;
    GameState(GameState&&) = delete;
    GameState& operator= (const GameState&) = delete;
    GameState& operator= (GameState&&) = delete;

    void UpdateVission(int fraction) {

    }

    template <UnitType UT>
    void MoveUnit(HexCoords from, HexCoords to) {
        auto munit = units.get_all_on_hex(from).get_opt_unit<UT>();
        if (!munit.has_value()) {
            return; // ! Maybe throw? Error is unhandled
        }
        auto unit = munit.value();
        // TODO - reaveal along path
        // TODO - allow this function to get the path
        // TODO - does this function need to get the path? or is it good enough to just, pathfind in here?
        units.teleport_unit<UT>(from, to);
        for(const auto hc : to.spiral_around(unit.vission_range)) {
            world.at_ref_normalized(hc).setFractionVisibility(unit.fraction, HexData::Visibility::SUPERIOR);
        }
    };

    void RunWorldgen(const WorldGen& gen, std::unordered_map<std::string, std::variant<double, std::string, bool>> options) {
        using sol::as_function;
        (void)options;
        
        std::cout << __func__ << " 1 \n";
        auto& mod = app_state->moduleLoader.m_loaded_modules.at(gen.generator.lua_state());
        std::cout << __func__ << " 2 \n";
        sol::table map_interface = mod.state.script(R"lua(
            return {
                AXIAL = 0,
                OFFSET = 1,
                _data = {{0}},
                _mode = 0,
                _width = 1,
                _height = 1,              
                setSize = function(self, width, height)
                    self._width = width
                    self._height = height
                    self._data = {}
                    for y=1, height do
                        self._data[y] = {}
                        for x=1, width do
                            self._data[y][x] = -1
                        end
                    end
                end,
                setTileCoords = function(self, mode)
                    self._mode = mode
                end,
                setTileAt = function(self, x, y, tile)
                    self._data[y][x] = tile
                end
            }
        )lua");
        std::cout << __func__ << " 3 \n";
        auto options_table = mod.state.create_table();
        try {
            std::cout << __func__ << " 4 \n";
            auto res = gen.generator.call(map_interface, options_table);
            std::cout << __func__ << " 5 \n";
            if (res.status() != sol::call_status::ok) {
                sol::error err = res;
                std::cerr << "Failed to run the world gen. Status = " << static_cast<int>(res.status()) << '\n';
                std::cerr << "Stack top: " << err.what() << '\n';
                abort();
            }

            std::cout << __func__ << " 6 \n";
            int w = map_interface["_width"];
            int h = map_interface["_height"];
            int mode = map_interface["_mode"];

            std::cout << __func__ << " 7 \n";
            world = CylinderHexWorld<HexData>(w, h, {}, {});
            for(const auto& [key, value] : map_interface["_data"].get<sol::table>()) {
                if (key.get_type() != sol::type::number) continue;
                if (value.get_type() != sol::type::table) continue;
                for(const auto& [kkey, vvalue] : value.as<sol::table>()) {
                    if (kkey.get_type() != sol::type::number) continue;
                    if (vvalue.get_type() != sol::type::number) continue;
                    HexCoords hc;
                    if (mode == 0) {
                        hc = HexCoords::from_axial(kkey.as<int>()-1, key.as<int>()-1);
                    } else {
                        hc = HexCoords::from_offset(kkey.as<int>()-1, key.as<int>()-1);
                    }
                    world.at_ref_normalized(hc).tileid = vvalue.as<int>();
                }
            }
            std::cout << __func__ << " 8 \n";
        } catch (std::exception& e) {
            std::cerr << "World Gen failed: " << e.what() << '\n';
        } catch (...) {
            std::cerr << "World gen somehow failed\n";
        }
    };
};