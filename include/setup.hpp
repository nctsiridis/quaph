#pragma once

#include <iostream>
#include <stdio.h>

#include "graphics.hpp"

#ifdef __EMSCRIPTEN__
#include "../external/imgui-1.92.6/examples/libs/emscripten/emscripten_mainloop_stub.h"
#endif

struct AppData {
	int next_id;
	SDL_Window *window;
	SDL_GLContext gl_context;
	bool running;
	Objects objects;
	Relationships relationships;
	DrawInfo draw_info;
};

// sdl2 + opengl3
int sdl2_gl3_close(AppData& app_data);
int sdl2_gl3_config(AppData& app_data);
