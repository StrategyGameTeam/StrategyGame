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
    Textbox error_text;
    SimpleButton play_button;
    SimpleButton exit_button;
    Writebox connection_addr_writebox;
	Writebox game_id_writebox;
	Writebox nickname_writebox;

    MainMenu(std::shared_ptr<AppState> as):
        app_state(as),
        game_name("S T R A T G A M E", 0, 0, 400, 100, 40.f),
        error_text("", 0, 0, 400, 50, 20.f),
        play_button("PLAY", 0, 0, 200, 60),
        exit_button("EXIT", 0, 0, 200, 60),
        connection_addr_writebox(0, 0, "Connection address (eg 127.0.0.1:4242):"),
        game_id_writebox(0, 0, "Game ID (eg 472948):"),
        nickname_writebox(0, 0, "Nickname:")
    {}
    
    void initialize() {
        adjust_to_window();
        connection_addr_writebox.setText("localhost:4242");
        std::mt19937 rg{std::random_device{}()};
        std::uniform_int_distribution<std::string::size_type> pick(0, 1000);

        nickname_writebox.setText("Player" + std::to_string(pick(rg)));
    }
    void loop (BehaviourStack& bs);

    ~MainMenu() {}

private:
    void adjust_to_window();
};
}