#include <iostream>

#define RAYMATH_IMPLEMENTATION

#include "raylib.h"
#include "raymath.h"
#include <sol/sol.hpp>
#include "module.hpp"
#include "hex.hpp"
#include "input.hpp"

struct GameState {
    InputMgr inputMgr;
    bool debug = true;
};

Vector3 intersect_with_ground_plane (const Ray ray, float plane_height) {
    const auto moveunit = (plane_height-ray.position.y) / ray.direction.y;
    const auto intersection_point = Vector3Add(ray.position, Vector3Scale(ray.direction, moveunit));
    return intersection_point;
}

void raylib_simple_example(GameState gs) {
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

    // this should be done per model, but for now, we don't even have a proper tile type, so it's fine
    const auto model_bb = GetModelBoundingBox(hex_models.at(0));
    const auto model_size = Vector3Subtract(model_bb.max, model_bb.min);
    const float scale = 2.0f / std::max({model_size.x, model_size.y, model_size.z});
    
    // for draging the map around
    Vector3 mouse_grab_point;

    CylinderHexWorld<char> world (200, 100, (char)0, (char)4);

    while(!WindowShouldClose()) {
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

        const auto to_render = world.all_within_unscaled_quad(
            {top_left.x, top_left.z},    
            {top_right.x, top_right.z},    
            {bottom_left.x, bottom_left.z},    
            {bottom_right.x, bottom_right.z}    
        );

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (auto hx = world.at_ref_abnormal(hovered_coords)) {
                hx.value().get() = (hx.value().get() + 1) % 4;
            }
        }

        // this is temporary and also terrible, and also shows the bad frustom in all_within_unscaled_quad
        const auto scroll = GetMouseWheelMove();
        if (abs(scroll) > 0.01f) {
            camera.fovy = Clamp(camera.fovy + scroll * 3.0f, 30.0f, 110.0f);
        };

        gs.inputMgr.handleKeyboard();

        BeginDrawing();
        {
            ClearBackground(WHITE);
            BeginMode3D(camera); 
            {
                DrawGrid(10, 1.0f);
                for(const auto coords : to_render) {
                    auto hx = world.at(coords);
                    auto tint = WHITE;
                    if (coords == hovered_coords) {
                        tint = BLUE;
                    }
                    const auto [tx, ty] = coords.to_world_unscaled();
                    assert(hx <= 4);
                    DrawModelEx(hex_models.at(hx), Vector3{tx, 0, ty}, Vector3{0, 1, 0}, 0.0, Vector3{scale, scale, scale}, tint);
                }
            }
            EndMode3D();
            if (gs.debug) {
                DrawFPS(10, 10);
                DrawText(TextFormat("Hovered: %i %i", hovered_coords.q, hovered_coords.r), 10, 30, 20, BLACK);
            }
        }
        EndDrawing();
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
        
        ml.LoadModules(module_load_candidates, gs.inputMgr);

        raylib_simple_example(gs);
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}