#pragma once

#include <iostream>
#include "../external/imgui-1.92.6/imgui.h"
#include "../external/imgui-1.92.6/backends/imgui_impl_sdl2.h"
#include "../external/imgui-1.92.6/backends/imgui_impl_opengl3.h"
#include "quaph.hpp"
#include <stdio.h>
#include <cmath>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

enum UserState {
	VOID,
	GRAPH_HOVER_VOID,
	GRAPH_HOVER_OBJECT,
	GRAPH_HOVER_RELATIONSHIP,
	GRAPH_HOVER_NEAR_OBJECT,
	GRAPH_LEFT_DOWN_VOID,
	GRAPH_LEFT_DOWN_OBJECT,
	GRAPH_DRAGGING_OBJECT,
	GRAPH_LEFT_DOWN_RELATIONSHIP,
	GRAPH_DRAWING_LINE,
	GRAPH_DRAWING_LINE_CONNECTED,
	INFO_HOVER_VOID
};

struct DrawInfo {
	UserState user_state;
	int object_id;
	int object_id2;
	float total_width, left_width;
	ImVec2 cursor_pos;
	ImVec2 draw_line_from, draw_line_to;
};

int update_draw_info(DrawInfo& draw_info, Objects& objects, Relationships& relationship);
int start_frames();
int complete_render(SDL_Window *window);
int sdl_event_loop(bool& running);
int draw_graph_info(float total_width);
int draw_objects(Objects& objects, ImDrawList *right_draw_list);
int draw_graph_window(DrawInfo& draw_info, Objects& objects);
int draw_main_window(int& next_id, DrawInfo& draw_info, Objects& objects, Relationships& relationships);
