#define RAYMATH_IMPLEMENTATION

#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include <sol/sol.hpp>
#include "module.hpp"
#include "hex.hpp"
#include "state_stack.hpp"

int main() 
{
    try 
    {
        const auto cwd = std::filesystem::current_path();
        ModuleLoader ml;
        
        auto modulepath = cwd;
        modulepath.append("resources/modules");

        auto module_load_candidates = ml.ListCandidateModules(modulepath);
        // for the time this just disables nothing, but change the lambda to start banning stuff
        module_load_candidates.erase(
            std::remove_if(module_load_candidates.begin(), module_load_candidates.end(), 
                [](const std::filesystem::path&){return false;}
            ),
            module_load_candidates.end()
        );
        ml.LoadModules(module_load_candidates);

        State_Stack state_stack(State_Stack::STATES::MAIN_MENU);
        state_stack.run();
    } 
    catch (std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}