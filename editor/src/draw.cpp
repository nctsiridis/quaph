#include "../include/draw.hpp"
#include "../include/remote.hpp"
#include <cstdlib>

static void draw_connect_panel(StateInfo& state_info) {
	RemoteState& remote = state_info.remote;
	bool busy = remote.pending_request_id != -1;

	switch (remote.stage) {
		case ConnectStage::DISCONNECTED: {
			ImGui::Text("Connect to a quaph server");
			ImGui::InputText("Host", remote.host_buf, HOST_BUF_SIZE);
			ImGui::InputText("Port", remote.port_buf, PORT_BUF_SIZE, ImGuiInputTextFlags_CharsDecimal);
			if (ImGui::Button("Connect")) {
				int port = atoi(remote.port_buf);
				if (net_connect(remote.client, remote.host_buf, port) == 0) {
					remote.stage = ConnectStage::CONNECTING;
					remote.last_error.clear();
				} else {
					remote.last_error = "failed to connect";
				}
			}
			break;
		}
		case ConnectStage::CONNECTING: {
			ImGui::Text("Connecting...");
			break;
		}
		case ConnectStage::ENTERING_IDENTITY: {
			ImGui::Text("Who's editing?");
			ImGui::InputText("Full name", remote.full_name_buf, IDENTITY_BUF_SIZE);
			ImGui::InputText("Email", remote.email_buf, IDENTITY_BUF_SIZE);
			bool ready = remote.full_name_buf[0] != '\0' && remote.email_buf[0] != '\0';
			ImGui::BeginDisabled(!ready || busy);
			if (ImGui::Button("Continue")) {
				remote.pending_request_id = remote_search_projects(remote.client);
			}
			ImGui::EndDisabled();
			break;
		}
		case ConnectStage::BROWSING_PROJECTS: {
			ImGui::Text("Projects");
			for (auto& name : remote.project_list) {
				if (ImGui::Selectable(name.c_str())) {
					remote.current_project_id = name;
					remote.commit_list.clear();
					remote.pending_request_id = remote_search_commits(remote.client, name);
				}
			}
			ImGui::Separator();
			ImGui::InputText("New project", remote.new_project_buf, PROJECT_ID_BUF_SIZE);
			ImGui::BeginDisabled(remote.new_project_buf[0] == '\0' || busy);
			if (ImGui::Button("Create")) {
				remote.current_project_id = remote.new_project_buf;
				remote.pending_request_id = remote_create_project(
					remote.client, remote.current_project_id,
					remote.full_name_buf, remote.email_buf
				);
				remote.new_project_buf[0] = '\0';
			}
			ImGui::EndDisabled();
			break;
		}
		case ConnectStage::BROWSING_COMMITS: {
			if (ImGui::Button("< Back")) {
				remote.stage = ConnectStage::BROWSING_PROJECTS;
			}
			ImGui::Text("%s", remote.current_project_id.c_str());
			for (auto& commit : remote.commit_list) {
				std::string label = commit.id.substr(0, 7) + " - " + commit.message;
				if (ImGui::Selectable(label.c_str())) {
					remote.current_commit_id = commit.id;
					remote.stage = ConnectStage::LOADING;
					remote.pending_request_id = remote_fetch_commit(
						remote.client, remote.current_project_id, commit.id
					);
				}
			}
			break;
		}
		case ConnectStage::LOADING: {
			ImGui::Text("Loading...");
			break;
		}
		case ConnectStage::IN_PROJECT:
			break; // handled by the AppState::IN_PROJECT branch below
	}

	if (!remote.last_error.empty()) {
		ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", remote.last_error.c_str());
	}
}

static void draw_in_project_panel(StateInfo& state_info) {
	RemoteState& remote = state_info.remote;
	bool busy = remote.pending_request_id != -1;

	ImGui::Text("%s", state_info.project_name.c_str());
	ImGui::Text("%zu unsaved change(s)", remote.pending_changes.size());

	ImGui::BeginDisabled(remote.pending_changes.empty() || busy);
	if (ImGui::Button("Save & Commit")) {
		remote.pending_request_id = remote_create_save(
			remote.client,
			remote.current_project_id,
			remote.current_commit_id,
			remote.pending_changes
		);
	}
	ImGui::EndDisabled();

	if (!remote.last_error.empty()) {
		ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", remote.last_error.c_str());
	}
}

int draw_state_info_window(StateInfo& state_info) {

	ImGui::BeginChild(
		"left",
		ImVec2(state_info.left_split_width, 0),
		true
	);

	if (state_info.app_state == AppState::EMPTY) {
		draw_connect_panel(state_info);
	} else if (state_info.app_state == AppState::IN_PROJECT) {
		draw_in_project_panel(state_info);
	}

	ImGui::EndChild();
	ImGui::SameLine();

	ImGui::InvisibleButton("splitter", ImVec2(10, -1));
	if (ImGui::IsItemActive())
		state_info.left_split_width += ImGui::GetIO().MouseDelta.x;

	if (ImGui::IsItemHovered())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

	float min_width = 100.0f;
	float max_width = state_info.screen_dimensions.x - 100.0f;
	state_info.left_split_width = std::clamp(state_info.left_split_width, min_width, max_width);

	ImGui::SameLine();

	return 0;
}

int draw_objects(Objects& objects, ImDrawList *graph_draw_list) {
	for (int i = 0; i < objects.ids.size(); i++) {
		float radius = objects.display_radius;
		ImU32 color = IM_COL32(255, 0, 0, 255);
		ImVec2 center = ImVec2((float)objects.display_xs[i], (float)objects.display_ys[i]);

		graph_draw_list->AddCircle(center, radius, color, 0, 2.0f);
	}
	return 0;
}

ImVec2 get_nearest_point_on_object(ImVec2 pos, int object_id, Objects& objects) {
	int index = objects.id_to_index[object_id];
	float x = objects.display_xs[index], y = objects.display_ys[index];
	float dx = x - pos.x, dy = y - pos.y;
	float dist = sqrt(dx*dx + dy*dy);

	float vx = pos.x - x;
	float vy = pos.y - y;
	vx /= dist / objects.display_radius;
	vy /= dist / objects.display_radius;

	return ImVec2(x + vx, y + vy);
}

int draw_relationship_builder_hover(
	int object_id, Objects& objects, ImDrawList *graph_draw_list
) {
	ImVec2 mouse_pos = ImGui::GetMousePos();
	ImU32 color = IM_COL32(0, 255, 0, 255);
	graph_draw_list->AddCircleFilled(
		get_nearest_point_on_object(mouse_pos, object_id, objects),
		3, color, 0
	);
	return 0;
}

int draw_relationship_builder_down(
	int object_id, Objects& objects, ImDrawList *graph_draw_list
) {
	ImVec2 mouse_pos = ImGui::GetMousePos();
	ImU32 color = IM_COL32(0, 255, 0, 255);
	ImVec2 point = get_nearest_point_on_object(mouse_pos, object_id, objects);
	graph_draw_list->AddLine(
		point, mouse_pos,
		color, 2.0f
	);
	return 0;
}

std::pair<ImVec2, ImVec2> get_nearest_point_pair(int object_id, int object_id2, Objects& objects) {
	int i1 = objects.id_to_index[object_id];
	float x1 = objects.display_xs[i1], y1 = objects.display_ys[i1];
	ImVec2 p2 = get_nearest_point_on_object(
		ImVec2(x1, y1), 
		object_id2, objects
	);

	int i2 = objects.id_to_index[object_id2];
	float x2 = objects.display_xs[i2], y2 = objects.display_ys[i2];
	ImVec2 p1 = get_nearest_point_on_object(
		ImVec2(x2, y2), 
		object_id, objects
	);

	return {p1, p2};
}

int draw_relationship_builder_connected(
	int object_id, int object_id2, Objects& objects, ImDrawList *graph_draw_list
) {
	ImU32 color = IM_COL32(0, 255, 0, 255);

	std::pair<ImVec2, ImVec2> nearest_point_pair = get_nearest_point_pair(
		object_id, object_id2, objects
	);

	graph_draw_list->AddLine(
		nearest_point_pair.first, nearest_point_pair.second,
		color, 2.0f
	);
	return 0;
}

int draw_relationships(Relationships& relationships, Objects& objects, ImDrawList *graph_draw_list) {
	for (int i = 0; i < relationships.ids.size(); i++) {
		int object_id = relationships.from[i];
		int object_id2 = relationships.to[i];
		std::pair<ImVec2, ImVec2> nearest_point_pair = get_nearest_point_pair(
			object_id, object_id2, objects
		);

		ImU32 color = IM_COL32(0, 0, 255, 255);
		graph_draw_list->AddLine(
			nearest_point_pair.first, nearest_point_pair.second,
			color, 2.0f
		);
	}
	return 0;
}

int draw_graph_window(StateInfo& state_info, Objects& objects, Relationships& relationships) {

	ImGui::BeginChild("right", ImVec2(0, 0), true);

	//draw world border 

	// ImGui::Text("Right view");
	ImDrawList *graph_draw_list = ImGui::GetWindowDrawList();
	if (ImGui::BeginPopupContextWindow("context", ImGuiPopupFlags_MouseButtonRight)) {
		if (ImGui::MenuItem("New Object")) {
			ImVec2 click_pos = ImGui::GetMousePos();
			ImVec2 true_pos = display_to_true(state_info, click_pos);

			create_object(
				true_pos.x,
				true_pos.y,
				click_pos.x,
				click_pos.y,
				objects
			);

			if (state_info.app_state == AppState::IN_PROJECT) {
				std::string temp_uid = "t-" + std::to_string(state_info.remote.next_temp_id++);
				nlohmann::json custom_data = {{"x", true_pos.x}, {"y", true_pos.y}};
				objects.uids.back() = temp_uid;
				objects.custom_data.back() = custom_data;
				state_info.remote.pending_changes.push_back({
					{"type", "add-obj"},
					{"uid", temp_uid},
					{"custom_data", custom_data}
				});
			}
		}
		/*
		if (ImGui::BeginMenu("Advanced")) {
			if (ImGui::MenuItem("Do Something")) {}
			ImGui::EndMenu();
		}
		*/
		ImGui::EndPopup();
	}

	draw_objects(objects, graph_draw_list);
	draw_relationships(relationships, objects, graph_draw_list);

	if (state_info.ui_state == UIState::HOVER_NEAR_OBJECT) {
		draw_relationship_builder_hover(state_info.object_id, objects, graph_draw_list);
	} else if (state_info.ui_state == UIState::DRAWING_LINE) {
		draw_relationship_builder_down(state_info.object_id, objects, graph_draw_list);
	} else if (state_info.ui_state == UIState::DRAWING_LINE_CONNECTED) {
		draw_relationship_builder_connected(
			state_info.object_id, 
			state_info.object_id2, objects, 
			graph_draw_list
		);
	}
	return 0;
}

int draw_all(StateInfo& state_info, SDL_Window *window) {
	Objects& objects = state_info.objects;
	Relationships& relationships = state_info.relationships;

	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	ImGuiWindowFlags flags =
	ImGuiWindowFlags_NoTitleBar |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoBringToFrontOnFocus;
	
	ImGui::Begin("Main", NULL, flags);

	draw_state_info_window(state_info);
	draw_graph_window(state_info, objects, relationships);
	ImGui::PopStyleVar(2);
	ImGui::EndChild();
	ImGui::End();

	return 0;
}
