#pragma once
#include "raylib.h"
#include "module.hpp"
#include <array>
#include <vector>
#include <string>
#include <unordered_map>

#define MAX_MODIFIERS 2

typedef std::function<void()> ActionCallback;

struct Action {
    std::string name;
    ActionCallback callback;
};

struct ActionShortcut {
    KeyboardKey key;
    std::array<KeyboardKey, MAX_MODIFIERS> modifiers;

    bool shouldFire(int clickedKey);
};

struct InputMgr : ModuleSymbol {
    std::vector<const Action> actions;
    std::unordered_map<std::string, const ActionShortcut> shortcuts;

    void registerAction(const Action &action, const ActionShortcut &defaultShortcut);
    void processKeyPress(int key);
    void handleKeyboard();

    void InjectSymbols(sol::state &lua) override;

    void RedefineKeyShortcut(sol::this_state ts, sol::string_view sv, KeyboardKey key, std::array<KeyboardKey, MAX_MODIFIERS> modifiers);
};