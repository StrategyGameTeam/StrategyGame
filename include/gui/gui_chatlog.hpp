#pragma once

#include <deque>

#include "gui_writebox.hpp"

class Chatlog : private Writebox
{
public:

	inline static const float width = 300.f;
	inline static const float height = 300.f;
	inline static const float font_size = 18.f;
	inline static const int max_visible_lines = 15;
	inline static const int max_remembered_lines = 512;

private:

	std::deque<std::string> texts;
	Rectangle chat_box;
	Rectangle chat_box_hitbox;

	Chatlog(const Chatlog&) = delete;
	Chatlog(Chatlog&&) = delete;

public:

	Chatlog(float arg_x, float arg_y, int arg_character_limit = 100);

	bool is_active() const;
	void handle_events(int key_pressed);
	void set_position(float x, float y);
	void draw() const;

};