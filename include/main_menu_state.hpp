#pragma once

#include "state_stack.hpp"

// This state is unfinished.

class Main_Menu_State final : public State_Base
{
private:

	Main_Menu_State(State_Stack& arg_state_stack_handle);

public:

	void handle_events() override;
	void update(double dt) override;
	void render() override;

	static State_Base* make_state(State_Stack& app_handle);

};