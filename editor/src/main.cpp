#include "../include/setup.hpp"
#include "../include/state.hpp"
#include "../include/draw.hpp"

void main_loop(void *arg) {
	AppData *app_data = (AppData*)arg;
	sdl_event_loop(app_data->running);
	start_frames();
	update_remote_state(app_data->state_info);
	update_state_info(app_data->state_info);
	draw_all(app_data->state_info, app_data->window);
	complete_render(app_data->window);
}

int main() {

	AppData app_data = default_app_data();

	if (sdl2_gl3_config(app_data)) return 1;

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(main_loop, &app_data, 0, true);
#else
	while (app_data.running) {
		main_loop(&app_data);
	}
#endif

	if (sdl2_gl3_close(app_data)) return 1;

	return 0;
}
