#pragma once
#include "raylib.h"
#include <array>
#include <vector>

typedef std::function<void()> ActionCallback;

struct Action {
    KeyboardKey key;
    std::array<KeyboardKey, 2> modifiers;
    ActionCallback callback;

    bool shouldFire(int key) const;
};

struct InputMgr {
    std::vector<const Action> actions;

    void registerAction(const Action &action);
    void processKeyPress(int key);
    void handleKeyboard();
};
