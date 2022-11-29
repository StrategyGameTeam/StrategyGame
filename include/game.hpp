#pragma once

#include "input.hpp"
#include "connection.hpp"
#include "hex.hpp"

struct GameState {
    InputMgr inputMgr;
    bool debug = true;
    CylinderHexWorld<char> world = {100, 50, (char) 0, (char) 3};
    Connection connection = {"127.0.0.1", 4242};
};