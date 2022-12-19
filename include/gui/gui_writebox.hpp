#pragma once

#include <string>

#include <raylib.h>

class Writebox
{
public:

	inline static const float width = 300.f;
	inline static const float height = 20.f;
	inline static const float font_size = 18.f;

private:

    std::string label;
	std::string text;
	Rectangle box;
	Rectangle box_hitbox;
	int character_limit;
	mutable int prefix = 0;
	bool active = false;

public:

	Writebox(float arg_x, float arg_y, int arg_character_limit = 100);
	Writebox(float arg_x, float arg_y, std::string label, int arg_character_limit = 100);

	bool is_active() const;
	void set_active(bool arg);
	void set_position(float x, float y);
	void handle_events(int key_pressed);
	std::string confirm();
	std::string getText();
	void draw() const;
    float getWidth() const;
    float getHeight() const;

};