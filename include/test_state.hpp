#pragma once

#include "state_stack.hpp"
#include "hex.hpp"
#include "game_state.hpp"

class Test_State final : public State_Base
{
private:

	Camera3D camera;
    GameState gs;

	BoundingBox model_bb;
	Vector3 model_size;
	float scale;
    const int pretend_fraction = 0;

	Vector3 mouse_grab_point;
	HexCoords hovered_coords;

	Vector3 intersect_with_ground_plane(const Ray ray, float plane_height);

public:

    Test_State(State_Stack& arg_state_stack_handle);

	void handle_events() override;
	void update(double dt) override;
	void render() override;
	void adjust_to_window() override;

	[[nodiscard]] static State_Base* make_state(State_Stack& app_handle);

};