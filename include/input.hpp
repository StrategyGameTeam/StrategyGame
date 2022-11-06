#pragma once
#include "raylib.h"
#include <array>
#include <vector>
#include <string>

#define MAX_MODIFIERS 2

typedef std::function<void()> ActionCallback;

struct Action {
    std::string name;
    KeyboardKey key;
    std::array<KeyboardKey, MAX_MODIFIERS> modifiers;
    ActionCallback callback;

    bool shouldFire(int key) const;
};

struct InputMgr {
    std::vector<const Action> actions;

    void registerAction(const Action &action);
    void processKeyPress(int key);
    void handleKeyboard();
};
