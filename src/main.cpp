#include <iostream>

#define RAYMATH_IMPLEMENTATION

#include "raylib.h"
#include "raymath.h"
#include <sol/sol.hpp>
#include "module.hpp"
#include "hex.hpp"
#include "input.hpp"

#define NOGDI
#define NODRAWTEXT
#define NOOPENFILE
#define NOUSER
#include "connection.hpp"
#undef max
#include "ui_input.hpp"

struct GameState {
    InputMgr inputMgr;
    bool debug = true;
    CylinderHexWorld<char> world = {100, 50, (char)0, (char)3};
    Connection connection = {"127.0.0.1", 4242};
};

Vector3 intersect_with_ground_plane(const Ray ray, float plane_height) {
    const auto moveunit = (plane_height - ray.position.y) / ray.direction.y;
    const auto intersection_point = Vector3Add(ray.position, Vector3Scale(ray.direction, moveunit));
    return intersection_point;
}

void raylib_simple_example(GameState &gs) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 480, "Strategy game");
    SetTargetFPS(60);

    gs.inputMgr.registerAction({"Toggle Debug Screen",[&] { gs.debug = !gs.debug; }}, {KEY_Q,{KEY_LEFT_CONTROL}});

    Camera3D camera;
    camera.fovy = 60.0;
    camera.projection = CameraProjection::CAMERA_PERSPECTIVE;
    camera.up = Vector3{0, 1, 0};
    camera.target = Vector3{0, 0, 0};
    camera.position = Vector3{0, 10.0f, 5.0f};

    gs.inputMgr.registerAction({"Test Left",[&]{camera.position.x -= 1;camera.target.x -= 1;}}, {KEY_LEFT,{}});
    gs.inputMgr.registerAction({"Test Right",[&]{camera.position.x += 1;camera.target.x += 1; }}, {KEY_RIGHT,{}});

    std::array<Model, 5> hex_models = {{
        LoadModel("resources/hexes/grass_forest.obj"),
        LoadModel("resources/hexes/grass_hill.obj"),
        LoadModel("resources/hexes/grass.obj"),
        LoadModel("resources/hexes/stone.obj"),
        LoadModel("resources/hexes/water.obj")
    }};

    //todo: replace with proper ui element management
    std::vector<UiInput> ui_elements;
    ui_elements.push_back({0, 440, 200, 40, [&](std::string &text){
        gs.connection.write(const_cast<char *>(text.c_str()), text.size() + 1);
        return true;
    }});

    // this should be done per model, but for now, we don't even have a proper tile type, so it's fine
    const auto model_bb = GetModelBoundingBox(hex_models.at(0));
    const auto model_size = Vector3Subtract(model_bb.max, model_bb.min);
    const float scale = 2.0f / std::max({model_size.x, model_size.y, model_size.z});
    
    // for draging the map around
    Vector3 mouse_grab_point;

    while(!WindowShouldClose()) {
        const auto frame_start = std::chrono::steady_clock::now();
        // for some reason, dragging around is unstable
        // i know, that the logical cursor is slightly delayed, but still, it should be delayed
        // equally for all of the frame. The exact pointthat is selected will be slightly changing,
        // i very much doubt it's because of float rounding. for the future to solve
        const auto mouse_position = GetMousePosition();
        const auto mouse_ray = GetMouseRay(mouse_position, camera);
        const auto mouse_on_ground = intersect_with_ground_plane(mouse_ray, 0.2f);
        const auto hovered_coords = HexCoords::from_world_unscaled(mouse_on_ground.x, mouse_on_ground.z);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (auto &item: ui_elements) {
                item.is_active = item.x <= mouse_position.x && item.x + item.w >= mouse_position.x &&
                                 item.y <= mouse_position.y && item.y + item.h >= mouse_position.y;
            }
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
            if (auto hx = gs.world.at_ref_abnormal(hovered_coords)) {
                hx.value().get() = (hx.value().get() + 1) % 4;
            }
        }

        // this is temporary and also terrible, and also shows the bad frustom in all_within_unscaled_quad
        const auto scroll = GetMouseWheelMove();
        Vector3 direction = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        camera.position = Vector3Add(camera.position, Vector3Scale(direction, scroll));

        bool keyboard_handled = false;
        for (auto &item: ui_elements){
            if(item.is_active){
                item.handleKeyboard();
                keyboard_handled = true;
            }
        }
        if (!keyboard_handled) {
            gs.inputMgr.handleKeyboard();
        }

        const auto light_logic_end = std::chrono::steady_clock::now();

        const auto to_render = gs.world.all_within_unscaled_quad(
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
                    auto hx = gs.world.at(coords);
                    auto tint = WHITE;
                    if (coords == hovered_coords) {
                        tint = BLUE;
                    }
                    const auto [tx, ty] = coords.to_world_unscaled();
                    assert(hx <= 4);
                    DrawModelEx(hex_models.at(hx), Vector3{tx, 0, ty}, Vector3{0, 1, 0}, 0.0,
                                Vector3{scale, scale, scale}, tint);
                }
            }
            EndMode3D();
            if (gs.debug) {
                DrawFPS(10, 10);
                DrawText(TextFormat("Hovered: %i %i", hovered_coords.q, hovered_coords.r), 10, 30, 20, BLACK);
            }
            for (const auto &item: ui_elements) {
                item.render();
            }
        }
        EndDrawing();

        const auto rendering_end = std::chrono::steady_clock::now();

        const auto total_time = (rendering_end - frame_start).count();
        const auto rendering_time = (rendering_end - rendering_start).count();
        const auto vis_test = (rendering_start - light_logic_end).count();

        std::cout << "TIME: TOTAL=" << total_time << "   RENDERPART=" << (double)(rendering_time)/(double)(total_time) << "   VISTESTPART=" << (double)(vis_test)/(double)(total_time) << '\n';  
    }
    CloseWindow();
}

int main () {
    try {
        GameState gs;
        const auto cwd = std::filesystem::current_path();
        ModuleLoader ml;
        
        auto modulepath = cwd;
        modulepath.append("resources/modules");

        auto module_load_candidates = ml.ListCandidateModules(modulepath);
        // for the time this just disables nothing, but change the lambda to start banning stuff
        module_load_candidates.erase(
            std::remove_if(module_load_candidates.begin(), module_load_candidates.end(), 
                [](const std::filesystem::path&){return false;}
            ),
            module_load_candidates.end()
        );
        
        ml.LoadModules(module_load_candidates, gs.inputMgr,gs.world);

        raylib_simple_example(gs);
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}