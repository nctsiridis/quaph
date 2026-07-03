#include "../include/state.hpp"
#include "../include/remote.hpp"
#include <unordered_map>

ImVec2 display_to_true(StateInfo& state_info, ImVec2 pos) {
	float ratio = state_info.screen_dimensions.y / state_info.screen_dimensions.y;
	
	// size of screen in terms of world dimensions
	float view_x = state_info.x_scale_factor * state_info.world_dimensions.x;
	float view_y = ratio * view_x;

	float click_ratio_x = pos.x / state_info.screen_dimensions.x;
	float click_ratio_y = pos.y / state_info.screen_dimensions.y;

	float dx = view_x * click_ratio_x;
	float dy = view_y * click_ratio_y;

	float true_x = state_info.view_offset.x + dx;
	float true_y = state_info.view_offset.y + dy;

	return ImVec2(true_x, true_y);

}

ImVec2 true_to_display(StateInfo& state_info, ImVec2 pos) {
	float ratio = state_info.screen_dimensions.y / state_info.screen_dimensions.y;
	float view_x = state_info.x_scale_factor * state_info.world_dimensions.x;
	float view_y = ratio * view_x;

	float x = pos.x, y = pos.y;
	float x_delta = x - state_info.view_offset.x;
	float y_delta = y - state_info.view_offset.y;

	float x_ratio = x_delta / view_x;
	float y_ratio = y_delta / view_y;

	return ImVec2(
		x_ratio * state_info.screen_dimensions.x,
		y_ratio * state_info.screen_dimensions.y
	);
}

int update_display_properties(StateInfo& state_info, Objects& objects) {
	state_info.screen_dimensions = ImGui::GetIO().DisplaySize;

	float ratio = state_info.screen_dimensions.y / state_info.screen_dimensions.y;
	float view_x = state_info.x_scale_factor * state_info.world_dimensions.x;
	float view_y = ratio * view_x;

	float radius_percentage = objects.true_radius / view_x;
	objects.display_radius = radius_percentage * state_info.screen_dimensions.x;

	for (int i = 0; i < objects.ids.size(); i++) {
		float x = objects.true_xs[i], y = objects.true_ys[i];
		ImVec2 display_pos = true_to_display(state_info, ImVec2(x, y));
		objects.display_xs[i] = display_pos.x;
		objects.display_ys[i] = display_pos.y;
	}
	return 0;
}

bool on_graph_side(ImVec2 cursor_pos, StateInfo& state_info) {
	return cursor_pos.x > state_info.left_split_width;
}

bool near_object(ImVec2 pos, int i, Objects& objects, float radius) {
	float x = objects.display_xs[i], y = objects.display_ys[i];
	float dx = x - pos.x, dy = y - pos.y;
	float dist = sqrt(dx*dx + dy*dy);
	return dist < radius;
}

bool near_object_by_id(ImVec2 pos, int object_id, Objects& objects, float radius) {
	int i = objects.id_to_index[object_id];
	return near_object(pos, i, objects, radius);
}

int near_any_object(ImVec2 pos, Objects& objects, float radius) {
	for (int i = 0; i < objects.ids.size(); i++) {
		if (near_object(pos, i, objects, radius)) return objects.ids[i];
	}
	return -1;
}

int near_any_object_excluding(ImVec2 pos, int object_id, Objects& objects, float radius) {
	for (int i = 0; i < objects.ids.size(); i++) {
		if (objects.ids[i] != object_id && near_object(pos, i, objects, radius)) return objects.ids[i];
	}
	return -1;
}

bool within_bounds_of_object_by_id(
	int object_id, ImVec2 pos, StateInfo& state_info, 
	Objects& objects, float inner, float outer
) {
	int i = objects.id_to_index[object_id];
	float x = objects.display_xs[i], y = objects.display_ys[i];
	float dx = x - pos.x, dy = y - pos.y;
	float dist = sqrt(dx*dx + dy*dy);
	return dist < outer && dist > inner;
}

int update_state_info(StateInfo& state_info) {
	Objects& objects = state_info.objects;
	Relationships& relationships = state_info.relationships;

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 cursor_pos = ImGui::GetMousePos();

	update_display_properties(state_info, objects); // TODO decide where to call this

	if (state_info.ui_state == UIState::NONE) {
		// not committed to either side of screen

		if (on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::HOVER_VOID;
			printf("switched to graph\n");
		}
	} else if (state_info.ui_state == UIState::HOVER_VOID) {
		// hovering on nothing on graph side of screen

		if (!on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if ((state_info.object_id = near_any_object(
			cursor_pos, 
			objects, 
			objects.display_radius
		)) >= 0) {
			state_info.ui_state = UIState::HOVER_OBJECT;
			printf("switched to graph hover object\n");
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			state_info.cursor_pivot = display_to_true(state_info, cursor_pos);
			state_info.ui_state = UIState::DRAGGING_SCREEN;
			printf("switch to graph left down void\n");
		} else if ((state_info.object_id = near_any_object(
			cursor_pos, 
			objects, 
			objects.display_radius + 20
		)) >= 0) {
			state_info.ui_state = UIState::HOVER_NEAR_OBJECT;
			printf("switched to hover near object\n");
		}
	} else if (state_info.ui_state == UIState::HOVER_OBJECT) {
		// hovering over object on graph

		if (!on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (!near_object_by_id(
			cursor_pos, 
			state_info.object_id, 
			objects, objects.display_radius
		)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			state_info.ui_state = UIState::LEFT_DOWN_OBJECT;
			printf("switched to graph left down object\n");
		}
	} else if (state_info.ui_state == UIState::HOVER_NEAR_OBJECT) {
		// graph hover near object

		if (!on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (!within_bounds_of_object_by_id(
			state_info.object_id, cursor_pos, state_info, objects,
			objects.display_radius, objects.display_radius + 20
		)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			state_info.ui_state = UIState::DRAWING_LINE;
			printf("switch to line drawin\n");
		}
	} else if (state_info.ui_state == UIState::DRAWING_LINE) {
		if (!on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			state_info.ui_state = UIState::NONE;
			printf("switch to void\n");
		} else if ((state_info.object_id2 = near_any_object_excluding(
			cursor_pos, 
			state_info.object_id,
			objects,
			objects.display_radius + 20
		)) >= 0) {
			state_info.ui_state = UIState::DRAWING_LINE_CONNECTED;
			printf("switched to drawing line connected\n");
		}
	} else if (state_info.ui_state == UIState::DRAWING_LINE_CONNECTED) {
		if (!on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (!near_object_by_id(
			cursor_pos, 
			state_info.object_id2, 
			objects, 
			objects.display_radius + 20
		)) {
			state_info.ui_state = UIState::DRAWING_LINE;
			printf("switching to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			state_info.ui_state = UIState::NONE;
			create_relationship(state_info.object_id, state_info.object_id2, relationships);
			if (state_info.app_state == AppState::IN_PROJECT) {
				std::string from_uid = objects.uids[objects.id_to_index[state_info.object_id]];
				std::string to_uid = objects.uids[objects.id_to_index[state_info.object_id2]];
				std::string temp_uid = "t-" + std::to_string(state_info.remote.next_temp_id++);
				relationships.uids.back() = temp_uid;
				state_info.remote.pending_changes.push_back({
					{"type", "add-rel"},
					{"uid", temp_uid},
					{"from_uid", from_uid},
					{"to_uid", to_uid},
					{"custom_data", nlohmann::json::object()}
				});
			}
			printf("switch to void and adding relationship\n");
		}
	} else if (state_info.ui_state == UIState::LEFT_DOWN_OBJECT) {
		// left mouse button down on graph object

		if (!on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			state_info.ui_state = UIState::NONE;
			// state_info.selected_object_id = 
		} else if (!near_object_by_id(
			cursor_pos, 
			state_info.object_id, 
			objects, 
			objects.display_radius
		)) {
			state_info.ui_state = UIState::DRAGGING_OBJECT;
			printf("switched to object dragging mode\n");
		}
	} else if (state_info.ui_state == UIState::DRAGGING_OBJECT) {
		if (!on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else {
			int index = objects.id_to_index[state_info.object_id];
			objects.display_xs[index] = cursor_pos.x;
			objects.display_ys[index] = cursor_pos.y;
			ImVec2 true_pos = display_to_true(state_info, cursor_pos);
			objects.true_xs[index] = true_pos.x;
			objects.true_ys[index] = true_pos.y;
		}
	} else if (state_info.ui_state == UIState::DRAGGING_SCREEN) {
		if (!on_graph_side(cursor_pos, state_info)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			state_info.ui_state = UIState::NONE;
			printf("switched to void\n");
		} else {
			ImVec2 true_cursor = display_to_true(state_info, cursor_pos);
			state_info.view_offset.x -= true_cursor.x - state_info.cursor_pivot.x;
			state_info.view_offset.y -= true_cursor.y - state_info.cursor_pivot.y;
		}
	}

	if (io.MouseWheel != 0.0f) {
		// how to make sure we're zooming centered at the point
		// ensure that current mouse pos stays globally the same
		std::cout << "scrolling\n";
		
		ImVec2 true_cursor = display_to_true(state_info, cursor_pos);
		state_info.x_scale_factor -= io.MouseWheel / 80;
		ImVec2 true_cursor2 = display_to_true(state_info, cursor_pos);

		state_info.view_offset.x -= true_cursor2.x - true_cursor.x;
		state_info.view_offset.y -= true_cursor2.y - true_cursor.y;
	}

	return 0;
}

static void apply_fetch_commit_result(StateInfo& state_info, const nlohmann::json& result) {
	state_info.objects = default_objects();
	state_info.relationships = default_relationships();

	std::unordered_map<std::string, int> uid_to_local_id;

	for (auto& [uid, obj_json] : result["objs"].items()) {
		nlohmann::json custom_data = obj_json.value("custom_data", nlohmann::json::object());
		float x = custom_data.value("x", 0.0f);
		float y = custom_data.value("y", 0.0f);
		create_object(x, y, x, y, state_info.objects);
		state_info.objects.uids.back() = uid;
		state_info.objects.custom_data.back() = custom_data;
		uid_to_local_id[uid] = state_info.objects.ids.back();
	}

	for (auto& [uid, rel_json] : result["rels"].items()) {
		std::string from_uid = rel_json.value("from_uid", "");
		std::string to_uid = rel_json.value("to_uid", "");
		auto from_it = uid_to_local_id.find(from_uid);
		auto to_it = uid_to_local_id.find(to_uid);
		if (from_it == uid_to_local_id.end() || to_it == uid_to_local_id.end()) {
			std::cerr << "apply_fetch_commit_result: relationship " << uid
			          << " references unknown endpoint uid, skipping\n";
			continue;
		}
		create_relationship(from_it->second, to_it->second, state_info.relationships);
		state_info.relationships.uids.back() = uid;
		state_info.relationships.custom_data.back() = rel_json.value("custom_data", nlohmann::json::object());
	}

	state_info.remote.pending_changes.clear();
	state_info.remote.next_temp_id = 0;
	state_info.project_name = state_info.remote.current_project_id;
	state_info.app_state = AppState::IN_PROJECT;
	state_info.remote.stage = ConnectStage::IN_PROJECT;
}

static void apply_create_commit_result(StateInfo& state_info, const nlohmann::json& result) {
	std::unordered_map<std::string, std::string> temp_to_real;
	for (auto& [temp_uid, real_uid] : result["temp_to_real_map"].items()) {
		temp_to_real[temp_uid] = real_uid.get<std::string>();
	}

	for (auto& uid : state_info.objects.uids) {
		auto it = temp_to_real.find(uid);
		if (it != temp_to_real.end()) uid = it->second;
	}
	for (auto& uid : state_info.relationships.uids) {
		auto it = temp_to_real.find(uid);
		if (it != temp_to_real.end()) uid = it->second;
	}

	state_info.remote.current_commit_id = result.value("new_commit_id", "");
	state_info.remote.current_save_id.clear();
	state_info.remote.pending_changes.clear();
}

int update_remote_state(StateInfo& state_info) {
	RemoteState& remote = state_info.remote;
	net_poll(remote.client);

	if (remote.stage == ConnectStage::CONNECTING) {
		if (remote.client.connected) {
			remote.stage = ConnectStage::ENTERING_IDENTITY;
		}
		// no timeout for MVP: a connection that never opens leaves the user on
		// the "Connecting..." screen indefinitely (acceptable for a first pass).
		return 0;
	}

	nlohmann::json msg;
	while (net_pop_message(remote.client, msg)) {
		if (!msg.contains("id") || msg["id"].get<int>() != remote.pending_request_id) {
			continue; // stale or unsolicited response, drop it
		}

		std::string op = msg.value("op", "");
		bool ok = msg.value("ok", false);

		if (!ok) {
			remote.last_error = msg["error"].value("message", "unknown error");
			if (remote.stage == ConnectStage::LOADING) {
				remote.stage = ConnectStage::BROWSING_COMMITS;
			}
			remote.pending_request_id = -1;
			continue;
		}

		const nlohmann::json& result = msg["result"];

		if (op == "search_projects") {
			remote.project_list.clear();
			for (auto& p : result["projects"]) remote.project_list.push_back(p.get<std::string>());
			remote.stage = ConnectStage::BROWSING_PROJECTS;
		} else if (op == "create_project") {
			remote.commit_list.clear();
			remote.stage = ConnectStage::BROWSING_COMMITS;
			remote.pending_request_id = remote_search_commits(remote.client, remote.current_project_id);
			continue; // chained follow-up request already in flight
		} else if (op == "search_commits") {
			remote.commit_list.clear();
			for (auto& c : result["commits"]) {
				CommitInfo info;
				info.id = c.value("id", "");
				info.message = c.value("message", "");
				info.author_name = c.value("author_name", "");
				info.time_unix = c.value("time", 0LL);
				remote.commit_list.push_back(info);
			}
			remote.stage = ConnectStage::BROWSING_COMMITS;
		} else if (op == "fetch_commit") {
			apply_fetch_commit_result(state_info, result);
		} else if (op == "create_save") {
			remote.current_save_id = result.value("save_id", "");
			remote.pending_request_id = remote_create_commit(
				remote.client, remote.current_project_id, remote.current_commit_id,
				remote.current_save_id, remote.full_name_buf, remote.email_buf
			);
			continue; // chained follow-up request already in flight ("Save & Commit")
		} else if (op == "create_commit") {
			apply_create_commit_result(state_info, result);
		}

		remote.pending_request_id = -1;
	}

	return 0;
}
