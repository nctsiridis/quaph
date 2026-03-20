#include "../include/graphics.hpp"

// STATE RELATED

bool on_left_side(ImVec2 cursor_pos, DrawInfo& draw_info) {
	return cursor_pos.x < draw_info.left_width;
}

bool on_right_side(ImVec2 cursor_pos, DrawInfo& draw_info) {
	return cursor_pos.x > draw_info.left_width;
}

bool near_object(ImVec2 pos, int i, Objects& objects, float radius) {
	float x = objects.xs[i], y = objects.ys[i];
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
	int object_id, ImVec2 pos, DrawInfo& draw_info, 
	Objects& objects, float inner, float outer
) {
	int i = objects.id_to_index[object_id];
	float x = objects.xs[i], y = objects.ys[i];
	float dx = x - pos.x, dy = y - pos.y;
	float dist = sqrt(dx*dx + dy*dy);
	return dist < outer && dist > inner;
}

int update_draw_info(DrawInfo& draw_info, Objects& objects, Relationships& relationships) {
	draw_info.total_width = ImGui::GetContentRegionAvail().x;
	ImVec2 cursor_pos = ImGui::GetMousePos();

	if (draw_info.user_state == UserState::VOID) {
		// not committed to either side of screen

		if (on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::GRAPH_HOVER_VOID;
			printf("switched to graph\n");
		} else if (on_left_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::INFO_HOVER_VOID;
			printf("switched to info\n");
		}
	} else if (draw_info.user_state == GRAPH_HOVER_VOID) {
		// hovering on nothing on graph side of screen

		if (!on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if ((draw_info.object_id = near_any_object(
			cursor_pos, 
			objects, 
			objects.display_radius
		)) >= 0) {
			draw_info.user_state = UserState::GRAPH_HOVER_OBJECT;
			printf("switched to graph hover object\n");
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			draw_info.user_state = UserState::GRAPH_LEFT_DOWN_VOID;
			printf("switch to graph left down void\n");
		} else if ((draw_info.object_id = near_any_object(
			cursor_pos, 
			objects, 
			objects.display_radius + 20
		)) >= 0) {
			draw_info.user_state = UserState::GRAPH_HOVER_NEAR_OBJECT;
			printf("switched to hover near object\n");
		}
	} else if (draw_info.user_state == UserState::INFO_HOVER_VOID) {
		// hovering on info side of screen

		if (!on_left_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		}
	} else if (draw_info.user_state == UserState::GRAPH_HOVER_OBJECT) {
		// hovering over object on graph

		if (!on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (!near_object_by_id(
			cursor_pos, 
			draw_info.object_id, 
			objects, objects.display_radius
		)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			draw_info.user_state = UserState::GRAPH_LEFT_DOWN_OBJECT;
			printf("switched to graph left down object\n");
		}
	} else if (draw_info.user_state == UserState::GRAPH_HOVER_NEAR_OBJECT) {
		// graph hover near object

		if (!on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (!within_bounds_of_object_by_id(
			draw_info.object_id, cursor_pos, draw_info, objects,
			objects.display_radius, objects.display_radius + 20
		)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			draw_info.user_state = UserState::GRAPH_DRAWING_LINE;
			printf("switch to line drawin\n");
		}
	} else if (draw_info.user_state == UserState::GRAPH_DRAWING_LINE) {
		if (!on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			draw_info.user_state = UserState::VOID;
			printf("switch to void\n");
		} else if ((draw_info.object_id2 = near_any_object_excluding(
			cursor_pos, 
			draw_info.object_id,
			objects,
			objects.display_radius + 20
		)) >= 0) {
			draw_info.user_state = UserState::GRAPH_DRAWING_LINE_CONNECTED;
			printf("switched to drawing line connected\n");
		}
	} else if (draw_info.user_state == UserState::GRAPH_DRAWING_LINE_CONNECTED) {
		if (!on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (!near_object_by_id(
			cursor_pos, 
			draw_info.object_id2, 
			objects, 
			objects.display_radius + 20
		)) {
			draw_info.user_state = UserState::GRAPH_DRAWING_LINE;
			printf("switching to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			draw_info.user_state = UserState::VOID;
			create_relationship(draw_info.object_id, draw_info.object_id2, relationships);
			printf("switch to void and adding relationship\n");
		}
	} else if (draw_info.user_state == UserState::GRAPH_LEFT_DOWN_VOID) {
		// left mouse down on void on graph

		if (!on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		}
	} else if (draw_info.user_state == UserState::GRAPH_LEFT_DOWN_OBJECT) {
		// left mouse button down on graph object

		if (!on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void, graph item clicked!\n");
		} else if (!near_object_by_id(
			cursor_pos, 
			draw_info.object_id, 
			objects, 
			objects.display_radius
		)) {
			draw_info.user_state = UserState::GRAPH_DRAGGING_OBJECT;
			printf("switched to object dragging mode\n");
		}
	} else if (draw_info.user_state == UserState::GRAPH_DRAGGING_OBJECT) {
		if (!on_right_side(cursor_pos, draw_info)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			draw_info.user_state = UserState::VOID;
			printf("switched to void\n");
		} else {
			int index = objects.id_to_index[draw_info.object_id];
			objects.xs[index] = cursor_pos.x;
			objects.ys[index] = cursor_pos.y;
		}
	}
	return 0;
}

// GRAPHICS RELATED

int start_frames() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	return 0;
}

int complete_render(SDL_Window *window) {
	ImGui::Render();

	glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(window);
	return 0;
}

int sdl_event_loop(bool& running) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT) {
			running = false;
			return 0;
		} 
	}
	return 0;
}

int draw_info_window(float total_width, float left_width) {

	ImGui::BeginChild("left", ImVec2(left_width, 0), true);
	// ImGui::Text("Left view");
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::InvisibleButton("splitter", ImVec2(4, -1));
	if (ImGui::IsItemActive())
		left_width += ImGui::GetIO().MouseDelta.x;

	if (ImGui::IsItemHovered())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

	float min_width = 100.0f;
	float max_width = total_width - 100.0f;
	left_width = std::clamp(left_width, min_width, max_width);

	ImGui::SameLine();

	return 0;
}

int draw_objects(Objects& objects, ImDrawList *graph_draw_list) {
	for (int i = 0; i < objects.ids.size(); i++) {
		float radius = objects.display_radius;
		ImU32 color = IM_COL32(255, 0, 0, 255);
		ImVec2 center = ImVec2((float)objects.xs[i], (float)objects.ys[i]);

		graph_draw_list->AddCircle(center, radius, color, 0, 2.0f);
	}
	return 0;
}

ImVec2 get_nearest_point_on_object(ImVec2 pos, int object_id, Objects& objects) {
	int index = objects.id_to_index[object_id];
	float x = objects.xs[index], y = objects.ys[index];
	float dx = x - pos.x, dy = y - pos.y;
	float dist = sqrt(dx*dx + dy*dy);

	float vx = pos.x - x;
	float vy = pos.y - y;
	vx /= dist / 20;
	vy /= dist / 20;

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
	float x1 = objects.xs[i1], y1 = objects.ys[i1];
	ImVec2 p2 = get_nearest_point_on_object(
		ImVec2(x1, y1), 
		object_id2, objects
	);

	int i2 = objects.id_to_index[object_id2];
	float x2 = objects.xs[i2], y2 = objects.ys[i2];
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

int draw_graph_window(DrawInfo& draw_info, Objects& objects, Relationships& relationships) {
	ImGui::BeginChild("right", ImVec2(0, 0), true);
	// ImGui::Text("Right view");
	ImDrawList *graph_draw_list = ImGui::GetWindowDrawList();
	if (ImGui::BeginPopupContextWindow("context", ImGuiPopupFlags_MouseButtonRight)) {
		if (ImGui::MenuItem("New Object")) {
			ImVec2 click_pos = ImGui::GetMousePos();
			create_object(
				(int) click_pos.x, 
				(int) click_pos.y, 
				objects
			);
			printf("added object\n");
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

	if (draw_info.user_state == UserState::GRAPH_HOVER_NEAR_OBJECT) {
		draw_relationship_builder_hover(draw_info.object_id, objects, graph_draw_list);
	} else if (draw_info.user_state == UserState::GRAPH_DRAWING_LINE) {
		draw_relationship_builder_down(draw_info.object_id, objects, graph_draw_list);
	} else if (draw_info.user_state == UserState::GRAPH_DRAWING_LINE_CONNECTED) {
		draw_relationship_builder_connected(draw_info.object_id, draw_info.object_id2, objects, graph_draw_list);
	}
	return 0;
}

int draw_main_window(int& next_id, DrawInfo& draw_info, Objects& objects, Relationships& relationships) {
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);

	ImGuiWindowFlags flags =
	ImGuiWindowFlags_NoTitleBar |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoBringToFrontOnFocus;
	
	ImGui::Begin("Main", NULL, flags);

	draw_info_window(draw_info.total_width, draw_info.left_width);
	draw_graph_window(draw_info, objects, relationships);
	ImGui::EndChild();
	ImGui::End();
	return 0;
}
