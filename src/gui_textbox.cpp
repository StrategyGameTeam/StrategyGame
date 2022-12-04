#include "gui_textbox.hpp"

Textbox::Textbox(const std::string& arg_text, float arg_x, float arg_y, float arg_width, float arg_height, float arg_font_size) :
	text(arg_text),
	box{arg_x, arg_y, arg_width, arg_height},
	text_size(MeasureTextEx(GetFontDefault(), arg_text.c_str(), arg_font_size, 0.f)),
	font_size(arg_font_size) {}

void Textbox::set_position(float x, float y)
{
	box.x = x;
	box.y = y;
}

void Textbox::draw() const
{
	DrawRectanglePro(box, Vector2{box.width * 0.5f, box.height * 0.5f}, 0.f, GRAY);
	DrawTextPro(GetFontDefault(), text.c_str(), Vector2{box.x, box.y},
		Vector2{text_size.x * 0.5f, text_size.y * 0.5f}, 0.f, font_size, 0.f, BLACK);
}

float Textbox::getWidth() const {
    return box.width;
}

float Textbox::getHeight() const {
    return box.height;
}
