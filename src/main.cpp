#include <iostream>

#include "raylib.h"
#define RAYMATH_IMPLEMENTATION
#include "raymath.h"

#include "behaviour_stack.hpp"
#include "app_state.hpp"
#include "behaviours/main_menu.hpp"

int main () {
    try {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(1200, 900, "Strategy Game");
        SetTargetFPS(60);
        auto app_state = std::make_shared<AppState>();
        
        BehaviourStack state_stack;
        state_stack.push(new behaviours::MainMenu(app_state));
        state_stack.run();
    } catch (std::exception& e) {
        std::cerr << "The whole of main crashed: " << e.what() << std::endl;
        CloseWindow();
        return -1;
    }
    CloseWindow();
    return 0;
}