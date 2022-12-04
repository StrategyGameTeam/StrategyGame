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
private:

	enum class ACTIONS
	{ 
		PUSH,
		POP,
		CLEAR
	};

	typedef State_Base* (*Factory)(State_Stack&);

	double time_passed = 0.;

	inline static const double time_per_frame = 1. / 60.;

	std::vector<std::unique_ptr<State_Base>> states_stack;
	std::queue<std::unique_ptr<State_Base>> states_queue;
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

	State_Stack(Factory initial_factory);

    template<typename T, typename ...Args>
	void request_push(Args ...args){
        actions_queue.emplace(ACTIONS::PUSH);
        states_queue.emplace(std::make_unique<T>(*this, args...));
    }
	void request_pop();
	void request_clear();

	void run();

};