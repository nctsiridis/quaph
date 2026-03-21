#pragma once

#include "../external/imgui-1.92.6/imgui.h"
#include "../external/imgui-1.92.6/backends/imgui_impl_sdl2.h"
#include "../external/imgui-1.92.6/backends/imgui_impl_opengl3.h"

#include "graphics.hpp"
#include "quaph.hpp"

#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

int draw_all(StateInfo& state_info, Objects& objects, Relationships& relationships, SDL_Window *window);
