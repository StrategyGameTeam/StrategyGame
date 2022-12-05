#include "main_menu_state.hpp"
#include <iostream>

Main_Menu_State::Main_Menu_State(State_Stack& arg_state_stack_handle) :
	State_Base(arg_state_stack_handle),
	game_name("S T R A T G A M E", 0, 0, 400, 100, 40.f),
	play_button("PLAY", 0, 0, 200, 60),
	exit_button("EXIT", 0, 0, 200, 60),
	test_writebox(0, 0),
	test_chatlog(0, 0)
{
	play_button.set_function([](State_Stack& state_stack)
		{
			state_stack.request_push(State_Stack::STATES::TEST);
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

	test_writebox.handle_events(key_pressed);
	if (IsKeyPressed(KEY_ENTER) && test_writebox.is_active())
		std::cout << "Tekst do poslania w swiat: " << test_writebox.confirm() << "\n";

	test_chatlog.handle_events(key_pressed);
}

void Main_Menu_State::update(double dt) {}

void Main_Menu_State::render()
{
	ClearBackground(DARKGRAY);

	game_name.draw();
	play_button.draw();
	exit_button.draw();
	test_writebox.draw();
	test_chatlog.draw();
}

void Main_Menu_State::adjust_to_window()
{
	game_name.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.2f);
	play_button.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f);
	exit_button.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.7f);
	test_writebox.set_position(GetScreenWidth() * 0.2f, GetScreenHeight() * 0.9f);
	test_chatlog.set_position(GetScreenWidth() - 400, GetScreenHeight() - 200);
}

State_Base* Main_Menu_State::make_state(State_Stack& state_stack_handle)
{
	return new Main_Menu_State(state_stack_handle);
}
