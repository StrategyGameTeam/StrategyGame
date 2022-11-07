#pragma once

#include "state_stack.hpp"
#include "hex.hpp"

class Test_State final : public State_Base
{
private:

	// Yeah that's a lot of data members. If we're not expecting this to improve, we can
	// consider merging the three operations (handle events, update, render) into one. But that's tbd

	Camera3D camera;

	CylinderHexWorld<char> world;
	
	std::array<Model, 5> hex_models;
	BoundingBox model_bb;
	Vector3 model_size;
	float scale;

	Vector3 mouse_grab_point;
	Ray mouse_ray;
	Vector3 mouse_on_ground;
	HexCoords hovered_coords;
	std::vector<HexCoords> to_render;

	Test_State(State_Stack& arg_state_stack_handle);

	Vector3 intersect_with_ground_plane(const Ray ray, float plane_height);

public:

	void handle_events() override;
	void update(double dt) override;
	void render() override;

	static State_Base* make_state(State_Stack& app_handle);

};