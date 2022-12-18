#pragma once
#include "raylib.h"
#include "player_state.hpp"
#include <memory>
#include "utils.hpp"
#include "behaviour_stack.hpp"

namespace behaviours {
struct MainGame {
    std::shared_ptr<PlayerState> player_state;

    Camera3D camera;
    // for draging the map around
    Vector3 mouse_grab_point;

    // For discriminating tile clicks vs dragging
    HexCoords click_start_coord;
    Vector2 click_start_screen_pos;

    Texture ui_atlas_texture;
    Rectangle cursor_atlas_position;

    float scale;

    MainGame(std::shared_ptr<PlayerState> ps);
    void initialize();
    void loop (BehaviourStack& bs);
    ~MainGame();
};
}