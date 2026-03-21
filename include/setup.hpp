#pragma once

#include <iostream>
#include <stdio.h>

#include "state.hpp"
#include "draw.hpp"

#ifdef __EMSCRIPTEN__
#include "../external/imgui-1.92.6/examples/libs/emscripten/emscripten_mainloop_stub.h"
#endif
// TODO remove (we aren't using this)

struct AppData {
	int next_id;
	SDL_Window *window;
	SDL_GLContext gl_context;
	bool running;
	Objects objects;
	Relationships relationships;
	StateInfo state_info;
};

// sdl2 + opengl3
int sdl2_gl3_close(AppData& app_data);
int sdl2_gl3_config(AppData& app_data);
int sdl_event_loop(bool& running);
int start_frames();
int complete_render(SDL_Window *window);
