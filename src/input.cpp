#include "input.hpp"
#include <vector>

void InputMgr::registerAction(const Action &action) {
    if(std::find_if(actions.begin(), actions.end(), [&](const auto &item) {
        return item.key == action.key && item.modifiers == action.modifiers;
    }) != actions.end()){
        throw std::invalid_argument("Redefinition of action");
    }
    actions.push_back(action);
}

void InputMgr::processKeyPress(int key) {
    for (const auto &action: actions) {
        if (action.shouldFire(key)) {
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

bool Action::shouldFire(int clickedKey) const {
    return key == clickedKey &&
           std::all_of(modifiers.begin(), modifiers.end(),
                       [&](auto modifier) {
                           return modifier == KEY_NULL || IsKeyDown(modifier);
                       });
}
