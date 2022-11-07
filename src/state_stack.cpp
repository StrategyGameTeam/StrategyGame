#include <chrono>

#include "state_stack.hpp"
#include "test_state.hpp"
#include "main_menu_state.hpp"

State_Stack::State_Stack(STATES arg_init_state)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(640, 480, "Strategy Game");
	SetTargetFPS(60);

	factories.emplace(STATES::TEST, Test_State::make_state);
	factories.emplace(STATES::MAIN_MENU, Main_Menu_State::make_state);

	states_stack.emplace(factories.find(arg_init_state)->second(*this), arg_init_state);
}

void State_Stack::perform_queued_actions()
{
	while (!actions_queue.empty())
	{
		ACTIONS id = actions_queue.front();
		actions_queue.pop();

		switch (id)
		{
		case(ACTIONS::PUSH):
		{
			auto state = states_queue.front();
			states_queue.pop();
			states_stack.emplace(factories.find(state)->second(*this), state);
			break;
		}
		case(ACTIONS::POP):
		{
			if (!states_stack.empty())
				states_stack.pop();
			break;
		}
		case(ACTIONS::CLEAR):
		{
			while (!states_stack.empty())
				states_stack.pop();
			break;
		}
		}
	}
}

void State_Stack::handle_events()
{
	if (WindowShouldClose())
		this->request_clear();

	states_stack.top().first->handle_events();
}

void State_Stack::update()
{
	while (time_passed > time_per_frame)
	{
		time_passed -= time_per_frame;
		states_stack.top().first->update(time_per_frame);
	}
}

void State_Stack::draw()
{
	BeginDrawing();
	ClearBackground(WHITE);

	states_stack.top().first->render();

	EndDrawing();
}

void State_Stack::request_push(STATES state_id)
{
	actions_queue.emplace(ACTIONS::PUSH);
	states_queue.emplace(state_id);
}

void State_Stack::request_pop()
{
	actions_queue.emplace(ACTIONS::POP);
}

void State_Stack::request_clear()
{
	actions_queue.emplace(ACTIONS::CLEAR);
}

void State_Stack::run()
{
	while (!states_stack.empty())
	{
		auto t1 = std::chrono::high_resolution_clock::now();
		
		handle_events();
		update();
		draw();
		perform_queued_actions();

		auto t2 = std::chrono::high_resolution_clock::now();

		time_passed += static_cast<double>((t2 - t1).count()) / 1'000'000'000.;
	}
}