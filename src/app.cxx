#include "includes.hpp"

#include "linalg.hpp"
#include "shaders.glsl.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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

template<typename T, size_t N>
struct Array {
	T data[N];
	operator T*() { return data; }
	T& operator[](size_t i) { return data[i]; }
};

// Wraps loading and reloading of images (just the main image being operated on)
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

	// loads an image from a filepath
	void load(std::string_view path) {
		release();

		// use stbi to load image
		{
			std::string pathStr(path);
			data = stbi_load(pathStr.c_str(), &width, &height, &nChannels, 4);
			nChannels = 4;
		}

		sg_image_data imageData;
		imageData.mip_levels[0].ptr = data;
		imageData.mip_levels[0].size = width * height * nChannels;

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
			}
		});
	}

	// loads an 8x8 white RGBA8 image
	static ImageInfo load_default() {
		ImageInfo info;
		sg_image_data imageData;

		info.width = 8;
		info.height = 8;
		info.nChannels = 4;

		size_t size = info.width * info.height * info.nChannels;
		void* ptr = malloc(size);
		imageData.mip_levels[0].ptr = ptr;
		imageData.mip_levels[0].size = size;
		memset(ptr, 255, size);

		info.image = sg_make_image(sg_image_desc{
			.type = SG_IMAGETYPE_2D,
			.usage = {
				.immutable = true
			},
			.width = info.width,
			.height = info.height,
			.num_slices = 1,
			.num_mipmaps = 1,
			.pixel_format = SG_PIXELFORMAT_RGBA8,
			.data = imageData
		});

		info.view = sg_make_view(sg_view_desc{
			.texture = {
				.image = info.image
			}
		});

		return info;
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
} loadedImage, defaultImage;

// doesn't do anything besides init/cleanup/hold sokol_gfx resources
struct SgRenderer {
	sg_shader mainShader;
	sg_pipeline mainPipeline;
	sg_sampler linearSampler;
	sg_buffer vertexBuffer;

	// NOTE: we'll bind the vertexBuffer and the linearSampler at the start
	sg_bindings bindings;

	struct Vertex {
		Vector2 position;
		Vector2 uv;
		Vector4 color;
	};

	union Quad {
		Vertex vertices[6];
		struct {
			Vertex tl;
			Vertex bl;
			Vertex br;
			Vertex tl2;
			Vertex br2;
			Vertex tr;
		};
	};

	Array<Quad, 4> quads;
	int numQuads;

	struct VertexParams {
		Matrix4 mvp;
	} vertexUniforms;

	struct FragmentParams {
		Matrix4 homography;

		Vector2 squareCentroid;
		float squareMagnitude;

		uint8_t pad1[4];

		Vector2 mutatedCentroid;
		float mutatedMagnitude;

		uint8_t pad2[4];
	} fragUniforms;

	static_assert(sizeof(VertexParams) == sizeof(VertexParams_t));
	static_assert(sizeof(FragmentParams) == sizeof(FragmentParams_t));

	void init() {
		linearSampler = sg_make_sampler(sg_sampler_desc{
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
			.wrap_v = SG_WRAP_CLAMP_TO_EDGE
		});

		mainShader = sg_make_shader(mainShd_shader_desc(sg_query_backend()));

		mainPipeline = sg_make_pipeline(sg_pipeline_desc{
			.compute = false,
			.shader = mainShader,
			.layout = {
				.attrs = {
					{ 0, 0, SG_VERTEXFORMAT_FLOAT2 },
					{ 0, 0, SG_VERTEXFORMAT_FLOAT2 },
					{ 0, 0, SG_VERTEXFORMAT_FLOAT4 },
				}
			},
			.depth = {
				.compare = SG_COMPAREFUNC_LESS_EQUAL,
				.write_enabled = true,
			},
			.primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
			.cull_mode = SG_CULLMODE_NONE, // i'm drawing like 5 quads, culling isn't important...
		});

		vertexBuffer = sg_make_buffer(sg_buffer_desc{
			.size = sizeof(quads),
			.usage = {
				.vertex_buffer = true,
				.index_buffer = false,
				.storage_buffer = false,
				.immutable = false,
				.dynamic_update = false,
				.stream_update = true,
			}
		});

		bindings = {
			.vertex_buffers = { vertexBuffer },
			.samplers = { linearSampler }
		};
	}

	void cleanup() {
		sg_destroy_buffer(vertexBuffer);
		sg_destroy_shader(mainShader);
		sg_destroy_pipeline(mainPipeline);
		sg_destroy_sampler(linearSampler);
		memset(this, 0, sizeof(SgRenderer));
	}

	void start_frame() {
		bindings.views[0] = defaultImage.view;
		numQuads = 0;
	}

	void add_line(float thickness, Vector2 p0, Vector2 p1, Vector4 col = { 1.0f, 1.0f, 1.0f, 1.0f }) {
		if(p0 == p1) return;

		thickness = fabsf(thickness / 2.0f); // using half thickness since we add it to p0 and p1
		Vector2 parallel = (p1 - p0).normalized();
		Vector2 perpendicular = rotate(parallel, static_cast<float>(M_PI) / 2.0f).normalized() * thickness;
		Quad& q = quads[numQuads++];

		for(int i = 0; i < 6; i++) {
			q.vertices[i].uv = { 0.0f, 0.0f };
			q.vertices[i].color = col;
		}

		q.tl.position = p0 + perpendicular;
		q.bl.position = p0 - perpendicular;
		q.br.position = p1 + perpendicular;
		q.tr.position = p1 - perpendicular;
		q.tl2 = q.tl;
		q.br2 = q.br;
	}

	void add_image(const ImageInfo& image) {
		const float width = static_cast<float>(loadedImage.width);
		const float height = static_cast<float>(loadedImage.height);
		const float halfWidth = width / 2.0f;
		const float halfHeight = height / 2.0f;
		Quad& q = quads[numQuads++];

		for(int i = 0; i < 6; i++) {
			q.vertices[i].color = { 1.0f, 1.0f, 1.0f, 0.0f };
		}

		q.tl.uv = { 0.0f, 0.0f };
		q.tl.position = { -halfWidth, -halfHeight };
		q.bl.uv = { 0.0f, 1.0f };
		q.bl.position = { -halfWidth, halfHeight };
		q.br.uv = { 1.0f, 1.0f };
		q.br.position = { halfWidth, halfHeight };
		q.tr.uv = { 1.0f, 0.0f };
		q.tr.position = { halfWidth, -halfHeight };
		q.tl2 = q.tl;
		q.br2 = q.br;

		bindings.views[0] = image.view;
	}

	void draw() {
		sg_update_buffer(vertexBuffer, { .ptr = quads, .size = sizeof(quads)});
		bindings.vertex_buffers[0] = vertexBuffer;

		sg_apply_pipeline(mainPipeline);
		sg_apply_bindings(bindings);
		sg_apply_uniforms(UB_VertexParams, { .ptr = &vertexUniforms, .size = sizeof(VertexParams)});
		sg_apply_uniforms(UB_FragmentParams, { .ptr = &fragUniforms, .size = sizeof(FragmentParams)});
		sg_draw(0, numQuads * 6, 1);
	}

} renderer;

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

	Array<Vector2, 4> get_vec2() {
		Array<Vector2, 4> av4;

		for(int i = 0; i < 4; i++) {
			av4[i] = { uvs[i].x, uvs[i].y };
		}

		return av4;
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
void draw_editor_with_sg(const ImGuiIO& io, float width, float height);

void app_init() {
	renderer.init();

	defaultImage = ImageInfo::load_default();

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
	draw_editor_with_sg(io, screenWidth, screenHeight);
}

void app_cleanup() {
	renderer.cleanup();
}

//
// INLINED FUNCTIONS - This is just done for the sake of cleanliness/organization...
//                     might change later...
//

static constexpr nfdu8filteritem_t IMAGE_FILTER_ITEMS = {
	.name = "Image Files",
	.spec = "png,jpg,jpeg,gif,pic,ppm,pgm,tga"
};

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

// we'll just reset this in the frame, it's used in
int numMatricesPrintedInFrame = 0;

template<int Rows, int Cols>
static void print_matrix_imgui(const Matrix<Rows, Cols>& matrix) {
	ImGui::PushID(numMatricesPrintedInFrame++);
	if (ImGui::BeginTable("MatrixTable", Cols)) {
		for(int row = 0; row < Rows; row++) {
			ImGui::TableNextRow();
			for(int col = 0; col < Cols; col++) {
				ImGui::TableSetColumnIndex(col);
				ImGui::Text("%.3f", matrix.get(row, col));
			}
		}
		ImGui::EndTable();
	}
	ImGui::PopID();
}

inline void build_imgui_controls(const ImGuiIO& io, float width, float height) {
	numMatricesPrintedInFrame = 0;

	if (loadedImage.is_loaded()) {
		// Controls Winodw
		ImGui::SetNextWindowSize(ImVec2(width / 2.0f, 2.0f * height / 3.0f), ImGuiCond_Appearing);
		if (ImGui::Begin("Controls")) {
			static constexpr float LONG_AXIS_PADDING = 32.0f;
			static constexpr float HANDLE_RADIUS = 16.0f;

			if(ImGui::Button("Reset to Default")) {
				quadUv.reset();
			}
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
			drawList->AddImage(simgui_imtextureid(loadedImage.view), rectTopLeft, rectBotRight);

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

		// Debug Info Window
		if(ImGui::Begin("Debug")) {
			ImGui::Text("Homography Matrix");
			print_matrix_imgui(renderer.fragUniforms.homography);
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
	} else if (deltaWheel < 0) {
		viewScale = std::clamp(viewScale / VIEWSCALE_FACTOR, 1.0f / MAX_VIEW_SCALE, MAX_VIEW_SCALE);
	}
}

inline void draw_editor_with_sg(const ImGuiIO& io, float width, float height) {
	renderer.start_frame();
	renderer.vertexUniforms.mvp =
		Matrix4::ortho(-width / 2.0f, width / 2.0f, height / 2.0f, -height / 2.0f, -100.0f, 100.0f)
		* Matrix4::scale(viewScale, viewScale, 1.0f)
		* Matrix4::translate(-viewPos.x, -viewPos.y, 0.0f);

	// draw cross at origin and texture
	renderer.add_line(1.0f, {0.0f, -height}, {0.0f, height});
	renderer.add_line(1.0f, {-width, 0.0f}, {width, 0.0f});

	if (loadedImage.data) {
		QuadUv squareQuadUv; // initialized to default square uvs
		Array<Vector2, 4> quadPoints = quadUv.get_vec2();
		Array<Vector2, 4> squareQuadPoints = squareQuadUv.get_vec2();

		// compute centroids/magnitudes for both input and output points
		Vector2& squareCentroid = renderer.fragUniforms.squareCentroid;
		Vector2& quadCentroid = renderer.fragUniforms.mutatedCentroid;
		float& squareMag = renderer.fragUniforms.squareMagnitude;
		float& quadMag = renderer.fragUniforms.mutatedMagnitude;

		squareCentroid.set_zero();
		quadCentroid.set_zero();
		for(int i = 0; i < 4; i++) {
			squareCentroid += squareQuadPoints[i];
			quadCentroid += quadPoints[i];
		}
		squareCentroid /= 4.0f;
		quadCentroid /= 4.0f;

		squareMag = 0.0f;
		quadMag = 0.0f;
		for (int i = 0; i < 4; i++) {
			squareMag += (squareQuadPoints[i] - squareCentroid).length();
			quadMag += (quadPoints[i] - quadCentroid).length();
		}
		squareMag /= 4.0f;
		quadMag /= 4.0f;

		// now use the centroids and average magnitudes to normalize each quad pair
		for(int i = 0; i < 4; i++) {
			quadPoints[i] = (quadPoints[i] - quadCentroid) / quadMag;
			squareQuadPoints[i] = (squareQuadPoints[i] - squareCentroid) / squareMag;
		}

		// now compute homography on normalized point pairs
		renderer.fragUniforms.homography = compute_homography(quadPoints, squareQuadPoints);

		// draw image using sg/renderer
		renderer.add_image(loadedImage);
	}

	// our matrices are stored in row-major order, and sokol_gfx expects column major order
	// so we need to transpose them before uploading them to the GPU
	renderer.vertexUniforms.mvp = renderer.vertexUniforms.mvp.transposed();
	renderer.fragUniforms.homography = renderer.fragUniforms.homography.transposed();

	renderer.draw();
}