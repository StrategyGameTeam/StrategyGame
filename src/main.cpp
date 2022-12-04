#include <iostream>

#define RAYMATH_IMPLEMENTATION

#include "state_stack.hpp"
#include "game_state.hpp"
#include "main_menu_state.hpp"

int main () {
    try {
        State_Stack state_stack(Main_Menu_State::make_state);
        state_stack.run();
    } catch (std::exception& e) {
        std::cerr << "The whole of main crashed: " << e.what() << std::endl;
        return -1;
    }
}