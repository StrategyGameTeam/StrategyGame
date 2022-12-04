#pragma once

#include <array>

class State_Stack;

class State_Base
{
protected:

	State_Stack& state_stack_handle;

	State_Base(const State_Base&) = delete;
	State_Base(State_Base&&) = delete;

public:

	State_Base(State_Stack& arg_state_stack_handle);

	virtual void handle_events() = 0;
	virtual void update(double dt) = 0;
	virtual void render() = 0;

	virtual void adjust_to_window() = 0;

	virtual ~State_Base() = default;

};