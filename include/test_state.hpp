#pragma once

#include "state_stack.hpp"
#include "hex.hpp"
#include "game_state.hpp"
#include "connection.hpp"

class Test_State final : public State_Base
{
private:

    std::shared_ptr<GameState> gs;
    std::shared_ptr<Connection> connection;
	Camera3D camera;

	BoundingBox model_bb;
	Vector3 model_size;
	float scale;

	Vector3 mouse_grab_point;
	HexCoords hovered_coords;

	Vector3 intersect_with_ground_plane(const Ray ray, float plane_height);

public:

    Test_State(State_Stack& arg_state_stack_handle, std::shared_ptr<Connection> connection, std::shared_ptr<GameState> gs);

	void handle_events() override;
	void update(double dt) override;
	void render() override;
	void adjust_to_window() override;

};