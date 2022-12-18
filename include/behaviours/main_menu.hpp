#pragma once

#include "behaviour_stack.hpp"
#include "gui/simple_button.hpp"
#include "gui/gui_textbox.hpp"
#include "gui/gui_writebox.hpp" // do wyjebania pozniej
#include "gui/gui_chatlog.hpp" // do wyjebania pozniej
#include "game_state.hpp"
#include "player_state.hpp"
#include "behaviours/main_game.hpp"

namespace behaviours {
struct MainMenu {
    std::shared_ptr<AppState> app_state;
    Textbox game_name;
    SimpleButton play_button;
    SimpleButton exit_button;
    Writebox test_writebox; // do wyjebania pozniej
    Chatlog test_chatlog; // do wyjebania pozniej
    
    MainMenu(std::shared_ptr<AppState> as):
        app_state(as),
        game_name("S T R A T G A M E", 0, 0, 400, 100, 40.f),
        play_button("PLAY", 0, 0, 200, 60),
        exit_button("EXIT", 0, 0, 200, 60),
        test_writebox(0, 0),
        test_chatlog(0, 0) 
    {
        adjust_to_window();
    }
    
    void loop (BehaviourStack& bs) {
        if (IsWindowResized()) {
            adjust_to_window();
        }
        play_button.handle_events();
        exit_button.handle_events();

        if (play_button.is_clicked()) {
            auto game_state = std::make_shared<GameState>(app_state); // ! this should not be here
            game_state->RunWorldgen(app_state->resourceStore.GetGenerator(app_state->resourceStore.FindGeneratorIndex("default")), {}); // this "errors" an insane number of times
            // but because LuaJIT goes through non-unwindable functions, they just, get discarded
            auto player_state = std::make_shared<PlayerState>(game_state);
			bs.defer_push(new behaviours::MainGame(player_state));
        }

        if (exit_button.is_clicked()) {
            std::cout << "Clearing???\n";
			bs.defer_clear();
        }

        auto key_pressed = GetCharPressed();

        test_writebox.handle_events(key_pressed);
        if (IsKeyPressed(KEY_ENTER) && test_writebox.is_active()) {
            std::cout << "Tekst do poslania w swiat: " << test_writebox.confirm() << "\n";
        }

        test_chatlog.handle_events(key_pressed);

        BeginDrawing();
            ClearBackground(DARKGRAY);

            game_name.draw();
            play_button.draw();
            exit_button.draw();
            test_writebox.draw();
            test_chatlog.draw();
        EndDrawing();
    }

    ~MainMenu() {}

private:
    void adjust_to_window() {
        game_name.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.2f);
        play_button.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f);
        exit_button.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.7f);
        test_writebox.set_position(GetScreenWidth() * 0.2f, GetScreenHeight() * 0.9f);
        test_chatlog.set_position(GetScreenWidth() - 400, GetScreenHeight() - 200);
    }
};
}