#include <iostream>

#define RAYMATH_IMPLEMENTATION

#include "raylib.h"
#include "raymath.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "utils.hpp"
#include "module.hpp"
#include "hex.hpp"
#include "input.hpp"
#include "resources.hpp"
#include "units.hpp"
#include "rendering_controller.hpp"
#include <exception>

// Since we need to have different elements of our state at different times, i decided to split them up into layers
// Their creation should loosely follow the operation of the game by the user
// An now, the description of those layers
// 
// AppState - True globals, things that are bound to the process. Inputs, Lua VMs, External library/device resources should reside here
// -> GameState - Things than concern the session of gameplay, but are not specific to a single player. True world state, true unit state, turn timing
// -> PlayerState - Anything that the player interacts with. UI Element Data, Rendering structures, Selections

struct AppState {
    InputMgr inputMgr;
    bool debug = true;
    ResourceStore resourceStore;
    ModuleLoader moduleLoader;
};

struct GameState {
    AppState& as;
    CylinderHexWorld<HexData> world;
    UnitStore units;

    GameState(AppState& as) : as(as) {}

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

    void RunWorldgen(WorldGen& gen, std::unordered_map<std::string, std::variant<double, std::string, bool>> options) {
        using sol::as_function;
        (void)options;

        auto& mod = as.moduleLoader.m_loaded_modules.at(gen.generator.lua_state());
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

        auto options_table = mod.state.create_table();
        try {
            auto res = gen.generator.call(map_interface, options_table);
            if (res.status() != sol::call_status::ok) {
                sol::error err = res;
                std::cerr << "Failed to run the world gen. Status = " << static_cast<int>(res.status()) << '\n';
                std::cerr << "Stack top: " << err.what() << '\n';
                abort();
            }

            int w = map_interface["_width"];
            int h = map_interface["_height"];
            int mode = map_interface["_mode"];

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
        } catch (std::exception& e) {
            std::cerr << "World Gen failed: " << e.what() << '\n';
        }
    };
};

struct PlayerState {
    GameState& gs;
    RenderingController rendering_controller;
    std::optional<std::pair<HexCoords, UnitType>> selected_unit;

    PlayerState(GameState& gs) : gs(gs) {}
};

Vector3 intersect_with_ground_plane (const Ray ray, float plane_height) {
    const auto moveunit = (plane_height-ray.position.y) / ray.direction.y;
    const auto intersection_point = Vector3Add(ray.position, Vector3Scale(ray.direction, moveunit));
    return intersection_point;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void raylib_simple_example(PlayerState &ps) {
    GameState &gs = ps.gs;
    AppState &as = gs.as;

    log::debug(__func__, "started");
    as.inputMgr.registerAction({"Toggle Debug Screen",[&] { as.debug = !as.debug; }}, {KEY_Q,{KEY_LEFT_CONTROL}});

    const int pretend_fraction = 0;
    // reveal a starting area
    gs.world.at_ref_normalized(HexCoords::from_axial(1, 1)).setFractionVisibility(pretend_fraction, HexData::Visibility::SUPERIOR);
    for(auto c : HexCoords::from_axial(1, 1).neighbours()) {
        gs.world.at_ref_normalized(c).setFractionVisibility(pretend_fraction, HexData::Visibility::SUPERIOR);
    }

    Camera3D camera;
    camera.fovy = 60.0;
    camera.projection = CameraProjection::CAMERA_PERSPECTIVE;
    camera.up = Vector3{0, 1, 0};
    camera.target = Vector3{0, 0, 0};
    camera.position = Vector3{0, 10.0f, 5.0f};

    as.inputMgr.registerAction({"Test Left",[&]{camera.position.x -= 1;camera.target.x -= 1;}}, {KEY_LEFT,{}});
    as.inputMgr.registerAction({"Test Right",[&]{camera.position.x += 1;camera.target.x += 1; }}, {KEY_RIGHT,{}});

    // this should be done per model, but for now, we don't even have a proper tile type, so it's fine
    const auto model_bb = GetModelBoundingBox(gs.as.resourceStore.m_hex_table.at(0).model);
    const auto model_size = Vector3Subtract(model_bb.max, model_bb.min);
    const float scale = 2.0f / std::max({model_size.x, model_size.y, model_size.z});
    
    Image ui_atlas = LoadImage("resources/ui_big_pieces.png");
    Texture ui_atlas_texture = LoadTextureFromImage(ui_atlas);
    UnloadImage(ui_atlas);
    Rectangle cursor_atlas_position = Rectangle{.x = 168, .y = 108, .width = 11, .height = 15};

    // for draging the map around
    Vector3 mouse_grab_point;

    // For discriminating tile clicks vs dragging
    HexCoords click_start_coord;
    Vector2 click_start_screen_pos;

    HideCursor();

    while(!WindowShouldClose()) {
        const auto frame_start = std::chrono::steady_clock::now();
        // for some reason, dragging around is unstable
        // i know, that the logical cursor is slightly delayed, but still, it should be delayed
        // equally for all of the frame. The exact pointthat is selected will be slightly changing,
        // i very much doubt it's because of float rounding. for the future to solve
        const auto mouse_position = GetMousePosition();

        const auto mouse_ray = GetMouseRay(GetMousePosition(), camera);
        const auto mouse_on_ground = intersect_with_ground_plane(mouse_ray, 0.2f);
        const auto hovered_coords = HexCoords::from_world_unscaled(mouse_on_ground.x, mouse_on_ground.z);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            mouse_grab_point = mouse_on_ground;
            click_start_coord = hovered_coords;
            click_start_screen_pos = mouse_position;
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            const auto grab_on_screen = GetWorldToScreen(mouse_grab_point, camera);
            const auto grab_in_world = intersect_with_ground_plane(GetMouseRay(grab_on_screen, camera), 0.2f);
            const auto grab_offset = Vector3Subtract(grab_in_world, mouse_on_ground);
            camera.position = Vector3Add(camera.position, grab_offset);
            camera.target = Vector3Add(camera.target, grab_offset);
        }

        const auto top_left = intersect_with_ground_plane(GetMouseRay(Vector2{0, 0}, camera), 0.0f);
        const auto top_right = intersect_with_ground_plane(GetMouseRay(Vector2{(float)GetScreenWidth(), 0}, camera), 0.0f);
        const auto bottom_left = intersect_with_ground_plane(GetMouseRay(Vector2{0, (float)GetScreenHeight()}, camera), 0.0f);
        const auto bottom_right = intersect_with_ground_plane(GetMouseRay({(float)GetScreenWidth(), (float)GetScreenHeight()}, camera), 0.0f);

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && Vector2DistanceSqr(click_start_screen_pos, mouse_position) < 250.0f) {
            auto& units = gs.units.get_all_on_hex(hovered_coords);
            if (units.has_any()) {
                if (ps.selected_unit.has_value() && ps.selected_unit.value().first == hovered_coords) {
                    auto already_present_type = ps.selected_unit.value().second;
                    if (already_present_type == UnitType::Millitary && units.civilian.has_value()) { ps.selected_unit.value().second = UnitType::Civilian; }
                    else if (already_present_type == UnitType::Millitary && units.special.has_value()) { ps.selected_unit.value().second = UnitType::Special; }
                    else if (already_present_type == UnitType::Civilian && units.special.has_value()) { ps.selected_unit.value().second = UnitType::Special; }
                    else if (already_present_type == UnitType::Civilian && units.military.has_value()) { ps.selected_unit.value().second = UnitType::Millitary; }
                    else if (already_present_type == UnitType::Special && units.military.has_value()) { ps.selected_unit.value().second = UnitType::Millitary; }
                    else if (already_present_type == UnitType::Special && units.civilian.has_value()) { ps.selected_unit.value().second = UnitType::Civilian; }
                } else {
                    UnitType type;
                    if (units.military.has_value()) { type = UnitType::Millitary; }
                    else if (units.civilian.has_value()) { type = UnitType::Civilian; }
                    else if (units.special.has_value()) { type = UnitType::Special; }
                    ps.selected_unit = {hovered_coords, type};
                }
            } else {
                ps.selected_unit.reset();
            }
        }

        if (ps.selected_unit.has_value() && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            // ps.selected_unit.value().first = hovered_coords;
            auto [location, type] = ps.selected_unit.value();
            switch (type) {
                case UnitType::Millitary: gs.MoveUnit<UnitType::Millitary>(location, hovered_coords); break;
                case UnitType::Civilian: gs.MoveUnit<UnitType::Civilian>(location, hovered_coords); break;
                case UnitType::Special: gs.MoveUnit<UnitType::Special>(location, hovered_coords); break;
                default:;
            }
            ps.selected_unit.value().first = hovered_coords;
            gs.UpdateVission(pretend_fraction);
        }

        if (IsKeyPressed(KEY_U)) {
            gs.units.put_unit_on_hex(hovered_coords, MilitaryUnit{{.id = 1, .health = 100}});
        }

        // this is temporary and also terrible, and also shows the bad frustom in all_within_unscaled_quad
        const auto scroll = GetMouseWheelMove();
        Vector3 direction = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        camera.position = Vector3Add(camera.position, Vector3Scale(direction, scroll));

        gs.as.inputMgr.handleKeyboard();

        const auto light_logic_end = std::chrono::steady_clock::now();

        const auto to_render = ps.rendering_controller.all_within_unscaled_quad(
            {top_left.x, top_left.z},    
            {top_right.x, top_right.z},    
            {bottom_left.x, bottom_left.z},    
            {bottom_right.x, bottom_right.z}    
        );

        const auto rendering_start = std::chrono::steady_clock::now();
    
        std::vector<HexCoords> movement_path;
        if (ps.selected_unit.has_value()) {
            movement_path = HexCoords::make_line(ps.selected_unit.value().first, hovered_coords);
        }

        BeginDrawing();
        {
            ClearBackground(DARKBLUE);
            BeginMode3D(camera); 
            {
                DrawGrid(10, 1.0f);
                for(const auto coords : to_render) {
                    auto hx = gs.world.at(coords);
                    auto tint = WHITE;
                    if (coords == hovered_coords) {
                        tint = BLUE;
                    }
                    const auto [tx, ty] = coords.to_world_unscaled();
                    if (hx.tileid != -1 && hx.getFractionVisibility(pretend_fraction) != HexData::Visibility::NONE) {
                        DrawModelEx(gs.as.resourceStore.m_hex_table.at(hx.tileid).model, Vector3{tx, -0.2, ty}, Vector3{0, 1, 0}, 0.0, Vector3{scale, scale, scale}, tint);
                    }

                    if (ps.selected_unit && coords == ps.selected_unit.value().first) {
                        DrawCircle3D(Vector3{tx, 0.3, ty}, 0.8f, Vector3{1, 0, 0}, 90.0f, RED);
                        DrawCircle3D(Vector3{tx, 0.3, ty}, 0.75f, Vector3{1, 0, 0}, 90.0f, RED);
                    }

                    auto units = gs.units.get_all_on_hex(coords);
                    if (units.military.has_value()) {
                        DrawSphere(Vector3{tx, 0.3, ty}, 0.4, RED);
                    } else if (units.special.has_value()) {
                        DrawSphere(Vector3{tx, 0.3, ty}, 0.4, YELLOW);
                    } else if (units.civilian.has_value()) {
                        DrawSphere(Vector3{tx, 0.3, ty}, 0.4, GREEN);
                    }
                }

                for(size_t i = 0; i + 1 < movement_path.size(); i++) {
                    const auto a = movement_path.at(i).to_world_unscaled();
                    const auto b = movement_path.at(i+1).to_world_unscaled();

                    DrawLine3D(
                        Vector3{a.first, 0.5, a.second},
                        Vector3{b.first, 0.5, b.second},
                        RED
                    );
                }
            }
            EndMode3D();
            DrawText("Left Click Drag to move camera, Right Click to reveal area", 150, 10, 20, BLACK);
            if (gs.as.debug) {
                DrawFPS(10, 10);
                DrawText(TextFormat("Hovered: %i %i", hovered_coords.q, hovered_coords.r), 10, 30, 20, BLACK);
                const auto hovered_tile = gs.world.at(hovered_coords);
                if (hovered_tile.tileid != -1) {
                    DrawText(gs.as.resourceStore.m_hex_table.at(hovered_tile.tileid).name.c_str(), 10, 50, 20, BLACK);
                }
            }
            DrawTexturePro(ui_atlas_texture, cursor_atlas_position, 
                Rectangle{.x = (float)mouse_position.x, .y = (float)mouse_position.y, .width = 22, .height = 30}, 
                {0, 0}, 0.0f,
                IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? GRAY : WHITE
            );
        }
        EndDrawing();

        const auto rendering_end = std::chrono::steady_clock::now();

        const auto total_time = (rendering_end - frame_start).count();
        const auto rendering_time = (rendering_end - rendering_start).count();
        const auto vis_test = (rendering_start - light_logic_end).count();

        (void)total_time;
        (void)rendering_time;
        (void)vis_test;

        // std::cout << "TIME: TOTAL=" << total_time << "   RENDERPART=" << (double)(rendering_time)/(double)(total_time) << "   VISTESTPART=" << (double)(vis_test)/(double)(total_time) << '\n';  
    }
    log::debug(__func__, "ended");
}

int main () {
    try {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 480, "Strategy game");
    SetTargetFPS(60);
    try {
        AppState as;
        const auto cwd = std::filesystem::current_path();
        
        auto modulepath = cwd;
        modulepath.append("resources/modules");

        auto module_load_candidates = as.moduleLoader.ListCandidateModules(modulepath);
        // for the time this just disables nothing, but change the lambda to start banning stuff
        module_load_candidates.erase(
            std::remove_if(module_load_candidates.begin(), module_load_candidates.end(), 
                [](const std::filesystem::path&){return false;}
            ),
            module_load_candidates.end()
        );
        
        bool any_issues_critical = false;
        const auto deal_with_an_issue = [&](auto issue){
            std::visit(overloaded{
                [&](auto) { std::cout << "An issue that was not properly handled. Please, inform developers\n"; any_issues_critical = true; },
                [&](issues::InvalidPath i) { std::cout << "Module in " << i.path << " did not have a structure that allowed it to load properly\n"; any_issues_critical = true; },
                [&](issues::UnknownError i) { std::cout << "An unknown error occured. This probably is a bug in the game, and not in the module. Please notify the developers. Error: " << i.message << '\n'; any_issues_critical = true; },
                [&](issues::LuaError i) { std::cout << "There was an error while running the module code. Error: " << i.message << '\n'; any_issues_critical = true; },
                [&](issues::NotATable i) { std::cout << "Module in " << i.mod << " did not provide its information in a format that we can understand.\n"; any_issues_critical = true; },
                [&](issues::ExtraneousElement i) { std::cout << "There was an extre element in the module definition in module " << i.mod << " at " << i.in << '\n';  },
                [&](issues::RequiredModuleNotFound i) { std::cout << "Module " << i.requiree << " requires the module " << i.required << ", but it was not found\n"; any_issues_critical = true; },
                [&](issues::InvalidFile i) { std::cout << "Module " << i.what_module << " tried to load the file " << i.filepath << ", but it could not be loaded\n"; any_issues_critical = true; },
                [&](issues::MissingField i) { std::cout << "Module " << i.what_module << " provided a definition of " << i.what_def << ", but it was missing the field " << i.fieldname << "\n"; any_issues_critical = true; },
                [&](issues::InvalidKey i) { std::cout << "Module " << i.what_module << " provided an invalid key when defining " << i.what_def << '\n'; any_issues_critical = true; }
            }, issue);
        };

        for(const auto& issue : as.moduleLoader.LoadModules(module_load_candidates, as.inputMgr, as.resourceStore)) {
            deal_with_an_issue(issue);
        }
        if (any_issues_critical) {
            return 0;
        }

        for(const auto& issue : as.resourceStore.LoadModuleResources(as.moduleLoader)) {
            deal_with_an_issue(issue);
        }
        if (any_issues_critical) {
            return 0;
        }

        auto def_gen_id = as.resourceStore.FindGeneratorIndex("default");
        if (def_gen_id == -1) {
            log::debug("No world generator found, abroting");
            abort();
        }
        log::debug("BEFORE WORLDGEN");
        auto& def_gen = as.resourceStore.m_worldgens.at(def_gen_id);
        log::debug("JUST BEFORE WORLDGEN");
        GameState gs (as);
        gs.RunWorldgen(*def_gen, {});
        log::debug("AFTER WORLDGEN");
        PlayerState ps (gs);
        raylib_simple_example(ps);
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return -1;
    }
    CloseWindow();
    log::debug("CLOSED WINDOW");
    }
    catch (std::exception& e) {
        std::cerr << "The whole of main crashed: " << e.what() << std::endl;
        return -1;
    }
}