#include "main_menu_state.hpp"

Main_Menu_State::Main_Menu_State(State_Stack& arg_state_stack_handle) :
	State_Base(arg_state_stack_handle) {}

void Main_Menu_State::handle_events()
{
	if (IsKeyPressed(KEY_ENTER)) 
		state_stack_handle.request_push(State_Stack::STATES::TEST);
}

void Main_Menu_State::update(double dt) {}

void Main_Menu_State::render()
{
	DrawText("SIEMA TU KIEDYS BEDZIE POTEZNE MENU XD", 10, 30, 20, BLACK);
	DrawText("ENTER - PROSTY TESCIOR", 10, 60, 20, BLACK);
}

State_Base* Main_Menu_State::make_state(State_Stack& state_stack_handle)
{
	return new Main_Menu_State(state_stack_handle);
}