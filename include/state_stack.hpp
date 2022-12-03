#pragma once

#include <map>
#include <queue>
#include <stack>
#include <memory>

#include "raylib.h"

#include "state_base.hpp"
#include "input.hpp"
#include "resources.hpp"
#include "module.hpp"

class State_Stack
{
public:

	enum class STATES
	{
		MAIN_MENU, TEST
	};

private:

	enum class ACTIONS
	{ 
		PUSH,
		POP,
		CLEAR,
	};

	typedef State_Base* (*Factory)(State_Stack&);

	double time_passed = 0.;

	inline static double time_per_frame = 1. / 60.;

	std::map<STATES, Factory> factories;

	std::vector<std::pair<std::unique_ptr<State_Base>, STATES>> states_stack;
	std::queue<STATES> states_queue;
	std::queue<ACTIONS> actions_queue;

	void perform_queued_actions();
	void handle_events();
	void update();
	void draw();

public:
    InputMgr inputMgr;
    bool debug = true;
    ResourceStore resourceStore;
    ModuleLoader moduleLoader;

	State_Stack(STATES arg_init_state);

	void request_push(STATES state_id);
	void request_pop();
	void request_clear();

	void run();

};