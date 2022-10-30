#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include <sol/sol.hpp>
#include "module.hpp"
#include "hex.hpp"

void raylib_simple_example() {
    InitWindow(640, 480, "Strategy game");
    SetTargetFPS(60);

    Camera3D camera;
    camera.fovy = 90.0;
    camera.projection = CameraProjection::CAMERA_PERSPECTIVE;
    camera.up = Vector3{0, 1, 0};
    camera.target = Vector3{0, 0, 0};
    camera.position = Vector3{0, 5, -10};

    std::array<Model, 5> hex_models = {{
        LoadModel("resources/hexes/grass_forest.obj"),
        LoadModel("resources/hexes/grass_hill.obj"),
        LoadModel("resources/hexes/grass.obj"),
        LoadModel("resources/hexes/water.obj"),
        LoadModel("resources/hexes/stone.obj")
    }};

    const auto model_bb = GetModelBoundingBox(hex_models.at(0));
    const auto model_size = Vector3Subtract(model_bb.max, model_bb.min);
    const float scale = 2.0f / std::max({model_size.x, model_size.y, model_size.z});

    SetCameraMode(camera, CAMERA_FREE);

    CylinderHexWorld<char> world (8, 8, false, true, 0);

    while(!WindowShouldClose()) {
        UpdateCamera(&camera);

        const auto mouse_ray = GetMouseRay(GetMousePosition(), camera);
        const auto moveunit = -mouse_ray.position.y / mouse_ray.direction.y;
        const auto mouse_ground_intersection_point = Vector3Add(mouse_ray.position, Vector3Scale(mouse_ray.direction, moveunit));

        const auto hovered_coords = HexCoords::from_world_unscaled(mouse_ground_intersection_point.x, mouse_ground_intersection_point.z);


        BeginDrawing();
            ClearBackground(WHITE);
            BeginMode3D(camera);
                DrawGrid(10, 1.0f);

                // https://www.redblobgames.com/grids/hexagons/
                for(int x = 0; x < world.width; x += 1) {
                    for(int y = 0; y < world.height; y += 1) {
                        const auto coords = HexCoords::from_axial(x, y);
                        if (const auto hx = world.at(coords)) {
                            auto tint = WHITE;
                            if (coords == hovered_coords) {
                                tint = RED;
                            }
                            const auto [x, y] = coords.to_world_unscaled();
                            assert(hx.value() <= 4);
                            DrawModelEx(hex_models.at(hx.value()), Vector3{x, 0, y}, Vector3{0, 1, 0}, 0.0, Vector3{scale, scale, scale}, tint);
                        }
                    }
                }
                // DrawPlane(Vector3Zero(), Vector2{10, 10}, BLUE);
            EndMode3D();

            DrawFPS(10, 10);
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