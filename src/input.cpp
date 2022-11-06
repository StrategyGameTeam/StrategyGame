#include "input.hpp"
#include <vector>
#include <iostream>

void InputMgr::registerAction(const Action &action, const ActionShortcut &defaultShortcut) {
    if (std::find_if(actions.begin(), actions.end(), [&](const auto &item) {
        return item.name == action.name;
    }) != actions.end()) {
        throw std::invalid_argument("Redefinition of action");
    }
    shortcuts.try_emplace(action.name, defaultShortcut);
    actions.push_back(action);
}

void InputMgr::processKeyPress(int key) {
    for (const auto &action: actions) {
        auto shortcut = shortcuts.find(action.name);
        if (shortcut != shortcuts.end() && shortcut->second.shouldFire(key)) {
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
    lua.set_function("RedefineActionShortcut", &InputMgr::RedefineKeyShortcut, this);
    lua["KEY_NULL"] = 0;
    lua["KEY_APOSTROPHE"] = 39;
    lua["KEY_COMMA"] = 44;
    lua["KEY_MINUS"] = 45;
    lua["KEY_PERIOD"] = 46;
    lua["KEY_SLASH"] = 47;
    lua["KEY_ZERO"] = 48;
    lua["KEY_ONE"] = 49;
    lua["KEY_TWO"] = 50;
    lua["KEY_THREE"] = 51;
    lua["KEY_FOUR"] = 52;
    lua["KEY_FIVE"] = 53;
    lua["KEY_SIX"] = 54;
    lua["KEY_SEVEN"] = 55;
    lua["KEY_EIGHT"] = 56;
    lua["KEY_NINE"] = 57;
    lua["KEY_SEMICOLON"] = 59;
    lua["KEY_EQUAL"] = 61;
    lua["KEY_A"] = 65;
    lua["KEY_B"] = 66;
    lua["KEY_C"] = 67;
    lua["KEY_D"] = 68;
    lua["KEY_E"] = 69;
    lua["KEY_F"] = 70;
    lua["KEY_G"] = 71;
    lua["KEY_H"] = 72;
    lua["KEY_I"] = 73;
    lua["KEY_J"] = 74;
    lua["KEY_K"] = 75;
    lua["KEY_L"] = 76;
    lua["KEY_M"] = 77;
    lua["KEY_N"] = 78;
    lua["KEY_O"] = 79;
    lua["KEY_P"] = 80;
    lua["KEY_Q"] = 81;
    lua["KEY_R"] = 82;
    lua["KEY_S"] = 83;
    lua["KEY_T"] = 84;
    lua["KEY_U"] = 85;
    lua["KEY_V"] = 86;
    lua["KEY_W"] = 87;
    lua["KEY_X"] = 88;
    lua["KEY_Y"] = 89;
    lua["KEY_Z"] = 90;
    lua["KEY_LEFT_BRACKET"] = 91;
    lua["KEY_BACKSLASH"] = 92;
    lua["KEY_RIGHT_BRACKET"] = 93;
    lua["KEY_GRAVE"] = 96;
    lua["KEY_SPACE"] = 32;
    lua["KEY_ESCAPE"] = 256;
    lua["KEY_ENTER"] = 257;
    lua["KEY_TAB"] = 258;
    lua["KEY_BACKSPACE"] = 259;
    lua["KEY_INSERT"] = 260;
    lua["KEY_DELETE"] = 261;
    lua["KEY_RIGHT"] = 262;
    lua["KEY_LEFT"] = 263;
    lua["KEY_DOWN"] = 264;
    lua["KEY_UP"] = 265;
    lua["KEY_PAGE_UP"] = 266;
    lua["KEY_PAGE_DOWN"] = 267;
    lua["KEY_HOME"] = 268;
    lua["KEY_END"] = 269;
    lua["KEY_CAPS_LOCK"] = 280;
    lua["KEY_SCROLL_LOCK"] = 281;
    lua["KEY_NUM_LOCK"] = 282;
    lua["KEY_PRINT_SCREEN"] = 283;
    lua["KEY_PAUSE"] = 284;
    lua["KEY_F1"] = 290;
    lua["KEY_F2"] = 291;
    lua["KEY_F3"] = 292;
    lua["KEY_F4"] = 293;
    lua["KEY_F5"] = 294;
    lua["KEY_F6"] = 295;
    lua["KEY_F7"] = 296;
    lua["KEY_F8"] = 297;
    lua["KEY_F9"] = 298;
    lua["KEY_F10"] = 299;
    lua["KEY_F11"] = 300;
    lua["KEY_F12"] = 301;
    lua["KEY_LEFT_SHIFT"] = 340;
    lua["KEY_LEFT_CONTROL"] = 341;
    lua["KEY_LEFT_ALT"] = 342;
    lua["KEY_LEFT_SUPER"] = 343;
    lua["KEY_RIGHT_SHIFT"] = 344;
    lua["KEY_RIGHT_CONTROL"] = 345;
    lua["KEY_RIGHT_ALT"] = 346;
    lua["KEY_RIGHT_SUPER"] = 347;
    lua["KEY_KB_MENU"] = 348;
    lua["KEY_KP_0"] = 320;
    lua["KEY_KP_1"] = 321;
    lua["KEY_KP_2"] = 322;
    lua["KEY_KP_3"] = 323;
    lua["KEY_KP_4"] = 324;
    lua["KEY_KP_5"] = 325;
    lua["KEY_KP_6"] = 326;
    lua["KEY_KP_7"] = 327;
    lua["KEY_KP_8"] = 328;
    lua["KEY_KP_9"] = 329;
    lua["KEY_KP_DECIMAL"] = 330;
    lua["KEY_KP_DIVIDE"] = 331;
    lua["KEY_KP_MULTIPLY"] = 332;
    lua["KEY_KP_SUBTRACT"] = 333;
    lua["KEY_KP_ADD"] = 334;
    lua["KEY_KP_ENTER"] = 335;
    lua["KEY_KP_EQUAL"] = 336;
    lua["KEY_BACK"] = 4;
    lua["KEY_MENU"] = 82;
    lua["KEY_VOLUME_UP"] = 24;
    lua["KEY_VOLUME_DOWN"] = 25;
}

void InputMgr::RedefineKeyShortcut(sol::this_state ts, sol::string_view sv, KeyboardKey key, std::array<KeyboardKey, MAX_MODIFIERS> modifiers) {
    //throw exception because luajit puts 1 when variable doesn't exist
    if(key == 1 || std::find(modifiers.begin(), modifiers.end(), 1) != modifiers.end()){
        std::cout << "Could not redefine shortcut - Unsupported key\n";
        throw std::invalid_argument("Unsupported key: 1");
    }

    shortcuts.insert_or_assign(std::string(sv), ActionShortcut{key, modifiers});
}

bool ActionShortcut::shouldFire(int clickedKey) {
    return key == clickedKey &&
           std::all_of(modifiers.begin(), modifiers.end(),
                       [&](auto modifier) {
                           return modifier == KEY_NULL || IsKeyDown(modifier);
                       });
}
