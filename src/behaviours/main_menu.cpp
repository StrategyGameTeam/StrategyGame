#include "behaviours/main_menu.hpp"
#include "behaviours/loading_screen.hpp"
#include <thread>

void behaviours::MainMenu::loop(BehaviourStack &bs) {
  if (IsWindowResized()) {
    adjust_to_window();
  }
  play_button.handle_events();
  exit_button.handle_events();

  if (play_button.is_clicked()) {
    auto loader = std::make_shared<behaviours::LoadingScreen<behaviours::MainGame>>();
    bs.defer_push(loader);
    auto next_step = std::thread([loader = loader->shared_from_this(), as = app_state]() {
      auto gs = std::make_shared<GameState>(as);
      gs->RunWorldgen(as->resourceStore.GetGenerator(
                          as->resourceStore.FindGeneratorIndex("default")),
                      {}); // this "errors" an insane number of times
      // but because LuaJIT goes through non-unwindable functions, they just,
      // get discarded
      auto ps = std::make_shared<PlayerState>(gs);
      logging::debug("Ready to proceed");
      loader->signal_done(new behaviours::MainGame(ps));
    });
    next_step.detach();
  }

  if (exit_button.is_clicked()) {
    bs.defer_clear();
  }

  auto key_pressed = GetCharPressed();

  test_writebox.handle_events(key_pressed);
  if (IsKeyPressed(KEY_ENTER) && test_writebox.is_active()) {
    std::cout << "Tekst do poslania w swiat: " << test_writebox.confirm()
              << "\n";
  }

  test_chatlog.handle_events(key_pressed);

  BeginDrawing();
  {
    ClearBackground(DARKGRAY);
    game_name.draw();
    play_button.draw();
    exit_button.draw();
    test_writebox.draw();
    test_chatlog.draw();
  }
  EndDrawing();
}

void behaviours::MainMenu::adjust_to_window() {
  game_name.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.2f);
  play_button.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f);
  exit_button.set_position(GetScreenWidth() * 0.5f, GetScreenHeight() * 0.7f);
  test_writebox.set_position(GetScreenWidth() * 0.2f, GetScreenHeight() * 0.9f);
  test_chatlog.set_position(GetScreenWidth() - 400, GetScreenHeight() - 200);
}