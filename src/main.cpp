#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include <sol/sol.hpp>
#include "module.hpp"
#include "hex.hpp"

Vector3 intersect_with_ground_plane (const Ray ray, float plane_height) {
    const auto moveunit = (plane_height-ray.position.y) / ray.direction.y;
    const auto intersection_point = Vector3Add(ray.position, Vector3Scale(ray.direction, moveunit));
    return intersection_point;
}

void raylib_simple_example() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(640, 480, "Strategy game");
    SetTargetFPS(60);

    Camera3D camera;
    camera.fovy = 60.0;
    camera.projection = CameraProjection::CAMERA_PERSPECTIVE;
    camera.up = Vector3{0, 1, 0};
    camera.target = Vector3{0, 0, 0};
    camera.position = Vector3{0, 10, 10};
    
    SetCameraMode(camera, CAMERA_CUSTOM);

    std::array<Model, 5> hex_models = {{
        LoadModel("resources/hexes/grass_forest.obj"),
        LoadModel("resources/hexes/grass_hill.obj"),
        LoadModel("resources/hexes/grass.obj"),
        LoadModel("resources/hexes/water.obj"),
        LoadModel("resources/hexes/stone.obj")
    }};

    // this should be done per model, but for now, we don't even have a proper tile type, so it's fine
    const auto model_bb = GetModelBoundingBox(hex_models.at(0));
    const auto model_size = Vector3Subtract(model_bb.max, model_bb.min);
    const float scale = 2.0f / std::max({model_size.x, model_size.y, model_size.z});
    
    // for draging the map around
    Vector3 mouse_grab_point;

    CylinderHexWorld<char> world (8, 8, 0);

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
            std::min(top_left.x, bottom_left.x),
            std::min(top_left.z, top_right.z),
            std::max(std::abs(top_left.x - top_right.x), std::abs(bottom_left.x - bottom_right.x)),
            std::max(std::abs(top_left.z - bottom_left.z), std::abs(top_right.z - bottom_right.z))
        );

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (auto hx = world.at_abnormal(hovered_coords)) {
                hx.value().get() = (hx.value().get() + 1) % 5;
            }
        }

        BeginDrawing();
            puts("BeginDrawing()");
            ClearBackground(WHITE);
            BeginMode3D(camera);
                DrawSphere(top_left, 3.0, RED);
                DrawSphere(top_right, 3.0, GREEN);
                DrawSphere(bottom_right, 3.0, BLUE);
                DrawSphere(bottom_left, 3.0, YELLOW);
                DrawGrid(10, 1.0f);
                for(const auto coords : to_render) {
                    printf("%i %i\n", coords.q, coords.r);
                    auto& hx = world.at_normalized(coords);
                    auto tint = WHITE;
                    if (coords == hovered_coords) {
                        tint = BLUE;
                    }
                    const auto [tx, ty] = coords.to_world_unscaled();
                    assert(hx <= 4);
                    DrawModelEx(hex_models.at(hx), Vector3{tx, 0, ty}, Vector3{0, 1, 0}, 0.0, Vector3{scale, scale, scale}, tint);
                }
            EndMode3D();
            DrawFPS(10, 10);
            puts("EndDrawing()");
        EndDrawing();
    }
    CloseWindow();
}

int main () {
    try {
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
        ml.LoadModules(module_load_candidates);

        raylib_simple_example();
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}