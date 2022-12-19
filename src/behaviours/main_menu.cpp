#include "behaviours/main_menu.hpp"
#include "behaviours/loading_screen.hpp"
#include <thread>
#include <charconv>

void behaviours::MainMenu::loop(BehaviourStack &bs) {
  if (IsWindowResized()) {
    adjust_to_window();
  }
  play_button.handle_events();
  exit_button.handle_events();

  if (play_button.is_clicked()) {
    ([&]{
      error_text.set_text("");
      auto connection_addr = this->connection_addr_writebox.getText();
      auto colon = connection_addr.find(':');
      if(colon == std::string::npos){
        error_text.set_text("Connection address must contain port number");
        return;
      }
      auto ip = connection_addr.substr(0, colon);
      if(ip.length() <= 0){
        error_text.set_text("Connection Address cannot be empty");
        return;
      }

      auto port = std::stoi(connection_addr.substr(colon+1));
      if(port < 0 || port > 64*1204){
        error_text.set_text("Port out of range");
        return;
      }
      
      auto connection = std::make_shared<Connection>(ip, port);
      auto gs = std::make_shared<GameState>(app_state, connection);
      gs->nickname = nickname_writebox.getText();

      auto loader = std::make_shared<behaviours::LoadingScreen<behaviours::MainGame>>([connection, ran = false, gs](auto&, auto& loader) mutable {      
        if (!ran) {
          ran = true;
          gs->ConnectAndInitialize([gs, loader = loader.shared_from_this()]{
            auto ps = std::make_shared<PlayerState>(gs);
            logging::debug("Ready to proceed");
            loader->signal_done(new behaviours::MainGame(ps));
          });
        }
        connection->handleTasks();
      });

      bs.defer_push(loader);
    })();
  }

  if (exit_button.is_clicked()) {
    bs.defer_clear();
  }

  auto key_pressed = GetCharPressed();
  connection_addr_writebox.handle_events(key_pressed);
  game_id_writebox.handle_events(key_pressed);
  nickname_writebox.handle_events(key_pressed);

  BeginDrawing();
  {
    ClearBackground(DARKGRAY);

    game_name.draw();
    play_button.draw();
    exit_button.draw();
    connection_addr_writebox.draw();
    game_id_writebox.draw();
    nickname_writebox.draw();
    error_text.draw();
  }
  EndDrawing();
}

void behaviours::MainMenu::adjust_to_window() {
  const int containerHeight = 380;
  //todo: auto layout
  float y = (GetScreenHeight() - containerHeight)/2;
  game_name.set_position(GetScreenWidth() * 0.5f, y);
  y += game_name.getHeight() + 10;

  connection_addr_writebox.set_position(GetScreenWidth() * 0.5f, y);
  y += connection_addr_writebox.getHeight() + 10;
  game_id_writebox.set_position(GetScreenWidth() * 0.5f, y);
  y += game_id_writebox.getHeight() + 10;
  nickname_writebox.set_position(GetScreenWidth() * 0.5f, y);
  y += nickname_writebox.getHeight() + 50;

  play_button.set_position(GetScreenWidth() * 0.5f, y);
  y += play_button.getHeight() + 10;
  exit_button.set_position(GetScreenWidth() * 0.5f, y);
  y += exit_button.getHeight();

  error_text.set_position(GetScreenWidth() * 0.5f, y);
  y += error_text.getHeight();
}