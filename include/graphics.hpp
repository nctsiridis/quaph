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

enum class GraphState { // GRAPH STATE
	NONE,
	HOVER_VOID,
	HOVER_OBJECT,
	HOVER_RELATIONSHIP,
	HOVER_NEAR_OBJECT,
	LEFT_DOWN_OBJECT,
	DRAGGING_OBJECT,
	DRAGGING_SCREEN,
	LEFT_DOWN_RELATIONSHIP,
	DRAWING_LINE,
	DRAWING_LINE_CONNECTED,
};

struct StateInfo {
	GraphState graph_state; // TODO user_state != graph state
	int object_id, object_id2;
	float left_split_width;
	ImVec2 cursor_pos, cursor_pivot;
	ImVec2 draw_line_from, draw_line_to;

	ImVec2 screen_dimensions;
	ImVec2 world_dimensions;
	ImVec2 view_offset;
	float x_scale_factor; // what percentage of world_x is view_x
	// graph_world_width, graph_world_height
	// graph_view_width, graph_view_height ! given by ImGui
	// graph_view_x, graph_view_y
	// zoom // (x axis scale factor)
}; 

int update_state_info(StateInfo& state_info, Objects& objects, Relationships& relationship);
ImVec2 display_to_true(StateInfo& state_info, ImVec2 pos);
