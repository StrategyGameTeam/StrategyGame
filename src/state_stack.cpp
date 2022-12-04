#include <chrono>

#include "state_stack.hpp"
#include "test_state.hpp"
#include "main_menu_state.hpp"

State_Stack::State_Stack(STATES arg_init_state)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1200, 900, "Strategy Game");
	SetTargetFPS(60);

	factories.emplace(STATES::TEST, Test_State::make_state);
	factories.emplace(STATES::MAIN_MENU, Main_Menu_State::make_state);

	states_stack.emplace_back(factories.find(arg_init_state)->second(*this), arg_init_state);

    inputMgr.registerAction({"Toggle Debug Screen",[&] { debug = !debug; }}, {KEY_Q,{KEY_LEFT_CONTROL}});

    const auto cwd = std::filesystem::current_path();

    auto modulepath = cwd;
    modulepath.append("resources").append("modules");

    auto module_load_candidates = moduleLoader.ListCandidateModules(modulepath);
    // for the time this just disables nothing, but change the lambda to start banning stuff
    module_load_candidates.erase(
            std::remove_if(module_load_candidates.begin(), module_load_candidates.end(),
                           [](const std::filesystem::path&){return false;}
            ),
            module_load_candidates.end()
    );

    bool any_issues_critical = false;
    const auto deal_with_an_issue = [&](auto issue){
        std::visit(overloaded{
                [&](auto) { std::cout << "An issue that was not properly handled. Please, inform developers\n"; any_issues_critical = true; },
                [&](issues::InvalidPath &i) { std::cout << "Module in " << i.path << " did not have a structure that allowed it to load properly\n"; any_issues_critical = true; },
                [&](issues::UnknownError &i) { std::cout << "An unknown error occured. This probably is a bug in the game, and not in the module. Please notify the developers. Error: " << i.message << '\n'; any_issues_critical = true; },
                [&](issues::LuaError &i) { std::cout << "There was an error while running the module code. Error: " << i.message << '\n'; any_issues_critical = true; },
                [&](issues::NotATable &i) { std::cout << "Module in " << i.mod << " did not provide its information in a format that we can understand.\n"; any_issues_critical = true; },
                [&](issues::ExtraneousElement &i) { std::cout << "There was an extre element in the module definition in module " << i.mod << " at " << i.in << '\n';  },
                [&](issues::RequiredModuleNotFound &i) { std::cout << "Module " << i.requiree << " requires the module " << i.required << ", but it was not found\n"; any_issues_critical = true; },
                [&](issues::InvalidFile &i) { std::cout << "Module " << i.what_module << " tried to load the file " << i.filepath << ", but it could not be loaded\n"; any_issues_critical = true; },
                [&](issues::MissingField &i) { std::cout << "Module " << i.what_module << " provided a definition of \"" << i.what_def << "\", but it was missing the field " << i.fieldname << "\n"; any_issues_critical = true; },
                [&](issues::InvalidKey &i) { std::cout << "Module " << i.what_module << " provided an invalid key when defining " << i.what_def << '\n'; any_issues_critical = true; }
        }, issue);
    };

    for(const auto& issue : moduleLoader.LoadModules(module_load_candidates, inputMgr, resourceStore)) {
        deal_with_an_issue(issue);
    }
    if (any_issues_critical) {
        exit(1);
    }

    for(const auto& issue : resourceStore.LoadModuleResources(moduleLoader)) {
        deal_with_an_issue(issue);
    }
    if (any_issues_critical) {
        exit(1);
    }
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
			states_stack.emplace_back(factories.find(state)->second(*this), state);
			break;
		}
		case(ACTIONS::POP):
		{
			if (!states_stack.empty())
				states_stack.pop_back();
			break;
		}
		case(ACTIONS::CLEAR):
		{
			while (!states_stack.empty())
				states_stack.pop_back();
			break;
		}
		}
	}
}

void State_Stack::handle_events()
{
	if (WindowShouldClose())
		this->request_clear();

	if (IsWindowResized())
		for (const auto& state : states_stack)
			state.first->adjust_to_window();

	states_stack.back().first->handle_events();
}

void State_Stack::update()
{
	while (time_passed > time_per_frame)
	{
		time_passed -= time_per_frame;
		states_stack.back().first->update(time_per_frame);
	}
}

void State_Stack::draw()
{
	BeginDrawing();
	ClearBackground(WHITE);

	states_stack.back().first->render();

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