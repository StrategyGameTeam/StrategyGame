#include "raylib.h"
#include "behaviour_stack.hpp"
#include "utils.hpp"

BehaviourStack::BehaviourStack() {}

void BehaviourStack::perform_queued_actions()
{
	for(auto& action : actions_queue) {
		auto& ss = states_stack; // needed because privacy
		std::visit(overloaded{
			[&](BehaviourStack::PopAction&) { ss.pop_back(); },
			[&](BehaviourStack::ClearAction&) { ss.clear(); },
			[&](BehaviourStack::PushAction& pa) { ss.push_back(pa.element); }
		}, action);
	}
}

void BehaviourStack::update()
{
	if (WindowShouldClose()) {
		clear();
	}

	BeginDrawing();
	ClearBackground(BLACK);
	states_stack.back().run_loop(*this);
	EndDrawing();
}


void BehaviourStack::defer_pop()
{
	actions_queue.push_back(PopAction{});
}

void BehaviourStack::defer_clear()
{
	actions_queue.push_back(ClearAction{});
}

void BehaviourStack::pop()
{
	states_stack.pop_back();
}

void BehaviourStack::clear()
{
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
