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
    {}
    
    void initialize() {
        adjust_to_window();
    }
    void loop (BehaviourStack& bs);

    ~MainMenu() {}

private:
    void adjust_to_window();
};
}