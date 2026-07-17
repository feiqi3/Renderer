#ifndef IMGUI_UTILS_H_
#define IMGUI_UTILS_H_

#include "function/InputDef.h"
#include "imgui.h"
namespace Render {
	ImTextureID toImTex(rs_image_view* view);
	ImGuiKey    toImKey(KeyCode code);
	ImGuiKey    toImKey(MouseButton btn);
}

#endif //!IMGUI_UTILS_H_