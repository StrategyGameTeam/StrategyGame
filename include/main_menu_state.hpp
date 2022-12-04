#pragma once

#include "state_stack.hpp"
#include "gui_button.hpp"
#include "gui_textbox.hpp"
#include "gui_writebox.hpp" // do wyjebania pozniej
#include "gui_chatlog.hpp" // do wyjebania pozniej

class Main_Menu_State final : public State_Base
{
private:

	Main_Menu_State(State_Stack& arg_state_stack_handle);

	Textbox game_name;
	Button<State_Stack&> play_button;
	Button<State_Stack&> exit_button;
	Writebox test_writebox; // do wyjebania pozniej
	Chatlog test_chatlog; // do wyjebania pozniej

public:

	void handle_events() override;
	void update(double dt) override;
	void render() override;

	void adjust_to_window();

	[[nodiscard]] static State_Base* make_state(State_Stack& app_handle);

};