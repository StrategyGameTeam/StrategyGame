#pragma once
#include "game_state.hpp"
#include "rendering_controller.hpp"
#include "units.hpp"
#include <optional>
#include <memory>

struct PlayerState {
    int fraction = 0;
    std::shared_ptr<GameState> gs;
    RenderingController rendering_controller;
    std::optional<std::pair<HexCoords, UnitType>> selected_unit;

    PlayerState(std::shared_ptr<GameState> gs) : gs(gs) {}

    PlayerState(const PlayerState&) = delete;
    PlayerState(PlayerState&&) = delete;
    PlayerState& operator= (const PlayerState&) = delete;
    PlayerState& operator= (PlayerState&&) = delete;
};