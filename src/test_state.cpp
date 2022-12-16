#include "test_state.hpp"
#include "packets.hpp"
#include "game_packets.hpp"

Test_State::Test_State(State_Stack& arg_state_stack_handle, std::shared_ptr<Connection> connection, std::shared_ptr<GameState> gs) :
	State_Base(arg_state_stack_handle),
    gs(std::move(gs)),
    connection(std::move(connection)),
    chatlog(0, 0)
{
    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();

    camera.fovy = 60.0;
    camera.projection = CameraProjection::CAMERA_PERSPECTIVE;
    camera.up = Vector3{0, 1, 0};
    camera.target = Vector3{0, 0, 0};
    camera.position = Vector3{0, 10.0f, 5.0f};

    state_stack_handle.inputMgr.registerAction({"Test Left",[&]{camera.position.x -= 1;camera.target.x -= 1;}}, {KEY_LEFT,{}});
    state_stack_handle.inputMgr.registerAction({"Test Right",[&]{camera.position.x += 1;camera.target.x += 1; }}, {KEY_RIGHT,{}});

    // this should be done per model, but for now, we don't even have a proper tile type, so it's fine
    model_bb = GetModelBoundingBox(state_stack_handle.resourceStore.m_hex_table.at(0).model);
    model_size = Vector3Subtract(model_bb.max, model_bb.min);
    scale = 2.0f / std::max({model_size.x, model_size.y, model_size.z});


    if (!this->gs->world.has_value()) {
        logger::error("World has no value at the start\n");
        return;
    }

    this->connection->registerPacketHandler(ProxyDataPacket::packetId, [&](PacketReader &reader) {
        auto packet = ProxyDataPacket::deserialize(reader);
        this->gs->is_host = packet.is_host;
        this->gs->players = packet.players;
        std::cout << "Proxy data updated: " << packet.players.size() << " players" << std::endl;
    });
    this->connection->registerPacketHandler(InitializePlayerRequestPacket::packetId, [&](PacketReader &reader){
        auto packet = InitializePlayerRequestPacket::deserialize(reader);
        std::cout << "INITIALIZE PLAYER: " << packet.player << std::endl;
        this->connection->writeToPlayer(packet.player, WorldUpdatePacket{this->gs->world.value()});
    });
    this->connection->registerPacketHandler(WorldUpdatePacket::packetId, [&](PacketReader &reader){
        auto packet = WorldUpdatePacket::deserialize(reader);
        std::cout << "UPDATE WORLD" << std::endl;
    });

    chatlog.set_text_acceptor([&](std::string &msg){
        this->connection->writeToHost(ChatPacket{msg});
    });
    this->connection->registerPacketHandler(ChatPacket::packetId, [&](PacketReader &reader){
        auto packet = ChatPacket::deserialize(reader);
        chatlog.add_text(packet.msg);
    });

    this->adjust_to_window();
}

Vector3 Test_State::intersect_with_ground_plane(const Ray ray, float plane_height)
{
	const auto moveunit = (plane_height - ray.position.y) / ray.direction.y;
	const auto intersection_point = Vector3Add(ray.position, Vector3Scale(ray.direction, moveunit));
	return intersection_point;
}

void Test_State::handle_events()
{
    connection->handleTasks();

    auto key_pressed = GetCharPressed();
    chatlog.handle_events(key_pressed);

    // for some reason, dragging around is unstable
    // i know, that the logical cursor is slightly delayed, but still, it should be delayed
    // equally for all of the frame. The exact pointthat is selected will be slightly changing,
    // i very much doubt it's because of float rounding. for the future to solve
    const auto mouse_ray = GetMouseRay(GetMousePosition(), camera);
    const auto mouse_on_ground = intersect_with_ground_plane(mouse_ray, 0.2f);
    hovered_coords = HexCoords::from_world_unscaled(mouse_on_ground.x, mouse_on_ground.z);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        mouse_grab_point = mouse_on_ground;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        const auto grab_on_screen = GetWorldToScreen(mouse_grab_point, camera);
        const auto grab_in_world = intersect_with_ground_plane(GetMouseRay(grab_on_screen, camera), 0.2f);
        const auto grab_offset = Vector3Subtract(grab_in_world, mouse_on_ground);
        camera.position = Vector3Add(camera.position, grab_offset);
        camera.target = Vector3Add(camera.target, grab_offset);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        auto& tiledata = gs->world.value().at_ref_normalized(hovered_coords);
        if (tiledata.getFractionVisibility(gs->pretend_fraction) != HexData::Visibility::NONE) {
            for (const auto hc : hovered_coords.neighbours()) {
                auto rt = gs->world.value().at_ref_abnormal(hc);
                if (rt.has_value()) {
                    rt.value().get().setFractionVisibility(gs->pretend_fraction, HexData::Visibility::SUPERIOR);
                }
            }
        }
    }
}

void Test_State::update(double dt) 
{
}

void DrawTextAlignRight(const char* text, int x, int y, int font_size, Color color){
    DrawText(text, x - MeasureText(text, font_size), y, font_size, color);
}

void Test_State::render()
{
    const auto frame_start = std::chrono::steady_clock::now();

    const auto top_left = intersect_with_ground_plane(GetMouseRay(Vector2{0, 0}, camera), 0.0f);
    const auto top_right = intersect_with_ground_plane(GetMouseRay(Vector2{(float)GetScreenWidth(), 0}, camera), 0.0f);
    const auto bottom_left = intersect_with_ground_plane(GetMouseRay(Vector2{0, (float)GetScreenHeight()}, camera), 0.0f);
    const auto bottom_right = intersect_with_ground_plane(GetMouseRay({(float)GetScreenWidth(), (float)GetScreenHeight()}, camera), 0.0f);

    // this is temporary and also terrible, and also shows the bad frustom in all_within_unscaled_quad
    const auto scroll = GetMouseWheelMove();
    Vector3 direction = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    camera.position = Vector3Add(camera.position, Vector3Scale(direction, scroll));

    state_stack_handle.inputMgr.handleKeyboard();

    const auto light_logic_end = std::chrono::steady_clock::now();

    const auto to_render = gs->world.value().all_within_unscaled_quad
    (
        {top_left.x, top_left.z},
        {top_right.x, top_right.z},
        {bottom_left.x, bottom_left.z},
        {bottom_right.x, bottom_right.z}
    );

    const auto rendering_start = std::chrono::steady_clock::now();

    {
        ClearBackground(DARKBLUE);
        BeginMode3D(camera);
        {
            DrawGrid(10, 1.0f);
            for(const auto coords : to_render) {
                auto hx = gs->world.value().at(coords);
                auto tint = WHITE;
                if (coords == hovered_coords) {
                    tint = BLUE;
                }
                const auto [tx, ty] = coords.to_world_unscaled();
                if (hx.tileid != -1 && hx.getFractionVisibility(gs->pretend_fraction) != HexData::Visibility::NONE) {
                    DrawModelEx(state_stack_handle.resourceStore.m_hex_table.at(hx.tileid).model, Vector3{tx, 0, ty}, Vector3{0, 1, 0}, 0.0, Vector3{scale, scale, scale}, tint);
                }
            }
        }
        EndMode3D();
        DrawText("Left Click Drag to move camera, Right Click to reveal area", 150, 10, 20, BLACK);
        if (state_stack_handle.debug) {
            DrawFPS(10, 10);
            DrawText(TextFormat("Hovered: %i %i", hovered_coords.q, hovered_coords.r), 10, 30, 20, BLACK);
            const auto hovered_tile = gs->world.value().at(hovered_coords);
            if (hovered_tile.tileid != -1) {
                DrawText(state_stack_handle.resourceStore.m_hex_table.at(hovered_tile.tileid).name.c_str(), 10, 50, 20, BLACK);
            }
        }
        DrawTextAlignRight(gs->game_id.c_str(), GetScreenWidth(), 0, 20, BLACK);
        DrawTextAlignRight(gs->nickname.c_str(), GetScreenWidth(), 20, 20, BLACK);
        chatlog.draw();
    }

    const auto rendering_end = std::chrono::steady_clock::now();

    const auto total_time = (rendering_end - frame_start).count();
    const auto rendering_time = (rendering_end - rendering_start).count();
    const auto vis_test = (rendering_start - light_logic_end).count();

    (void)total_time;
    (void)rendering_time;
    (void)vis_test;

    // std::cout << "TIME: TOTAL=" << total_time << "   RENDERPART=" << (double)(rendering_time)/(double)(total_time) << "   VISTESTPART=" << (double)(vis_test)/(double)(total_time) << '\n';
}

void Test_State::adjust_to_window() {
    chatlog.set_position(5, GetScreenHeight() - chatlog.font_size - 10);
}