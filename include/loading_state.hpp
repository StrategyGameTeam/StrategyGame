#pragma once

#include "state_stack.hpp"
#include "../common/connection.hpp"
#include "gui_textbox.hpp"
#include "game_state.hpp"

class Loading_State final : public State_Base
{
private:
    std::shared_ptr<GameState> gs;
    std::shared_ptr<Connection> connection;
    std::string game_id;
    std::string nickname;

    Textbox status_box;
public:
    Loading_State(State_Stack& arg_state_stack_handle,
                  std::string &ip, int port,
                  std::string &game_id, std::string &nickname);

    void handle_events() override;
    void update(double dt) override;
    void render() override;
    void adjust_to_window() override;

};