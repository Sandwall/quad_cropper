#include "includes.hpp"

#include "linalg.hpp"
#include "shaders.glsl.h"

// This file is #included inside of main.cpp as part of a "jumbo" build,
// which means any code here has access to the same stuff that's #included at the top of main.cpp

/*

So the main functionality is:
- You can open an image with a main menu button
- You can save the output image with a main menu button

When the image is open:
- You can change the position of the 4 handles in the corners that determines
  how the output image is sampled

When the save menu is open:
- You can set the output resampling filter, as well as the output size.
- You can edit a path to save to. This should come in the form of a textbox
  that has the output path with a browse button next to it.
- Under all of that is an actual "Save" button that saves to the path, and a "Cancel" button

*/

static constexpr nfdu8filteritem_t IMAGE_FILTER_ITEMS = {
	.name = "Image Files",
	.spec = "png,jpg,jpeg,gif,pic,ppm,pgm,tga"
};


struct ImageInfo {
	sg_view view;
	sg_image image;

	// we'll make it so that this pointer being nullptr means that the image isn't loaded
	void* data;
	int width, height, nChannels;

	ImageInfo() {
		// being very lazy here lol
		memset(this, 0, sizeof(ImageInfo));
	}

	bool is_loaded() const { return nullptr != data; }

	void load(std::string_view path) {
		release();

		// use stbi to load image
		{
			std::string pathStr(path);
			data = stbi_load(pathStr.c_str(), &width, &height, &nChannels, 4);
			nChannels = 4;
		}

		sg_image_data imageData;
		imageData.subimage[0][0].ptr = data;
		imageData.subimage[0][0].size = width * height * nChannels;

		image = sg_make_image(sg_image_desc{
			.type = SG_IMAGETYPE_2D,
			.usage = {
				.immutable = true
			},
			.width = width,
			.height = height,
			.num_slices = 1,
			.num_mipmaps = 1,
			.pixel_format = SG_PIXELFORMAT_RGBA8,
			.data = imageData
			});


		view = sg_make_view(sg_view_desc{
			.texture = {
				.image = image
			} });
	}

	void release() {
		if (is_loaded()) {
			stbi_image_free(data);
			data = nullptr;

			width = 0;
			height = 0;
			nChannels = 0;

			sg_destroy_image(image);
			image = { 0 };

			sg_destroy_view(view);
			view = { 0 };
		}
	}
} loadedImage;

sg_sampler linearSampler;
sg_shader mainShader;
sgl_pipeline mainPipeline;

float viewScale;
ImVec2 viewPos;

union QuadUv {
	ImVec2 uvs[4];

	struct {
		ImVec2 tl, bl, br, tr;
	};

	QuadUv() { reset(); }

	void reset() {
		tl = { 0.0f, 0.0f };
		bl = { 0.0f, 1.0f };
		br = { 1.0f, 1.0f };
		tr = { 1.0f, 0.0f };
	}
} quadUv;

std::string nfdError;

float magnitude(const ImVec2& vec) {
	return sqrtf((vec.x * vec.x) + (vec.y * vec.y));
}

// forward declarations
void setup_mainmenu_bar();
void build_imgui_modals();
void build_imgui_export_modal();
void build_imgui_controls(const ImGuiIO& io, float width, float height);
void handle_mouse_controls(const ImGuiIO& io, float width, float height);
void draw_editor_with_sgl(const ImGuiIO& io, float width, float height);

void app_init() {
	linearSampler = sg_make_sampler(sg_sampler_desc{
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
		.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
		.wrap_v = SG_WRAP_CLAMP_TO_EDGE
		});

	mainShader = sg_make_shader(mainShd_shader_desc(sg_query_backend()));

	mainPipeline = sgl_make_pipeline(sg_pipeline_desc{
		.shader = mainShader,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},

		// i'm drawing like 5 quads with SGL, culling isn't important...
		.cull_mode = SG_CULLMODE_NONE,
		});

	viewScale = 1.0f;
	viewPos = { 0.0f, 0.0f };
}

void app_frame() {
	const float screenWidth = sapp_widthf();
	const float screenHeight = sapp_heightf();
	const ImGuiIO& io = ImGui::GetIO();

	setup_mainmenu_bar();

	// ImGui::ShowDemoWindow();

	build_imgui_controls(io, screenWidth, screenHeight);
	build_imgui_modals();

	handle_mouse_controls(io, screenWidth, screenHeight);
	draw_editor_with_sgl(io, screenWidth, screenHeight);
}

void app_cleanup() {
	sg_destroy_shader(mainShader);
	sgl_destroy_pipeline(mainPipeline);
	sg_destroy_sampler(linearSampler);
}

//
// INLINED FUNCTIONS - This is just done for the sake of cleanliness/organization...
//                     might change later...
//

inline void setup_mainmenu_bar() {
	ImGui::BeginMainMenuBar();

	if (ImGui::MenuItem("Open Image")) {
		nfdu8char_t* path = nullptr;
		nfdresult_t result = NFD_OpenDialogU8(&path, &IMAGE_FILTER_ITEMS, 1, nullptr);

		switch (result) {
		case NFD_OKAY: {
			std::string_view pathView(path);

			// TODO: actually load the image
			// maybe we'll want to reset the UV corners when we load a new image
			loadedImage.load(pathView);

			NFD_FreePathU8(path);
			path = nullptr;

		} break;
		case NFD_ERROR:
			ImGui::OpenPopup("NFD Error!");
			nfdError = NFD_GetError();
			break;
		}
	}

	if (ImGui::MenuItem("Save Output", nullptr, nullptr, loadedImage.is_loaded())) {
		ImGui::OpenPopup("Export Cropped Image");
	}

	build_imgui_export_modal();

	ImGui::EndMainMenuBar();
}

inline void build_imgui_modals() {
	// Define NFD Error popup modal
	if (ImGui::BeginPopupModal("NFD Error!")) {
		if (nfdError.length() > 0) {
			ImGui::Text("Error message: %s", nfdError.c_str());
		}

		ImGui::Dummy(ImVec2(0, 120));

		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	// Define STB image error popup modal
	if (ImGui::BeginPopupModal("STB Image Error!")) {
		ImGui::EndPopup();
	}
}

void build_imgui_export_modal() {
	if (ImGui::BeginPopupModal("Export Cropped Image")) {
		static int outputFilterChoice = 0;
		constexpr int numOutputFilters = 2;
		static const char* outputFilters[numOutputFilters] = {
			"Nearest Neighbor",
			"Bilinear"
		};
		static int outputResolution[2] = { 512, 512 };

		ImGui::ListBox("Output Filters", &outputFilterChoice, outputFilters, numOutputFilters);
		ImGui::DragInt2("Output Resolution", outputResolution, 0, 0);

		if (ImGui::Button("Save")) {
			nfdu8char_t* path = nullptr;
			nfdresult_t result = NFD_SaveDialogU8(&path, &IMAGE_FILTER_ITEMS, 1, nullptr, "image.png");

			switch (result) {
			case NFD_OKAY: {
				std::string_view pathView(path);

				// TODO: actually load the image
				// maybe we'll want to reset the UV corners when we load a new image
				loadedImage.load(pathView);

				NFD_FreePathU8(path);
				path = nullptr;

			} break;
			case NFD_ERROR:
				ImGui::OpenPopup("NFD Error!");
				nfdError = NFD_GetError();
				break;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

inline void build_imgui_controls(const ImGuiIO& io, float width, float height) {
	if (loadedImage.is_loaded()) {
		ImGui::SetNextWindowSize(ImVec2(width / 2.0f, 2.0f * height / 3.0f), ImGuiCond_Appearing);
		if (ImGui::Begin("Controls")) {
			static constexpr float LONG_AXIS_PADDING = 32.0f;
			static constexpr float HANDLE_RADIUS = 16.0f;

			ImGui::SliderFloat2("Top Left", &quadUv.tl.x, 0.0, 1.0);
			ImGui::SliderFloat2("Bottom Left", &quadUv.bl.x, 0.0, 1.0);
			ImGui::SliderFloat2("Bottom Right", &quadUv.br.x, 0.0, 1.0);
			ImGui::SliderFloat2("Top Right", &quadUv.tr.x, 0.0, 1.0);

			ImVec2 editorSize = ImGui::GetContentRegionAvail();
			ImVec2 editorTopLeft = ImGui::GetCursorScreenPos();
			ImVec2 editorBotRight = editorTopLeft + editorSize;

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(editorTopLeft, editorBotRight, IM_COL32(50, 50, 50, 255));
			drawList->AddRect(editorTopLeft, editorBotRight, IM_COL32(255, 255, 255, 255));
			drawList->PushClipRect(editorTopLeft, editorBotRight, true);
			ImGui::InvisibleButton("uv_editor_widget", editorSize, ImGuiButtonFlags_MouseButtonLeft);

			const float aspectRatio = static_cast<float>(loadedImage.width) / static_cast<float>(loadedImage.height);

			// Calculate centered rect with image's aspect ratio
			ImVec2 rectSize = ImVec2(editorSize.x - (2.0f * LONG_AXIS_PADDING), editorSize.x / aspectRatio);
			if (rectSize.y > editorSize.y) {
				rectSize.y = editorSize.y - (2.0f * LONG_AXIS_PADDING);
				rectSize.x = editorSize.y * aspectRatio;
			}

			ImVec2 rectTopLeft = editorTopLeft + ImVec2((editorSize.x - rectSize.x) / 2.0f, (editorSize.y - rectSize.y) / 2.0f);
			ImVec2 rectBotRight = rectTopLeft + rectSize;
			drawList->AddLine(ImVec2(rectTopLeft.x, editorTopLeft.y), ImVec2(rectTopLeft.x, editorBotRight.y), IM_COL32(100, 100, 100, 255), 1.0f);
			drawList->AddLine(ImVec2(rectBotRight.x, editorTopLeft.y), ImVec2(rectBotRight.x, editorBotRight.y), IM_COL32(100, 100, 100, 255), 1.0f);
			drawList->AddLine(ImVec2(editorTopLeft.x, rectTopLeft.y), ImVec2(editorBotRight.x, rectTopLeft.y), IM_COL32(100, 100, 100, 255), 1.0f);
			drawList->AddLine(ImVec2(editorTopLeft.x, rectBotRight.y), ImVec2(editorBotRight.x, rectBotRight.y), IM_COL32(100, 100, 100, 255), 1.0f);

			ImVec2 centers[4] = {
				rectTopLeft + (quadUv.tl * rectSize),
				rectTopLeft + (quadUv.bl * rectSize),
				rectTopLeft + (quadUv.br * rectSize),
				rectTopLeft + (quadUv.tr * rectSize),
			};
			drawList->AddPolyline(centers, 4, IM_COL32(150, 150, 150, 255), ImDrawFlags_Closed, 2.0f);

			const bool itemHovered = ImGui::IsItemHovered();
			const bool itemHeld = ImGui::IsItemActive();
			const ImVec2 mousePosUv = (io.MousePos - rectTopLeft) / rectSize;

			static int heldCorner = 0;
			if (!itemHeld) {
				heldCorner = 0;
			}

			for (int i = 0; i < 4; i++) {
				if ((0 == heldCorner) &&
					itemHeld &&
					(magnitude(io.MousePos - centers[i]) < HANDLE_RADIUS)) {
					heldCorner = i + 1;
				}
			}

			if (0 != heldCorner) {
				quadUv.uvs[heldCorner - 1] = mousePosUv;
			}

			for (int i = 0; i < 4; i++) {
				if ((i + 1) == heldCorner)
					drawList->AddCircleFilled(centers[i], HANDLE_RADIUS, IM_COL32(255, 255, 255, 255), 8);
				else
					drawList->AddCircle(centers[i], HANDLE_RADIUS, IM_COL32(255, 255, 255, 255), 8, 1.0f);
			}

			drawList->PopClipRect();
		} ImGui::End();
	}

}

inline void handle_mouse_controls(const ImGuiIO& io, float width, float height) {
	const ImVec2 mouseDelta = io.WantCaptureMouse ? ImVec2(0.0f, 0.0f) : io.MouseDelta;
	const float deltaWheel = io.WantCaptureMouse ? 0.0f : io.MouseWheel;
	const bool lmbDown = !io.WantCaptureMouse && io.MouseDown[0];
	static constexpr float VIEWSCALE_FACTOR = 1.5f;
	static constexpr float MAX_VIEW_SCALE = 4.0f;

	if (lmbDown) {
		viewPos -= mouseDelta / viewScale;
	}

	viewPos = {
		std::clamp(viewPos.x, -width * 2, width * 2),
		std::clamp(viewPos.y, -height * 2, height * 2)
	};

	if (deltaWheel > 0) {
		viewScale = std::clamp(viewScale * VIEWSCALE_FACTOR, 1.0f / MAX_VIEW_SCALE, MAX_VIEW_SCALE);
	}
	else if (deltaWheel < 0) {
		viewScale = std::clamp(viewScale / VIEWSCALE_FACTOR, 1.0f / MAX_VIEW_SCALE, MAX_VIEW_SCALE);
	}
}

inline void draw_editor_with_sgl(const ImGuiIO& io, float width, float height) {
	sgl_defaults();
	sgl_push_pipeline();
	sgl_load_pipeline(mainPipeline);

	// camera

	sgl_matrix_mode_projection();
	sgl_ortho(
		-width / 2.0f, width / 2.0f,
		height / 2.0f, -height / 2.0f,
		-100.0f, 100.0f
	);

	sgl_matrix_mode_modelview();
	sgl_scale(viewScale, viewScale, 1.0f);
	sgl_translate(-viewPos.x, -viewPos.y, 0.0f);

	// draw cross at origin and texture
	{
		sgl_c3f(1.0f, 1.0f, 1.0f);
		sgl_begin_lines();
		sgl_v2f(0.0f, -height); sgl_v2f(0.0f, height);
		sgl_v2f(-width, 0.0f);  sgl_v2f(width, 0.0f);
		sgl_end();

		if (loadedImage.data) {
			sgl_enable_texture();
			sgl_texture(loadedImage.view, linearSampler);
			sgl_begin_quads();

			const float width = static_cast<float>(loadedImage.width);
			const float height = static_cast<float>(loadedImage.height);
			const float halfWidth = width / 2.0f;
			const float halfHeight = height / 2.0f;

			// step 1: find the normal vector

			Vector3 points[4] = {
				Vector3(0.0f, 0.0f, 0.0f),       // tl
				Vector3(0.0f, height, 0.0f),     // bl
				Vector3(width, height, 0.0f),    // br
				Vector3(width, 0.0f, 0.0f)       // tr
			};

			for (int i = 0; i < 4; i++) {
				points[i].x = quadUv.uvs[i].x * width;
				points[i].y = quadUv.uvs[i].y * height;
				points[i] -= Vector3(halfWidth, halfHeight, 0.0f);
			}

			Vector3 normal;
			{
				Vector3 top = (points[0] + points[3]) / 2.0f;
				Vector3 bottom = (points[1] + points[2]) / 2.0f;
				Vector3 upVector = (bottom - top).normalized();

				Vector3 left = (points[0] + points[1]) / 2.0f;
				Vector3 right = (points[2] + points[3]) / 2.0f;
				Vector3 rightVector = (right - left).normalized();

				normal = Vector3::cross(rightVector, upVector);
				// printf("%f %f %f\n", rightVector.x, rightVector.y, rightVector.z);
				// printf("%f %f %f\n", upVector.x, upVector.y, upVector.z);
				// printf("%f %f %f\n", normal.x, normal.y, normal.z);
				// printf("\n");
			}

			for (int i = 0; i < 4; i++) {
				sgl_v3f_t2f(points[i].x, points[i].y, points[i].z, quadUv.uvs[i].x, quadUv.uvs[i].y);
			}

			sgl_end();
			sgl_disable_texture();
		}
	}

	sgl_pop_pipeline();
}