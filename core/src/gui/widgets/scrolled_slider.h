#pragma once
#include <imgui.h>
#include <imgui_internal.h>

namespace ImGui {
    bool SliderFloatWithScrolling(const char* label, float* v, float v_min, float v_max, float scroll_step, const char* display_format = "%.3f");
    bool VSliderFloatWithScrolling(const char* label,  const ImVec2& size, float* v, float v_min, float v_max, float scroll_step, const char* display_format = "%.3f");
    bool SliderIntWithScrolling(const char* label, int* v, int v_min, int v_max, int scroll_step, const char* display_format = "%.3f");
    bool InputFloatWithScrolling(const char* label, float* v, float step, float step_fast, const char* format = "%.3f", ImGuiInputTextFlags flags = 0);
} 
