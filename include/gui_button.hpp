#pragma once

#include <string>
#include <functional>

#include <raylib.h>

template <typename... Args>
class Button
{
private:

	std::function<void(Args...)> function;
	std::string text;
	Rectangle box;
	Rectangle box_hitbox;
	Vector2 text_size;
	bool selected = false;

	Button(const Button&) = delete;
	Button(Button&&) = delete;

public:

	Button(const std::string& arg_text, float arg_x, float arg_y, float arg_width, float arg_height) :
		text(arg_text),
		box{arg_x, arg_y, arg_width, arg_height},
		box_hitbox{arg_x - arg_width * 0.5f, arg_y - arg_height * 0.5f, arg_width, arg_height},
		text_size(MeasureTextEx(GetFontDefault(), arg_text.c_str(), 20.f, 0.f)) {}

	void handle_events(Args... args)
	{
		if (CheckCollisionPointRec(GetMousePosition(), box_hitbox))
		{
			selected = true;
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				function(std::forward<Args>(args)...);
		}
		else
			selected = false;
	}

	bool is_selected()
	{
		return selected;
	}

	void set_function(std::function<void(Args...)> arg)
	{
		function = arg;
	}

	void set_position(float x, float y)
	{
		box.x = x;
		box.y = y;
		box_hitbox.x = x - box.width * 0.5f;
		box_hitbox.y = y - box.height * 0.5f;
	}

	void draw() const
	{
		DrawRectanglePro(box, Vector2{box.width * 0.5f, box.height * 0.5f}, 0.f, GRAY);
		DrawTextPro(GetFontDefault(), text.c_str(), Vector2{box.x, box.y},
			Vector2{text_size.x * 0.5f, text_size.y * 0.5f}, 0.f, 20.f, 0.f, (selected ? ORANGE : BLACK));
	}

};