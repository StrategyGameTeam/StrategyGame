#include <iostream>

#define RAYMATH_IMPLEMENTATION

#include "raylib.h"
#include "raymath.h"
#include <sol/sol.hpp>
#include "utils.hpp"
#include "module.hpp"
#include "hex.hpp"
#include "input.hpp"
#include "resources.hpp"

// I have added too much fields to that, since there is more than a single "progress point" in loading of the game
// ? @8Haze Would the new system with the stack and the frames solve that?
struct GameState {
    InputMgr inputMgr;
    bool debug = true;
    std::optional<CylinderHexWorld<HexData>> world;
    ResourceStore resourceStore;
    ModuleLoader moduleLoader;

    void RunWorldgen(WorldGen& gen, std::unordered_map<std::string, std::variant<double, std::string, bool>> options) {
        using sol::as_function;
        (void)options;

        auto& mod = moduleLoader.m_loaded_modules.at(gen.generator.lua_state());
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
                    world->at_ref_normalized(hc).tileid = vvalue.as<int>();
                }
            }
        } catch (std::exception& e) {
            std::cerr << "World Gen failed: " << e.what() << '\n';
        }
    };
};

Vector3 intersect_with_ground_plane (const Ray ray, float plane_height) {
    const auto moveunit = (plane_height-ray.position.y) / ray.direction.y;
    const auto intersection_point = Vector3Add(ray.position, Vector3Scale(ray.direction, moveunit));
    return intersection_point;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void raylib_simple_example(GameState &gs) {
    log::debug(__func__, "started");
    gs.inputMgr.registerAction({"Toggle Debug Screen",[&] { gs.debug = !gs.debug; }}, {KEY_Q,{KEY_LEFT_CONTROL}});

    Camera3D camera;
    camera.fovy = 60.0;
    camera.projection = CameraProjection::CAMERA_PERSPECTIVE;
    camera.up = Vector3{0, 1, 0};
    camera.target = Vector3{0, 0, 0};
    camera.position = Vector3{0, 10.0f, 5.0f};

    gs.inputMgr.registerAction({"Test Left",[&]{camera.position.x -= 1;camera.target.x -= 1;}}, {KEY_LEFT,{}});
    gs.inputMgr.registerAction({"Test Right",[&]{camera.position.x += 1;camera.target.x += 1; }}, {KEY_RIGHT,{}});

    // this should be done per model, but for now, we don't even have a proper tile type, so it's fine
    const auto model_bb = GetModelBoundingBox(gs.resourceStore.m_hex_table.at(0).model);
    const auto model_size = Vector3Subtract(model_bb.max, model_bb.min);
    const float scale = 2.0f / std::max({model_size.x, model_size.y, model_size.z});
    
    // for draging the map around
    Vector3 mouse_grab_point;

    while(!WindowShouldClose()) {
        if (!gs.world.has_value()) {
            log::error("World has no value at the start\n");
            break;
        }

        const auto frame_start = std::chrono::steady_clock::now();
        // for some reason, dragging around is unstable
        // i know, that the logical cursor is slightly delayed, but still, it should be delayed
        // equally for all of the frame. The exact pointthat is selected will be slightly changing,
        // i very much doubt it's because of float rounding. for the future to solve
        const auto mouse_ray = GetMouseRay(GetMousePosition(), camera);
        const auto mouse_on_ground = intersect_with_ground_plane(mouse_ray, 0.2f);
        const auto hovered_coords = HexCoords::from_world_unscaled(mouse_on_ground.x, mouse_on_ground.z);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            mouse_grab_point = mouse_on_ground;
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

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            (void)0; // do something here later on
        }

        // this is temporary and also terrible, and also shows the bad frustom in all_within_unscaled_quad
        const auto scroll = GetMouseWheelMove();
        Vector3 direction = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        camera.position = Vector3Add(camera.position, Vector3Scale(direction, scroll));

        gs.inputMgr.handleKeyboard();

        const auto light_logic_end = std::chrono::steady_clock::now();

        const auto to_render = gs.world.value().all_within_unscaled_quad(
            {top_left.x, top_left.z},    
            {top_right.x, top_right.z},    
            {bottom_left.x, bottom_left.z},    
            {bottom_right.x, bottom_right.z}    
        );

        const auto rendering_start = std::chrono::steady_clock::now();
    
        BeginDrawing();
        {
            ClearBackground(WHITE);
            BeginMode3D(camera); 
            {
                DrawGrid(10, 1.0f);
                for(const auto coords : to_render) {
                    auto hx = gs.world.value().at(coords);
                    auto tint = WHITE;
                    if (coords == hovered_coords) {
                        tint = BLUE;
                    }
                    const auto [tx, ty] = coords.to_world_unscaled();
                    if (hx.tileid != -1) {
                        DrawModelEx(gs.resourceStore.m_hex_table.at(hx.tileid).model, Vector3{tx, 0, ty}, Vector3{0, 1, 0}, 0.0, Vector3{scale, scale, scale}, tint);
                    }
                }
            }
            EndMode3D();
            if (gs.debug) {
                DrawFPS(10, 10);
                DrawText(TextFormat("Hovered: %i %i", hovered_coords.q, hovered_coords.r), 10, 30, 20, BLACK);
                const auto hovered_tile = gs.world.value().at(hovered_coords);
                if (hovered_tile.tileid != -1) {
                    DrawText(gs.resourceStore.m_hex_table.at(hovered_tile.tileid).name.c_str(), 10, 50, 20, BLACK);
                }
            }
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
        GameState gs;
        const auto cwd = std::filesystem::current_path();
        
        auto modulepath = cwd;
        modulepath.append("resources/modules");

        auto module_load_candidates = gs.moduleLoader.ListCandidateModules(modulepath);
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
                [&](issues::MissingField i) { std::cout << "Module " << i.what_module << " provided a definition of \"" << i.what_def << "\", but it was missing the field " << i.fieldname << "\n"; any_issues_critical = true; },
                [&](issues::InvalidKey i) { std::cout << "Module " << i.what_module << " provided an invalid key when defining " << i.what_def << '\n'; any_issues_critical = true; }
            }, issue);
        };

        for(const auto& issue : gs.moduleLoader.LoadModules(module_load_candidates, gs.inputMgr, gs.resourceStore)) {
            deal_with_an_issue(issue);
        }
        if (any_issues_critical) {
            return 0;
        }

        for(const auto& issue : gs.resourceStore.LoadModuleResources(gs.moduleLoader)) {
            deal_with_an_issue(issue);
        }
        if (any_issues_critical) {
            return 0;
        }

        auto def_gen_id = gs.resourceStore.FindGeneratorIndex("default");
        if (def_gen_id == -1) {
            log::debug("No world generator found, abroting");
            abort();
        }
        log::debug("BEFORE WORLDGEN");
        auto& def_gen = gs.resourceStore.m_worldgens.at(def_gen_id);
        log::debug("JUST BEFORE WORLDGEN");
        gs.RunWorldgen(*def_gen, {});
        log::debug("AFTER WORLDGEN");
        raylib_simple_example(gs);
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