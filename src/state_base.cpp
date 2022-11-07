#include "state_base.hpp"
#include "state_stack.hpp"

State_Base::State_Base(State_Stack& arg_state_stack_handle) :
	state_stack_handle(arg_state_stack_handle) {}