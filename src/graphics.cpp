#include "../include/graphics.hpp"

// STATE RELATED

int update_draw_info(DrawInfo& draw_info, Objects& objects, Relationships& relationship) {

	draw_info.total_width = ImGui::GetContentRegionAvail().x;

	UserState& state = draw_info.user_state;
	ImVec2 cursor_pos = ImGui::GetMousePos();

	if (state == UserState::VOID) {
		// not committed to either side of screen

		if (cursor_pos.x > draw_info.left_width) {
			state = UserState::GRAPH_HOVER_VOID;
			printf("switched to graph\n");
		} else if (cursor_pos.x < draw_info.left_width) {
			state = UserState::INFO_HOVER_VOID;
			printf("switched to info\n");
		}
	} else if (state == GRAPH_HOVER_VOID) {
		// hovering on nothing on graph side of screen

		if (cursor_pos.x <= draw_info.left_width) {
			state = UserState::VOID;
			printf("switched to void\n");
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			bool found = false;
			for (int i = 0; i < objects.ids.size(); i++) {
				float x = objects.xs[i], y = objects.ys[i];
				float dx = x - cursor_pos.x, dy = y - cursor_pos.y;
				float dist = sqrt(dx*dx + dy*dy);
				if (dist < 20.0) {
					draw_info.object_id = objects.ids[i];
					state = GRAPH_LEFT_DOWN_OBJECT;
					printf("switched to graph left down object\n");
					found = true;
					break;
				}
			}
			if (!found) {
				state = GRAPH_LEFT_DOWN_VOID;
				printf("switched to graph left down void\n");
			}
		} else {
			// nothing to do on graph hover void
		}
	} else if (state == UserState::INFO_HOVER_VOID) {
		std::cout << "posX: " << cursor_pos.x << std::endl;
		if (cursor_pos.x >= draw_info.left_width) {
			state = UserState::VOID;
			printf("switched to void\n");
		}
	} else if (state == UserState::GRAPH_LEFT_DOWN_VOID) {
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			state = UserState::VOID;
			printf("switched to void\n");
		}
	} else if (state == UserState::GRAPH_LEFT_DOWN_OBJECT) {
		// left mouse button down on graph object

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			state = UserState::VOID;
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

int draw_objects(Objects& objects, ImDrawList* graph_draw_list) {
	for (int i = 0; i < objects.ids.size(); i++) {
		float radius = 20.0f;
		ImU32 color = IM_COL32(255, 0, 0, 255);
		ImVec2 center = ImVec2((float)objects.xs[i], (float)objects.ys[i]);

		graph_draw_list->AddCircle(center, radius, color, 0, 2.0f);
	}
	return 0;
}

int draw_relationship_builder(Objects& objects, ImDrawList *graph_draw_list) {
	ImVec2 mouse_pos = ImGui::GetMousePos();

	float mn_prox = 50;
	float mn_dist = MAXFLOAT;
	int best_x = -1, best_y = -1;

	for (int i = 0; i < objects.ids.size(); i++) {
		int x = objects.xs[i], y = objects.ys[i];
		float dx = x - mouse_pos.x, dy = y - mouse_pos.y;
		float dist = std::sqrt((dx*dx + dy*dy));
		if (dist < mn_dist && dist < mn_prox) {
			mn_dist = dist;
			best_x = x;
			best_y = y;
		}
	}

	if (best_x >= 0 && best_y >= 0) {
		ImU32 color = IM_COL32(0, 255, 0, 255);

		float vx = mouse_pos.x - (float)best_x;
		float vy = mouse_pos.y - (float)best_y;
		vx /= mn_dist / 20;
		vy /= mn_dist / 20;
		
		graph_draw_list->AddCircleFilled(
			ImVec2((float)best_x + vx, (float)best_y + vy), 
			3, color, 0
		);
	}
	return 0;
}

int draw_graph_window(DrawInfo& draw_info, Objects& objects) {
	ImGui::BeginChild("right", ImVec2(0, 0), true);
	// ImGui::Text("Right view");
	ImDrawList *graph_draw_list = ImGui::GetWindowDrawList();
	if (ImGui::BeginPopupContextWindow("context", ImGuiPopupFlags_MouseButtonRight)) {
		if (ImGui::MenuItem("New Object")) {
			ImVec2 click_pos = ImGui::GetMousePos();
			create_object(
				objects.next_id++, 
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
	if (draw_info.user_state == UserState::GRAPH_HOVER_VOID)
		draw_relationship_builder(objects, graph_draw_list);
	// draw_relationships();
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
	draw_graph_window(draw_info, objects);
	ImGui::EndChild();
	ImGui::End();
	return 0;
}
