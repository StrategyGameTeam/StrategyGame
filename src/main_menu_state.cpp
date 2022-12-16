#include "main_menu_state.hpp"
#include "loading_state.hpp"
#include <iostream>

Main_Menu_State::Main_Menu_State(State_Stack& arg_state_stack_handle) :
	State_Base(arg_state_stack_handle),
	game_name("S T R A T G A M E", 0, 0, 400, 100, 40.f),
    error_text("", 0, 0, 400, 50, 20.f),
	play_button("PLAY", 0, 0, 200, 60),
	exit_button("EXIT", 0, 0, 200, 60),
	connection_addr_writebox(0, 0, "Connection address (eg 127.0.0.1:4242):"),
	game_id_writebox(0, 0, "Game ID (eg 472948):"),
	nickname_writebox(0, 0, "Nickname:")
{
	play_button.set_function([this](State_Stack& state_stack)
		{
            error_text.set_text("");
            auto connection_addr = this->connection_addr_writebox.getText();
            auto colon = connection_addr.find(':');
            if(colon == std::string::npos){
                error_text.set_text("Connection address must contain port number");
                return;
            }
            auto ip = connection_addr.substr(0, colon);
            if(ip.length() <= 0){
                error_text.set_text("Connection Address cannot be empty");
                return;
            }
            auto port = std::stoi(connection_addr.substr(colon+1));
            if(port < 0 || port > 64*1204){
                error_text.set_text("Port out of range");
                return;
            }
			state_stack.request_push<Loading_State>(ip, port, game_id_writebox.getText(), nickname_writebox.getText());
		});

	exit_button.set_function([](State_Stack& state_stack)
		{
			state_stack.request_clear();
		});

	adjust_to_window();
}

void Main_Menu_State::handle_events()
{
	play_button.handle_events(state_stack_handle);
	exit_button.handle_events(state_stack_handle);

	auto key_pressed = GetCharPressed();


    connection_addr_writebox.handle_events(key_pressed);
    game_id_writebox.handle_events(key_pressed);
    nickname_writebox.handle_events(key_pressed);

}

void Main_Menu_State::update(double dt) {}

void Main_Menu_State::render()
{
	ClearBackground(DARKGRAY);

	game_name.draw();
	play_button.draw();
	exit_button.draw();
    connection_addr_writebox.draw();
    game_id_writebox.draw();
    nickname_writebox.draw();
    error_text.draw();
}

void Main_Menu_State::adjust_to_window()
{
    const int containerHeight = 380;
    //todo: auto layout
    float y = (GetScreenHeight() - containerHeight)/2;
	game_name.set_position(GetScreenWidth() * 0.5f, y);
    y += game_name.getHeight() + 10;

    connection_addr_writebox.set_position(GetScreenWidth() * 0.5f, y);
    y += connection_addr_writebox.getHeight() + 10;
    game_id_writebox.set_position(GetScreenWidth() * 0.5f, y);
    y += game_id_writebox.getHeight() + 10;
    nickname_writebox.set_position(GetScreenWidth() * 0.5f, y);
    y += nickname_writebox.getHeight() + 50;

    play_button.set_position(GetScreenWidth() * 0.5f, y);
    y += play_button.getHeight() + 10;
	exit_button.set_position(GetScreenWidth() * 0.5f, y);
    y += exit_button.getHeight();

    error_text.set_position(GetScreenWidth() * 0.5f, y);
    y += error_text.getHeight();
}

State_Base* Main_Menu_State::make_state(State_Stack& state_stack_handle)
{
	return new Main_Menu_State(state_stack_handle);
}
