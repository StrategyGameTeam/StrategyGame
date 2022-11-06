#include "input.hpp"
#include <vector>
#include <iostream>

void InputMgr::registerAction(const Action &action, const ActionShortcut &defaultShortcut) {
    if (std::find_if(actions.begin(), actions.end(), [&](const auto &item) {
        return item.name == action.name;
    }) != actions.end()) {
        throw std::invalid_argument("Redefinition of action");
    }
    if (!shortcuts.contains(action.name)) {
        auto [insert_it, insert_ok] = shortcuts.emplace(action.name, defaultShortcut);
        if (!insert_ok) {
            std::cout
                    << "Could not insert shortcut - this should not happen, and should be reported to the developers\n";
            return;
        }
    }
    actions.push_back(action);
}

void InputMgr::processKeyPress(int key) {
    for (const auto &action: actions) {
        if (!shortcuts.contains(action.name)) {
            continue;
        }
        auto shortcut = shortcuts[action.name];
        if (shortcut.shouldFire(key)) {
            action.callback();
        }
    }
}

void InputMgr::handleKeyboard() {
    const auto key_pressed = GetKeyPressed();
    if (key_pressed != KEY_NULL) {
        processKeyPress(key_pressed);
    }
}

void InputMgr::InjectSymbols(sol::state &lua) {
    lua.set_function("redefine_action_shortcut", &InputMgr::RedefineKeyShortcut, this);
}

void InputMgr::RedefineKeyShortcut(sol::this_state ts, sol::string_view sv, KeyboardKey key) {
    auto [insert_it, insert_ok] = shortcuts.emplace(sv, ActionShortcut{
        key, {}
    });
    if (!insert_ok) {
        std::cout << "Could not redefine shortcut - this should not happen, and should be reported to the developers\n";
    }else{
        std::cout << "Redefined \"" << sv << "\"" << std::endl;
    }
}

bool ActionShortcut::shouldFire(int clickedKey) {
    return key == clickedKey &&
           std::all_of(modifiers.begin(), modifiers.end(),
                       [&](auto modifier) {
                           return modifier == KEY_NULL || IsKeyDown(modifier);
                       });
}
