#pragma once
#include <string>

struct UiInput{
    int x;
    int y;
    int w;
    int h;
    std::function<bool(std::string &)> submit_handler;
    std::string text;
    bool is_active = false;

    void handleKeyboard();
    void render() const;
};