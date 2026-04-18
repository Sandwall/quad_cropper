// This file simply contains shared include headers between `app.cxx` and `main.cpp`
// Not really any reason for this other than C++ editor symbol indexing.

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

// C++ STL
#include <string>
#include <string_view>
#include <algorithm>