#include "gui/gui_chatlog.hpp"

Chatlog::Chatlog(float arg_x, float arg_y, int arg_character_limit) :
	Writebox(arg_x + width * 0.5f, arg_y + height * 0.5f, arg_character_limit),
	chat_box{arg_x, arg_y, width, height},
	chat_box_hitbox{arg_x, arg_y - height, width, height} {}

bool Chatlog::is_active() const
{
	return Writebox::is_active();
}

void Chatlog::set_position(float x, float y)
{
	chat_box.x = x;
	chat_box.y = y;
	chat_box_hitbox.x = x;
	chat_box_hitbox.y = y - chat_box.height;
	Writebox::set_position(x + width * 0.5f, y + Writebox::height * 0.5f + 5);
}

void Chatlog::handle_events(int key_pressed)
{
	Writebox::handle_events(key_pressed);

	if (!is_active() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), chat_box_hitbox))
		set_active(true);

	if (IsKeyPressed(KEY_ENTER))
		if (std::string result = confirm(); !result.empty())
		{
			texts.push_back(std::move(result));
			if (texts.size() > max_remembered_lines)
				texts.pop_front();
		}
}

void Chatlog::draw() const
{
	DrawRectanglePro(chat_box, Vector2{0, chat_box.height}, 0.f, (is_active() ? Color{0, 0, 0, 200} : Color{0, 0, 0, 100}));
	
	for (int a = 0; a < std::min(static_cast<int>(texts.size()), max_visible_lines); ++a)
		DrawText(texts[texts.size() - 1 - a].data(), chat_box.x, chat_box.y - 20 * (1 + a), font_size, (is_active() ? WHITE : LIGHTGRAY));	

	Writebox::draw();
}
