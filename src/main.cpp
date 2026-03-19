#include "../include/setup.hpp"
#include "../include/quaph.hpp"

void main_loop(void *arg) {
	{
		AppData *app_data = (AppData*)arg;
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT) {
				app_data->running = false;
			} 
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		ImDrawList* draw_list = ImGui::GetForegroundDrawList(); 

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			ImVec2 click_pos = ImGui::GetMousePos();
			create_object(
				app_data->next_id++, 
				(int) click_pos.x, 
				(int) click_pos.y, 
				app_data->objects
			);
			printf("added object\n");
		}

		for (int i = 0; i < app_data->objects.ids.size(); i++) {
				ImU32 color = IM_COL32(255, 0, 0, 255);
				int x = app_data->objects.xs[i];
				int y = app_data->objects.ys[i];
				ImVec2 center = ImVec2((float) x, (float) y);
				draw_list->AddCircle(
					center,
					20, 
					color, 
					0, 
					2.0f
				);
		}

		ImGui::Render();

		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(app_data->window);
	}
}

int main() {

	AppData app_data = {
		.window=NULL, 
		.gl_context=NULL, 
		.running=true, 
		.objects={}, 
		.relationships={}, 
		.next_id=0,
	};
	SDL_Window *window;
	SDL_GLContext gl_context;
	if (sdl2_gl3_config(app_data)) return 1;

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(main_loop, &app_data, 0, true);
#else
	while (app_data.running) {main_loop(&app_data);}
#endif

	if (sdl2_gl3_close(app_data)) return 1;

	return 0;
}
