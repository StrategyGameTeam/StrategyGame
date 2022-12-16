#pragma once

#include <string>

#include <raylib.h>

class Textbox
{
private:

	std::string text;
	Rectangle box;
	Vector2 text_size;
	float font_size;

public:

	Textbox(const std::string& arg_text, float arg_x, float arg_y, float arg_width, float arg_height, float arg_font_size = 20.f);

	void set_position(float x, float y);
	void set_text(const std::string& arg_text);
	void draw() const;

    float getWidth() const;
    float getHeight() const;
};