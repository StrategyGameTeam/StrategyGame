#include "gui_writebox.hpp"

Writebox::Writebox(float arg_x, float arg_y, int arg_character_limit) :
	box{arg_x, arg_y, width, height},
	box_hitbox{arg_x - width * 0.5f, arg_y - height * 0.5f, width, height},
	character_limit(arg_character_limit) {}

Writebox::Writebox(float arg_x, float arg_y, std::string label, int arg_character_limit) :
    label(std::move(label)),
	box{arg_x, arg_y + font_size, width, height},
	box_hitbox{arg_x - width * 0.5f, arg_y - height * 0.5f + font_size, width, height},
	character_limit(arg_character_limit) {}

bool Writebox::is_active() const
{
	return active;
}

void Writebox::set_active(bool arg)
{
	active = arg;
}

void Writebox::set_position(float x, float y)
{
	box.x = x;
	box.y = y + (label.length() > 0 ? font_size : 0);
	box_hitbox.x = x - box.width * 0.5f;
	box_hitbox.y = y - box.height * 0.5f + (label.length() > 0 ? font_size : 0);
}

void Writebox::handle_events(int key_pressed)
{
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		active = CheckCollisionPointRec(GetMousePosition(), box_hitbox);

	if (active)
	{
		if (IsKeyPressed(KEY_BACKSPACE) && !text.empty())
		{
			text.pop_back();
			if (prefix > 0)
				--prefix;
		}
		else if (key_pressed > 0 && text.size() < character_limit)
			text.push_back(static_cast<char>(key_pressed));
	}
}

void Writebox::draw() const
{
    if(label.length() > 0){
        DrawText(label.data(), box_hitbox.x, box_hitbox.y - font_size, font_size, WHITE);
    }
	DrawRectanglePro(box, Vector2{box.width * 0.5f, box.height * 0.5f}, 0.f, (active ? Color{0, 0, 0, 200} : Color{0, 0, 0, 100}));

	std::string_view text_view = text;
	text_view.remove_prefix(prefix);
	while (auto text_size = MeasureTextEx(GetFontDefault(), std::string(text_view).c_str(), font_size, 0.f).x > box.width - 40)
	{
		++prefix;
		text_view.remove_prefix(1);
	}

	DrawText(text_view.data(), box_hitbox.x, box_hitbox.y, font_size, (active ? WHITE : LIGHTGRAY));
}

std::string Writebox::confirm()
{
	std::string result = std::move(text);
	prefix = 0;
	return result;
}

std::string Writebox::getText()
{
	return text;
}

float Writebox::getWidth() const {
    return box.width;
}

float Writebox::getHeight() const {
    return box.height + (label.length() > 0 ? font_size : 0);
}