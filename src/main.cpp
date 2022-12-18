#include <iostream>

#define SOL_ALL_SAFETIES_ON 1
#define SOL_PRINT_ERROR 1
#define SOL_LUAJIT 1
#define SOL_SAFE_FUNCTION_CALLS 1
#define SOL_EXCEPTIONS_SAFE_PROPAGATION 1
#include <sol/sol.hpp>

#include "raylib.h"
#define RAYMATH_IMPLEMENTATION
#include "raymath.h"

#include "behaviour_stack.hpp"
#include "app_state.hpp"
#include "behaviours/main_menu.hpp"

int main () {    
    try {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(1200, 900, "Strategy Game");
        SetTargetFPS(60);

        auto app_state = std::make_shared<AppState>();
        const auto cwd = std::filesystem::current_path();
        auto modulepath = cwd;
        modulepath.append("resources/modules");

        auto module_load_candidates = app_state->moduleLoader.ListCandidateModules(modulepath);
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
                [&](issues::InvalidPath i) { std::cout << "Module in " << i.path << " did not have a structure that allowed it to load properly\n"; any_issues_critical = true; },
                [&](issues::UnknownError i) { std::cout << "An unknown error occured. This probably is a bug in the game, and not in the module. Please notify the developers. Error: " << i.message << '\n'; any_issues_critical = true; },
                [&](issues::LuaError i) { std::cout << "There was an error while running the module code. Error: " << i.message << '\n'; any_issues_critical = true; },
                [&](issues::NotATable i) { std::cout << "Module in " << i.mod << " did not provide its information in a format that we can understand.\n"; any_issues_critical = true; },
                [&](issues::ExtraneousElement i) { std::cout << "There was an extre element in the module definition in module " << i.mod << " at " << i.in << '\n';  },
                [&](issues::RequiredModuleNotFound i) { std::cout << "Module " << i.requiree << " requires the module " << i.required << ", but it was not found\n"; any_issues_critical = true; },
                [&](issues::InvalidFile i) { std::cout << "Module " << i.what_module << " tried to load the file " << i.filepath << ", but it could not be loaded\n"; any_issues_critical = true; },
                [&](issues::MissingField i) { std::cout << "Module " << i.what_module << " provided a definition of " << i.what_def << ", but it was missing the field " << i.fieldname << "\n"; any_issues_critical = true; },
                [&](issues::InvalidKey i) { std::cout << "Module " << i.what_module << " provided an invalid key when defining " << i.what_def << '\n'; any_issues_critical = true; }
            }, issue);
        };

        for(const auto& issue : app_state->moduleLoader.LoadModules(module_load_candidates, app_state->inputMgr, app_state->resourceStore)) {
            deal_with_an_issue(issue);
        }
        if (any_issues_critical) {
            return 0;
        }

        for(const auto& issue : app_state->resourceStore.LoadModuleResources(app_state->moduleLoader)) {
            deal_with_an_issue(issue);
        }
        if (any_issues_critical) {
            return 0;
        }
        
        BehaviourStack state_stack;
        state_stack.push(new behaviours::MainMenu(app_state));
        std::cout << "========================================================\n";
        state_stack.run();
        std::cout << "========================================================2\n";
    } catch (std::exception& e) {
        std::cerr << "The whole of main crashed: " << e.what() << std::endl;
        CloseWindow();
        return -1;
    }
    CloseWindow();
    return 0;
}