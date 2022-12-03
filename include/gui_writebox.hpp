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

	std::string text;
	Rectangle box;
	Rectangle box_hitbox;
	int character_limit;
	mutable int prefix = 0;
	bool active = false;

	Writebox(const Writebox&) = delete;
	Writebox(Writebox&&) = delete;

public:

	Writebox(float arg_x, float arg_y, int arg_character_limit = 100);

	bool is_active() const;
	void set_active(bool arg);
	void set_position(float x, float y);
	void handle_events(int key_pressed);
	std::string confirm();
	void draw() const;

};