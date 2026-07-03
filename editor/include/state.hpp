#pragma once

#include <iostream>
#include "../external/imgui-1.92.6/imgui.h"
#include "../external/imgui-1.92.6/backends/imgui_impl_sdl2.h"
#include "../external/imgui-1.92.6/backends/imgui_impl_opengl3.h"
#include "quaph.hpp"
#include "net.hpp"
#include <stdio.h>
#include <cmath>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

enum class AppState { // APP STATE
	EMPTY,
	IN_PROJECT
};

enum class UIState { // UI STATE
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

enum class ConnectStage { // where the user is in the connect -> project -> commit flow
	DISCONNECTED,
	CONNECTING, // net_connect() issued; on native this resolves ~immediately, on
	            // Emscripten the handshake is async so we wait for client.connected
	ENTERING_IDENTITY,
	BROWSING_PROJECTS,
	BROWSING_COMMITS,
	LOADING,
	IN_PROJECT
};

struct CommitInfo {
	std::string id;
	std::string message;
	std::string author_name;
	long long time_unix = 0;
};

#define HOST_BUF_SIZE 64
#define PORT_BUF_SIZE 8
#define IDENTITY_BUF_SIZE 128
#define PROJECT_ID_BUF_SIZE 64

struct RemoteState {
	NetClient client;
	ConnectStage stage = ConnectStage::DISCONNECTED;
	int pending_request_id = -1; // request id we're currently waiting on, or -1

	char host_buf[HOST_BUF_SIZE] = "127.0.0.1";
	char port_buf[PORT_BUF_SIZE] = "8765";
	char full_name_buf[IDENTITY_BUF_SIZE] = "";
	char email_buf[IDENTITY_BUF_SIZE] = "";
	char new_project_buf[PROJECT_ID_BUF_SIZE] = "";

	std::vector<std::string> project_list;
	std::vector<CommitInfo> commit_list;

	std::string current_project_id;
	std::string current_commit_id;
	std::string current_save_id;

	std::vector<nlohmann::json> pending_changes; // AddObj/AddRel/... Change JSON since the last save
	int next_temp_id = 0;

	std::string last_error;
};

struct StateInfo {
	std::string project_name;

	Objects objects;
	Relationships relationships;

	AppState app_state;
	UIState ui_state; // TODO user_state != graph state

	int object_id, object_id2;
	float left_split_width;
	ImVec2 cursor_pos, cursor_pivot;
	ImVec2 draw_line_from, draw_line_to;

	ImVec2 screen_dimensions;
	ImVec2 world_dimensions;
	ImVec2 view_offset;
	float x_scale_factor; // what percentage of world_x is view_x

	int selected_object_id;
	int selected_relationship_id;
	// graph_world_width, graph_world_height
	// graph_view_width, graph_view_height ! given by ImGui
	// graph_view_x, graph_view_y
	// zoom // (x axis scale factor)

	RemoteState remote;
};

int update_state_info(StateInfo& state_info);
int update_remote_state(StateInfo& state_info);
ImVec2 display_to_true(StateInfo& state_info, ImVec2 pos);
