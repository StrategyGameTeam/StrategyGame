#pragma once
#include "input.hpp"
#include "resources.hpp"
#include "module.hpp"

// Since we need to have different elements of our state at different times, i decided to split them up into layers
// Their creation should loosely follow the operation of the game by the user
// An now, the description of those layers
// 
// AppState - True globals, things that are bound to the process. Inputs, Lua VMs, External library/device resources should reside here
// -> GameState - Things than concern the session of gameplay, but are not specific to a single player. True world state, true unit state, turn timing
// -> PlayerState - Anything that the player interacts with. UI Element Data, Rendering structures, Selections

struct AppState {
    InputMgr inputMgr;
    bool debug = false;
    ResourceStore resourceStore;
    ModuleLoader moduleLoader;

    AppState() = default;
    AppState(const AppState&) = delete;
    AppState(AppState&&) = delete;
    AppState& operator= (const AppState&) = delete;
    AppState& operator= (AppState&&) = delete;
};