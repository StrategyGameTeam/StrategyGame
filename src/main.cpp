#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include <sol/sol.hpp>
#include "module.hpp"

void lua_simple_example() {
    constexpr auto script = 
        "function fact (n)\n"
        "   if n == 0 then\n"
        "       return 1\n"
        "   else\n"
        "       return n * fact(n-1)\n"
        "   end\n"
        "end\n"
        "\n"
        "for i=1,10 do\n"
        "   print(fact(i))\n"
        "end"
    ;

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package);
    lua.script(script);
    std::cout << '\n';
}

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

    SetCameraMode(camera, CAMERA_FREE);
    SetCameraPanControl(KEY_A);

    while(!WindowShouldClose()) {
        UpdateCamera(&camera);

        BeginDrawing();
            ClearBackground(WHITE);
            BeginMode3D(camera);
                // https://www.redblobgames.com/grids/hexagons/
                for(int x = 0; x < 25; x += 1) {
                    for(int y = 0; y < 25; y += 1) {
                        float right = x;
                        float top = y * sqrtf(3.0f) / 1.5 - (x%2==0) * sqrtf(3.0f)/2.0/1.5;

                        Vector3 pos = Vector3{right, 0, -top};

                        DrawModelEx(hex_models.at((x*14 + y*4)%5), pos, Vector3{0, 1, 0}, 30.0, Vector3{1, 1, 1}, WHITE);
                        // DrawModelWiresEx(hex_model, pos, Vector3{0, 1, 0}, 30.0, Vector3{1, 1, 1}, BLACK);
                    }
                }
            EndMode3D();
        EndDrawing();
    }

    CloseWindow();
}

int main () {
    try {
        ModuleLoader ml;
        ml.LoadModules(ml.ListCandidateModules());

        lua_simple_example();
        raylib_simple_example();
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}