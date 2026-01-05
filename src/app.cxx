#include <string>
#include <string_view>

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

// This struct primarily contains the stuff that's dependent on the user's input.
//
// This means that we're pretty much only going to be using this to store
// the input/output images as well as any choices the user is allowed to make
struct AppState {
    bool inputIsLoaded;

    struct ImageInfo {
        sg_view view;
        int width, height, nChannels;
    } input, output;

    ImTextureRef outputImageRef;

} state;

sg_sampler nearestSampler;
sg_sampler linearSampler;
sgl_pipeline mainPipeline;

float viewScale;
float viewPosX, viewPosY;

std::string nfdError;

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
    viewPosX = 0.0f;
    viewPosY = 0.0f;

    memset(&state, 0, sizeof(AppState));
}

void app_frame() {
    //
    // imgui stuff
    //
    setup_mainmenu_bar();

    if (ImGui::Begin("Viewport")) {
        // TODO:
        // 1. we need the viewport to fit this image
        // 2. maybe we want a dummy checkerboard texture for the input?
        //    --> this could lead to not even neeeding to track if the input image is loaded with a bool since one would always be loaded
        if (state.inputIsLoaded) {
            ImGui::Image(state.outputImageRef, ImVec2(state.output.width, state.output.height));
        }
    }
    ImGui::End();

    define_modals();

    //
    // Draw actual editor
    //
    sgl_defaults();
    sgl_load_pipeline(mainPipeline);

    // camera
    const float screenWidth = sapp_widthf();
    const float screenHeight = sapp_heightf();
    const float halfScreenWidth = screenWidth / 2.0f;
    const float halfScreenHeight = screenHeight / 2.0f;

    sgl_matrix_mode_projection();
    sgl_ortho(-halfScreenWidth, halfScreenWidth, halfScreenHeight, -halfScreenHeight, 0.1, 1000.0);

    sgl_matrix_mode_modelview();
    sgl_scale(viewScale, viewScale, 1.0f);
    sgl_translate(-viewPosX, -viewPosY, 0.0f);
    // TODO: implement mouse controls for panning and zooming

    // TODO: generate test texture on startup so we can avoid checking if the texture is even loaded
    if (state.inputIsLoaded) {

    }

    sgl_draw();
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