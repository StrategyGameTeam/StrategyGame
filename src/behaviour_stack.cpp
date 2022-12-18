#include "raylib.h"
#include "behaviour_stack.hpp"
#include "utils.hpp"

BehaviourStack::BehaviourStack() {}

void BehaviourStack::perform_queued_actions()
{
	while(!actions_stack.empty()) {
		auto& ss = states_stack; // needed because privacy
		std::visit(overloaded{
			[&](BehaviourStack::PopAction&) { pop(); },
			[&](BehaviourStack::ClearAction&) { clear(); },
			[&](BehaviourStack::PushAction& pa) { ss.push_back(pa.element); pa.run_initialize(); }
		}, actions_stack.back());
		actions_stack.pop_back();
	}
}

void BehaviourStack::update()
{
	if (WindowShouldClose()) {
		clear();
		return;
	}
	states_stack.back().run_loop(*this);
}


void BehaviourStack::defer_pop()
{
	actions_stack.push_back(PopAction{});
}

void BehaviourStack::defer_clear()
{
	actions_stack.push_back(ClearAction{});
}

void BehaviourStack::pop()
{
	states_stack.back().run_destroy();
	states_stack.pop_back();
}

void BehaviourStack::clear()
{
	for(auto it = states_stack.rbegin(); it != states_stack.rend(); it++) {
		it->run_destroy();
	}
	states_stack.clear();
}

void BehaviourStack::run()
{
	perform_queued_actions();

	while (!states_stack.empty())
	{
		update();
		perform_queued_actions();
	}
}
