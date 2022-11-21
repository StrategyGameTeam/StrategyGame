#include "ui_input.hpp"
#include "raylib.h"

void UiInput::handleKeyboard() {
    auto key = GetKeyPressed();
    if (key == KEY_BACKSPACE) {
        text = text.substr(0, text.size() - 1);
    } else if (key != KEY_NULL) {
        this->text += char(key);
    }
    if (this->submit_handler && key == KEY_ENTER){
        if (this->submit_handler(text)) {
            this->text.clear();
        }
    }
}

void UiInput::render() const {
    DrawRectangle(x, y, w, h, {0, 0, 0, 64});
    auto text_to_render = text;
    if(is_active){
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now).count();
        if(seconds % 2 == 0){
            text_to_render += "_";
        }
    }
    DrawText(text_to_render.c_str(), 0, y + h / 4, h/2, {255, 255, 255, 255});
}