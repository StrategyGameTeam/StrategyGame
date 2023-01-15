#include "behaviours/main_game.hpp"
#include <random>

namespace {
Vector3
intersect_with_ground_plane(const Ray ray, float plane_height)
{
  const auto moveunit = (plane_height - ray.position.y) / ray.direction.y;
  const auto intersection_point =
    Vector3Add(ray.position, Vector3Scale(ray.direction, moveunit));
  return intersection_point;
}
}; // namespace

behaviours::MainGame::MainGame(std::shared_ptr<PlayerState> ps)
  : player_state(ps)
{
}

void
behaviours::MainGame::initialize()
{
  GameState& gs = *player_state->gs;
  AppState& as = *gs.app_state;

  logging::debug(__func__, "started");
  as.inputMgr.registerAction(
    { "Toggle Debug Screen", [&] { as.debug = !as.debug; } },
    { KEY_Q, { KEY_LEFT_CONTROL } });

  camera.fovy = 60.0;
  camera.projection = CameraProjection::CAMERA_PERSPECTIVE;
  camera.up = Vector3{ 0, 1, 0 };
  camera.target = Vector3{ gs.startX * 1.73205080f + gs.startY * 1.73205080f/2.f, 0, gs.startY * 1.5f };
  camera.position = Vector3{ gs.startX * 1.73205080f + gs.startY * 1.73205080f/2.f, 10.0f, gs.startY * 1.5f + 5.0f };

  as.inputMgr.registerAction({ "Test Left",
                               [&] {
                                 camera.position.x -= 1.73205080f;
                                 camera.target.x -= 1.73205080f;
                               } },
                             { KEY_LEFT, {} });
  as.inputMgr.registerAction({ "Test Right",
                               [&] {
                                 camera.position.x += 1.73205080f;
                                 camera.target.x += 1.73205080f;
                               } },
                             { KEY_RIGHT, {} });
  as.inputMgr.registerAction({ "Test UP",
                               [&] {
                                   camera.position.z -= 1.5f;
                                   camera.target.z -= 1.5f;
                                   camera.position.x -= 1.73205080f/2.f;
                                   camera.target.x -= 1.73205080f/2.f;
                               } },
                             { KEY_UP, {} });
  as.inputMgr.registerAction({ "Test DOWN",
                               [&] {
                                   camera.position.z += 1.5f;
                                   camera.target.z += 1.5f;
                                   camera.position.x += 1.73205080f/2.f;
                                   camera.target.x += 1.73205080f/2.f;
                               } },
                             { KEY_DOWN, {} });

  // this should be done per model, but for now, we don't even have a proper
  // tile type, so it's fine
  auto model_bb = GetModelBoundingBox(as.resourceStore.m_hex_table.at(0).model);
  const auto model_size = Vector3Subtract(model_bb.max, model_bb.min);
  scale = 2.0f / std::max({ model_size.x, model_size.y, model_size.z });

  Image ui_atlas = LoadImage("resources/ui_big_pieces.png");
  ui_atlas_texture = LoadTextureFromImage(ui_atlas);
  UnloadImage(ui_atlas);
  cursor_atlas_position =
    Rectangle{ .x = 168, .y = 108, .width = 11, .height = 15 };

  HideCursor();
}

void
behaviours::MainGame::loop(BehaviourStack& bs)
{
  (void)bs;
  PlayerState& ps = *player_state;
  GameState& gs = *ps.gs;
  AppState& as = *gs.app_state;

  const auto frame_start = std::chrono::steady_clock::now();
  // for some reason, dragging around is unstable
  // i know, that the logical cursor is slightly delayed, but still, it should
  // be delayed equally for all of the frame. The exact pointthat is selected
  // will be slightly changing, i very much doubt it's because of float
  // rounding. for the future to solve
  const auto mouse_position = GetMousePosition();

  const auto mouse_ray = GetMouseRay(GetMousePosition(), camera);
  const auto mouse_on_ground = intersect_with_ground_plane(mouse_ray, 0.2f);
  const auto hovered_coords =
    HexCoords::from_world_unscaled(mouse_on_ground.x, mouse_on_ground.z);
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    mouse_grab_point = mouse_on_ground;
    click_start_coord = hovered_coords;
    click_start_screen_pos = mouse_position;
  }
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    const auto grab_on_screen = GetWorldToScreen(mouse_grab_point, camera);
    const auto grab_in_world =
      intersect_with_ground_plane(GetMouseRay(grab_on_screen, camera), 0.2f);
    const auto grab_offset = Vector3Subtract(grab_in_world, mouse_on_ground);
    camera.position = Vector3Add(camera.position, grab_offset);
    camera.target = Vector3Add(camera.target, grab_offset);
  }

  const auto top_left =
    intersect_with_ground_plane(GetMouseRay(Vector2{ 0, 0 }, camera), 0.0f);
  const auto top_right = intersect_with_ground_plane(
    GetMouseRay(Vector2{ (float)GetScreenWidth(), 0 }, camera), 0.0f);
  const auto bottom_left = intersect_with_ground_plane(
    GetMouseRay(Vector2{ 0, (float)GetScreenHeight() }, camera), 0.0f);
  const auto bottom_right = intersect_with_ground_plane(
    GetMouseRay({ (float)GetScreenWidth(), (float)GetScreenHeight() }, camera),
    0.0f);

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
      Vector2DistanceSqr(click_start_screen_pos, mouse_position) < 250.0f) {
    auto& units = gs.units.get_all_on_hex(hovered_coords);
    if (units.has_any()) {
      if (ps.selected_unit.has_value() &&
          ps.selected_unit.value().first == hovered_coords) {
        auto already_present_type = ps.selected_unit.value().second;
        if (already_present_type == UnitType::Millitary &&
            units.civilian.has_value()) {
          ps.selected_unit.value().second = UnitType::Civilian;
        } else if (already_present_type == UnitType::Millitary &&
                   units.special.has_value()) {
          ps.selected_unit.value().second = UnitType::Special;
        } else if (already_present_type == UnitType::Civilian &&
                   units.special.has_value()) {
          ps.selected_unit.value().second = UnitType::Special;
        } else if (already_present_type == UnitType::Civilian &&
                   units.military.has_value()) {
          ps.selected_unit.value().second = UnitType::Millitary;
        } else if (already_present_type == UnitType::Special &&
                   units.military.has_value()) {
          ps.selected_unit.value().second = UnitType::Millitary;
        } else if (already_present_type == UnitType::Special &&
                   units.civilian.has_value()) {
          ps.selected_unit.value().second = UnitType::Civilian;
        }
      } else {
        UnitType type;
        if (units.military.has_value()) {
          type = UnitType::Millitary;
        } else if (units.civilian.has_value()) {
          type = UnitType::Civilian;
        } else if (units.special.has_value()) {
          type = UnitType::Special;
        }
        ps.selected_unit = { hovered_coords, type };
      }
    } else {
      ps.selected_unit.reset();
    }
  }

  std::vector<HexCoords> movement_path;
  std::vector<PFHexCoords> closed_path;
  if (ps.selected_unit.has_value()) {
      const auto [path, closed] = gs.world.make_line(ps.selected_unit.value().first, hovered_coords, gs.units.get_all_on_hex(ps.selected_unit->first).getStamina(), [&](HexData &data){
            return as.resourceStore.m_hex_table.at(data.tileid).movement_cost;
      });
      movement_path = path;
      closed_path = closed;
  }

  if (ps.selected_unit.has_value() &&
      IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && movement_path.size() > 1 &&
          !gs.units.get_all_on_hex(movement_path.back()).has_any()) {
    auto [location, type] = ps.selected_unit.value();
    switch (type) {
      case UnitType::Millitary:
        gs.MoveUnit<UnitType::Millitary>(location, movement_path);
        break;
      case UnitType::Civilian:
        gs.MoveUnit<UnitType::Civilian>(location, movement_path);
        break;
      case UnitType::Special:
        gs.MoveUnit<UnitType::Special>(location, movement_path);
        break;
      default:;
    }
    std::for_each(movement_path.begin() + 1, movement_path.end(),[&](HexCoords &coords){
        gs.units.get_all_on_hex(movement_path.back()).addStamina(-1 * as.resourceStore.m_hex_table.at(gs.world.at_ref_normalized(coords).tileid).movement_cost);
    });

    ps.selected_unit.value().first = hovered_coords;
    gs.UpdateVission(ps.fraction);
  }

  if (IsKeyPressed(KEY_U)) {
    gs.units.put_unit_on_hex(hovered_coords,
                             MilitaryUnit{ { .id = 1, .health = 100 } });
  }

  // this is temporary and also terrible, and also shows the bad frustom in
  // all_within_unscaled_quad
  const auto scroll = GetMouseWheelMove();
  Vector3 direction =
    Vector3Normalize(Vector3Subtract(camera.target, camera.position));
  camera.position =
    Vector3Add(camera.position, Vector3Scale(direction, scroll));

  as.inputMgr.handleKeyboard();

  const auto light_logic_end = std::chrono::steady_clock::now();

  const auto to_render = ps.rendering_controller.all_within_unscaled_quad(
    { top_left.x, top_left.z },
    { top_right.x, top_right.z },
    { bottom_left.x, bottom_left.z },
    { bottom_right.x, bottom_right.z });

  const auto rendering_start = std::chrono::steady_clock::now();

  BeginDrawing();
  {
    ClearBackground(DARKBLUE);
    BeginMode3D(camera);
    {
      for (const auto coords : to_render) {
        auto hx = gs.world.at(coords);
        auto tint = WHITE;
        if (coords == hovered_coords) {
          tint = BLUE;
        }
        if(as.debug && std::find_if(closed_path.begin(), closed_path.end(),[&](HexCoords &c){
            return c.q == coords.q && c.r == coords.r && c.s == coords.s;
        }) != closed_path.end()){
            tint = RED;
        }
        const auto [tx, ty] = coords.to_world_unscaled();
        if (hx.tileid != -1 && (as.debug || hx.getFractionVisibility(ps.fraction) != HexData::Visibility::NONE)) {
          DrawModelEx(as.resourceStore.m_hex_table.at(hx.tileid).model,
                      Vector3{ tx, -0.2, ty },
                      Vector3{ 0, 1, 0 },
                      0.0,
                      Vector3{ scale, scale, scale },
                      tint);
        }

        if (ps.selected_unit && coords == ps.selected_unit.value().first) {
          DrawCircle3D(
            Vector3{ tx, 0.3, ty }, 0.8f, Vector3{ 1, 0, 0 }, 90.0f, RED);
          DrawCircle3D(
            Vector3{ tx, 0.3, ty }, 0.75f, Vector3{ 1, 0, 0 }, 90.0f, RED);
        }

        auto units = gs.units.get_all_on_hex(coords);
        if (units.military.has_value()) {
          DrawSphere(Vector3{ tx, 0.3, ty }, 0.4, RED);
        } else if (units.special.has_value()) {
          DrawSphere(Vector3{ tx, 0.3, ty }, 0.4, YELLOW);
        } else if (units.civilian.has_value()) {
          DrawSphere(Vector3{ tx, 0.3, ty }, 0.4, GREEN);
        }
      }

      for (size_t i = 0; i + 1 < movement_path.size(); i++) {
        const auto a = movement_path.at(i).to_world_unscaled();
        const auto b = movement_path.at(i + 1).to_world_unscaled();

        DrawLine3D(Vector3{ a.first, 0.5, a.second },
                   Vector3{ b.first, 0.5, b.second },
                   RED);
      }
    }
    EndMode3D();
    DrawText("Left Click Drag to move camera, Right Click to reveal area",
             150,
             10,
             20,
             BLACK);
      auto& hoveredUnits = gs.units.get_all_on_hex(hovered_coords);
      if(hoveredUnits.has_any()) {
          DrawText(TextFormat("Stamina: %i", hoveredUnits.getStamina()), GetMouseX(), GetMouseY(), 10, BLACK);
      }
    if (as.debug) {
      DrawFPS(10, 10);
      DrawText(TextFormat("Hovered: %i %i", hovered_coords.q, hovered_coords.r),
               10,
               30,
               20,
               BLACK);
      const auto hovered_tile = gs.world.at(hovered_coords);
      if (hovered_tile.tileid != -1) {
        DrawText(
          as.resourceStore.m_hex_table.at(hovered_tile.tileid).name.c_str(),
          10,
          50,
          20,
          BLACK);
      }
    }
    DrawTexturePro(ui_atlas_texture,
                   cursor_atlas_position,
                   Rectangle{ .x = (float)mouse_position.x,
                              .y = (float)mouse_position.y,
                              .width = 22,
                              .height = 30 },
                   { 0, 0 },
                   0.0f,
                   IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
                       IsMouseButtonDown(MOUSE_BUTTON_RIGHT)
                     ? GRAY
                     : WHITE);
  }
  EndDrawing();

  const auto rendering_end = std::chrono::steady_clock::now();

  const auto total_time = (rendering_end - frame_start).count();
  const auto rendering_time = (rendering_end - rendering_start).count();
  const auto vis_test = (rendering_start - light_logic_end).count();

  (void)total_time;
  (void)rendering_time;
  (void)vis_test;
}

behaviours::MainGame::~MainGame()
{
  UnloadTexture(ui_atlas_texture);
}