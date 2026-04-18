#include "includes.hpp"

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

	void load(std::string_view path) {
		if (data) {
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
} loadedImage;

sg_sampler nearestSampler;
sg_sampler linearSampler;
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
void define_modals();

void app_init() {
	nearestSampler = sg_make_sampler(sg_sampler_desc{
		.min_filter = SG_FILTER_NEAREST,
		.mag_filter = SG_FILTER_NEAREST,
		.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
		.wrap_v = SG_WRAP_CLAMP_TO_EDGE
		});

	linearSampler = sg_make_sampler(sg_sampler_desc{
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
		.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
		.wrap_v = SG_WRAP_CLAMP_TO_EDGE
		});

	mainPipeline = sgl_make_pipeline(sg_pipeline_desc{
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

	//
	// imgui stuff
	//
	setup_mainmenu_bar();
	define_modals();

	// ImGui::ShowDemoWindow();

	ImGui::SetNextWindowSize(ImVec2(screenWidth / 2.0f, 2.0f * screenHeight / 3.0f), ImGuiCond_Appearing);
	if (loadedImage.data) {
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

	// mouse control for panning and zooming
	{
		const ImVec2 mouseDelta = io.WantCaptureMouse ? ImVec2(0.0f, 0.0f) : io.MouseDelta;
		const float deltaWheel = io.WantCaptureMouse ? 0.0f : io.MouseWheel;
		const bool lmbDown = !io.WantCaptureMouse && io.MouseDown[0];
		static constexpr float VIEWSCALE_FACTOR = 1.5f;
		static constexpr float MAX_VIEW_SCALE = 4.0f;

		if (lmbDown) {
			viewPos -= mouseDelta / viewScale;
		}

		viewPos = {
			std::clamp(viewPos.x, -screenWidth * 2, screenWidth * 2),
			std::clamp(viewPos.y, -screenHeight * 2, screenHeight * 2)
		};

		if (deltaWheel > 0) {
			viewScale = std::clamp(viewScale * VIEWSCALE_FACTOR, 1.0f / MAX_VIEW_SCALE, MAX_VIEW_SCALE);
		}
		else if (deltaWheel < 0) {
			viewScale = std::clamp(viewScale / VIEWSCALE_FACTOR, 1.0f / MAX_VIEW_SCALE, MAX_VIEW_SCALE);
		}

		printf("%f\n", deltaWheel);
	}

	//
	// Draw actual editor
	//

	sgl_defaults();
	sgl_push_pipeline();
	sgl_load_pipeline(mainPipeline);

	// camera

	sgl_matrix_mode_projection();
	sgl_ortho(
		-screenWidth / 2.0f,
		screenWidth / 2.0f,
		screenHeight / 2.0f,
		-screenHeight / 2.0f,
		-100.0f, 100.0f
	);

	sgl_matrix_mode_modelview();
	sgl_scale(viewScale, viewScale, 1.0f);
	sgl_translate(-viewPos.x, -viewPos.y, 0.0f);

	// draw cross at origin and texture
	{
		sgl_c3f(1.0f, 1.0f, 1.0f);
		sgl_begin_lines();
		sgl_v2f(0.0f, -screenHeight); sgl_v2f(0.0f, screenHeight);
		sgl_v2f(-screenWidth, 0.0f);  sgl_v2f(screenWidth, 0.0f);
		sgl_end();

		sgl_begin_quads();
		sgl_end();

		if (loadedImage.data) {
			sgl_enable_texture();
			sgl_texture(loadedImage.view, linearSampler);
			sgl_begin_quads();

			const float halfWidth = static_cast<float>(loadedImage.width) / 2.0f;
			const float halfHeight = static_cast<float>(loadedImage.height) / 2.0f;
			sgl_v2f_t2f(-halfWidth, -halfHeight, quadUv.tl.x, quadUv.tl.y);
			sgl_v2f_t2f(-halfWidth, halfHeight, quadUv.bl.x, quadUv.bl.y);
			sgl_v2f_t2f(halfWidth, halfHeight, quadUv.br.x, quadUv.br.y);
			sgl_v2f_t2f(halfWidth, -halfHeight, quadUv.tr.x, quadUv.tr.y);
			sgl_end();
			sgl_disable_texture();
		}
	}

	sgl_pop_pipeline();
}

void app_cleanup() {
	// TODO: implement this function after the main functionality is here...
}

//
// INLINED FUNCTIONS - This is just done for the sake of cleanliness/organization...
//                     might change later...
//

inline void setup_mainmenu_bar() {
	ImGui::BeginMainMenuBar();

	if (ImGui::MenuItem("Open Image")) {
		constexpr nfdu8filteritem_t filterItem = {
			.name = "Image Files",
			.spec = "png,jpg,jpeg,gif,pic,ppm,pgm,tga"
		};

		nfdu8char_t* path = nullptr;
		nfdresult_t result = NFD_OpenDialogU8(&path, &filterItem, 1, nullptr);

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

	if (ImGui::MenuItem("Save Output")) {
		ImGui::OpenPopup("Export Cropped Image");
	}

	ImGui::EndMainMenuBar();
}

inline void define_modals() {
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

	if (ImGui::BeginPopupModal("Export Cropped Image")) {
		static int outputFitlerChoice = 0;
		constexpr int numOutputFilters = 2;
		static const char* outputFilters[numOutputFilters] = {
			"Nearest Neighbor",
			"Bilinear"
		};
		static int outputResolution[2] = { 512, 512 };

		ImGui::ListBox("Output Filters", &outputFitlerChoice, outputFilters, numOutputFilters);
		ImGui::DragInt2("Output Resolution", outputResolution, 0, 0);

		if (ImGui::Button("Save")) {
			// TODO: actual saving logic, might want to move this into a function

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