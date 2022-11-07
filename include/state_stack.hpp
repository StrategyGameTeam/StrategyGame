#pragma once

#include <map>
#include <queue>
#include <stack>
#include <memory>

#include "raylib.h"

#include "state_base.hpp"

// I am very confident with this solution because it has done wonders for me in my own game.
// I've got a couple aces up my sleeve remaining, for example: 
// inter-state communication, shared resources, accessing multiple states (not just the top one) etc.

class State_Stack
{
public:

	// This enum will hold code-names for all states we desire.
	// A few examples of future states could include: main menu, settings screen, lobby, gameplay, etc.
	enum class STATES
	{
		MAIN_MENU, TEST
	};

private:

	// Actions are used in order to signal that we want to perform an operation on the stack.
	enum class ACTIONS
	{ 
		PUSH,
		POP,
		CLEAR,
	};

	typedef State_Base* (*Factory)(State_Stack&);

	double time_passed = 0.;

	// idk how you wish for the FPS to be handled, for now I'll go with my hardcoded comfort zone
	inline static double time_per_frame = 1. / 60.;

	// Factories produce pointers to newly created state instances that are requested.
	std::map<STATES, Factory> factories;

	std::stack<std::pair<std::unique_ptr<State_Base>, STATES>> states_stack;
	std::queue<STATES> states_queue;
	std::queue<ACTIONS> actions_queue;

	void perform_queued_actions();
	void handle_events();
	void update();
	void draw();

public:

	State_Stack(STATES arg_init_state);

	void request_push(STATES state_id);
	void request_pop();
	void request_clear();

	void run();

};