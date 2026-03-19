#include <iostream>
#include "../external/imgui-1.92.6/imgui.h"
#include "../external/imgui-1.92.6/backends/imgui_impl_sdl2.h"
#include "../external/imgui-1.92.6/backends/imgui_impl_opengl3.h"
#include "quaph.hpp"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#ifdef __EMSCRIPTEN__
#include "../external/imgui-1.92.6/examples/libs/emscripten/emscripten_mainloop_stub.h"
#endif

#pragma once

struct AppData {
	SDL_Window *window;
	SDL_GLContext gl_context;
	bool running;
	Objects objects;
	Relationships relationships;
	int next_id;
};

// sdl2 + opengl3
int sdl2_gl3_close(AppData& app_data);
int sdl2_gl3_config(AppData& app_data);
