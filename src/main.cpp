#include <iostream>

#define RAYMATH_IMPLEMENTATION

#include "state_stack.hpp"
#include "game_state.hpp"

int main () {
    try {
        State_Stack state_stack(State_Stack::STATES::MAIN_MENU);
        state_stack.run();

    } catch (std::exception& e) {
        std::cerr << "The whole of main crashed: " << e.what() << std::endl;
        return -1;
    }
}