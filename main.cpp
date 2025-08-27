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

void init() {
	sg_setup(sg_desc{
		.logger = {.func = slog_func },
		.environment = sglue_environment(),
		});

	sgl_setup(sgl_desc_t{
		.logger = {.func = slog_func },
		});

	simgui_setup(simgui_desc_t{
		.logger = {.func = slog_func },
		});
}

void event(const sapp_event* event) {
	\
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

	ImGui::ShowDemoWindow();

	simgui_render();
	sg_end_pass();
	sg_commit();
}

void cleanup() {
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
		.win32_console_create = true,
	};
}
