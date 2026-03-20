#include "../include/setup.hpp"

void main_loop(void *arg) {
	AppData *app_data = (AppData*)arg;
	sdl_event_loop(app_data->running);
	start_frames();
	update_draw_info(
		app_data->draw_info, 
		app_data->objects, 
		app_data->relationships
	);
	draw_main_window(
		app_data->next_id, 
		app_data->draw_info, 
		app_data->objects, 
		app_data->relationships
	);
	complete_render(app_data->window);
}

int main() {

	AppData app_data = {
		.window=NULL, 
		.gl_context=NULL, 
		.running=true, 
		.objects={.display_radius = 20.0, .next_id = 0}, 
		.relationships={.next_id = 0}, 
		.next_id=0,
		.draw_info = {.user_state = VOID, .left_width = 200.0f}
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
