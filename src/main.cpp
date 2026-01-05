#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>
#include <stb/stb_image_write.h>

#include <sokol/sokol_app.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_gl.h>
#include <sokol/sokol_log.h>
#include <sokol/sokol_glue.h>
#include <sokol/sokol_imgui.h>

#include <imgui/imgui.h>

#include <nfde/nfd.h>

#include "app.cxx"

void init() {
	sg_setup(sg_desc{
		.logger = {.func = slog_func },
		.environment = sglue_environment(),
		});

	sgl_setup(sgl_desc_t{
		.logger = {.func = slog_func },
		});

	simgui_setup(simgui_desc_t{
		.ini_filename = nullptr,
		.logger = {.func = slog_func },
		});
	ImGui::GetIO().FontGlobalScale = 2.0f;

	NFD_Init();

	app_init();
}

void event(const sapp_event* event) {
	simgui_handle_event(event);
}

void frame() {
	simgui_new_frame({ sapp_width(), sapp_height(), sapp_frame_duration(), sapp_dpi_scale() });

	sg_begin_pass(sg_pass{
		.action = sg_pass_action{
			.colors = {{
					.load_action = SG_LOADACTION_CLEAR,
					.clear_value = { 0.25f, 0.25f, 0.25f, 1.0f },
				}},
			},
			.swapchain = sglue_swapchain()
		});

	app_frame();

	simgui_render();
	sg_end_pass();
	sg_commit();
}

void cleanup() {

	app_cleanup();

	NFD_Quit();
	simgui_shutdown();
	sgl_shutdown();
	sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
	return sapp_desc{
		.init_cb = init,
		.frame_cb = frame,
		.cleanup_cb = cleanup,
		.event_cb = event,
		.width = 1280,
		.height = 720,
		.sample_count = 1,
		.swap_interval = 1,
		.window_title = "quad_cropper",
		.enable_dragndrop = true,
		.icon = {.sokol_default = true, },
		.logger = {.func = slog_func},
#ifdef _DEBUG
		.win32_console_create = true,
#endif
	};
}
