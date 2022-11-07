#include "test_state.hpp"

Test_State::Test_State(State_Stack& arg_state_stack_handle) :
	State_Base(arg_state_stack_handle),
	world(200, 100, (char)0, (char)4)
{
	camera.fovy = 60.0;
	camera.projection = CameraProjection::CAMERA_PERSPECTIVE;
	camera.up = Vector3{ 0, 1, 0 };
	camera.target = Vector3{ 0, 0, 0 };
	camera.position = Vector3{ 0, 10.0f, 5.0f };

	hex_models = { {
		LoadModel("resources/hexes/grass_forest.obj"),
		LoadModel("resources/hexes/grass_hill.obj"),
		LoadModel("resources/hexes/grass.obj"),
		LoadModel("resources/hexes/stone.obj"),
		LoadModel("resources/hexes/water.obj")
	} };
}

Vector3 Test_State::intersect_with_ground_plane(const Ray ray, float plane_height)
{
	const auto moveunit = (plane_height - ray.position.y) / ray.direction.y;
	const auto intersection_point = Vector3Add(ray.position, Vector3Scale(ray.direction, moveunit));
	return intersection_point;
}

void Test_State::handle_events()
{
	if (IsKeyPressed(KEY_BACKSPACE))
		state_stack_handle.request_pop();

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) 
		mouse_grab_point = mouse_on_ground;

	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) 
	{
		const auto grab_on_screen = GetWorldToScreen(mouse_grab_point, camera);
		const auto grab_in_world = intersect_with_ground_plane(GetMouseRay(grab_on_screen, camera), 0.2f);
		const auto grab_offset = Vector3Subtract(grab_in_world, mouse_on_ground);
		camera.position = Vector3Add(camera.position, grab_offset);
		camera.target = Vector3Add(camera.target, grab_offset);
	}
}

void Test_State::update(double dt) 
{
	mouse_ray = GetMouseRay(GetMousePosition(), camera);
	mouse_on_ground = intersect_with_ground_plane(mouse_ray, 0.2f);
	hovered_coords = HexCoords::from_world_unscaled(mouse_on_ground.x, mouse_on_ground.z);

	// this should be done per model, but for now, we don't even have a proper tile type, so it's fine
	model_bb = GetModelBoundingBox(hex_models.at(0));
	model_size = Vector3Subtract(model_bb.max, model_bb.min);
	scale = 2.0f / std::max({ model_size.x, model_size.y, model_size.z });

	const auto top_left = intersect_with_ground_plane(GetMouseRay(Vector2{ 0, 0 }, camera), 0.0f);
	const auto top_right = intersect_with_ground_plane(GetMouseRay(Vector2{ (float)GetScreenWidth(), 0 }, camera), 0.0f);
	const auto bottom_left = intersect_with_ground_plane(GetMouseRay(Vector2{ 0, (float)GetScreenHeight() }, camera), 0.0f);
	const auto bottom_right = intersect_with_ground_plane(GetMouseRay({ (float)GetScreenWidth(), (float)GetScreenHeight() }, camera), 0.0f);

	to_render = world.all_within_unscaled_quad(
		{ top_left.x, top_left.z },
		{ top_right.x, top_right.z },
		{ bottom_left.x, bottom_left.z },
		{ bottom_right.x, bottom_right.z }
	);

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		if (auto hx = world.at_ref_abnormal(hovered_coords)) {
			hx.value().get() = (hx.value().get() + 1) % 4;
		}
	}

	// this is temporary and also terrible, and also shows the bad frustom in all_within_unscaled_quad
	const auto scroll = GetMouseWheelMove();
	if (abs(scroll) > 0.01f) {
		camera.fovy = Clamp(camera.fovy + scroll * 3.0f, 30.0f, 110.0f);
	};
}

void Test_State::render()
{
	BeginMode3D(camera);
	{
		DrawGrid(10, 1.0f);
		for (const auto& coords : to_render) {
			auto hx = world.at(coords);
			auto tint = WHITE;
			if (coords == hovered_coords) {
				tint = BLUE;
			}
			const auto [tx, ty] = coords.to_world_unscaled();
			DrawModelEx(hex_models.at(hx), Vector3{ tx, 0, ty }, Vector3{ 0, 1, 0 }, 0.0, Vector3{ scale, scale, scale }, tint);
		}
	}
	EndMode3D();
	DrawFPS(10, 10);
	DrawText(TextFormat("Hovered: %i %i", hovered_coords.q, hovered_coords.r), 10, 30, 20, BLACK);
}

State_Base* Test_State::make_state(State_Stack& state_stack_handle)
{
	return new Test_State(state_stack_handle);
}