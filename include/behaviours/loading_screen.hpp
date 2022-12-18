#pragma once
#include <memory>
#include "behaviour_stack.hpp"

namespace behaviours {
template <typename T>
struct LoadingScreen : public std::enable_shared_from_this<LoadingScreen<T>> {
    bool done = false;
    T* next = nullptr;
    float font_size = 32.0f;
    const char *text = "Loading...";
    float time = 0.0f;

    void signal_done(T* _next) {
        next = _next;
        done = true;
    }

    LoadingScreen() {};

    void initialize() {}; 
    void loop (BehaviourStack& bs) {
        if (done && next != nullptr) {
            // remove self, insert next, FIFO semantics
            bs.defer_push(next);
            bs.defer_pop();
        }
        
        time += GetFrameTime();
        BeginDrawing();
        {
            ClearBackground(BLACK);
            Vector2 screen_center = {(float)GetScreenWidth()/2, (float)GetScreenHeight()/2};
            auto text_size = MeasureTextEx(GetFontDefault(), text, font_size, 1.0f);
            auto top_left_corner = Vector2Subtract(screen_center, Vector2Scale(text_size, 0.5f));
            unsigned char bright = abs(sinf(time)) * 255;
            DrawTextEx(GetFontDefault(), text, top_left_corner, font_size, 1.0f, Color{255, 255, 255, bright});
        }
        EndDrawing();
    };
};
}