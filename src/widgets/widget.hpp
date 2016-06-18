#pragma once

#include <SDL2/SDL_keycode.h>

struct input_event_t {
	SDL_Keycode key;
};

struct widget_t {
	widget_t *parent;
	widget_t(): parent(nullptr) {}
	widget_t(widget_t *_parent): parent(_parent) {}
	virtual ~widget_t() {}

	virtual void init() {}
	virtual void input(const input_event_t &) {};
	virtual void update(double dt) = 0;
	virtual void draw() = 0;
};
