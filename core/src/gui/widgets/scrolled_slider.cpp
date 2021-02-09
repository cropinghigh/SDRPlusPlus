#include <gui/widgets/scrolled_slider.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <gui/widgets/scroll_behavior.h>
#include <string>

namespace ImGui {
    bool SliderFloatWithScrolling(const char* label, float* v, float v_min, float v_max, float scroll_step, const char* display_format) {
        ImVec2 frame_size;
        frame_size.x = ImGui::CalcItemWidth();
        frame_size.y = ImGui::CalcTextSize(label, NULL, true).y + GImGui->Style.FramePadding.y*2.0f;
        ImGui::BeginChild((label + std::string("_slider")).c_str(), frame_size);
        ImGui::GetCurrentWindow()->ScrollMax.y = 1.0f;
        bool a = ImGui::SliderFloat(label, v, v_min, v_max, display_format) || ImGui::AllowScrollwheelStSz<float>(*v, scroll_step, v_min, v_max);
        ImGui::EndChild();
        return a;
    }
    
    bool VSliderFloatWithScrolling(const char* label, const ImVec2& size, float* v, float v_min, float v_max, float scroll_step, const char* display_format) {
        ImGui::BeginChild((label + std::string("_slider")).c_str(), size);
        ImGui::GetCurrentWindow()->ScrollMax.y = 1.0f;
        bool a = ImGui::VSliderFloat(label, size, v, v_min, v_max, display_format) || ImGui::AllowScrollwheelStSz<float>(*v, scroll_step, v_min, v_max);
        ImGui::EndChild();
        return a;
    }
    
    bool SliderIntWithScrolling(const char* label, int* v, int v_min, int v_max, int scroll_step, const char* display_format) {
        ImVec2 frame_size;
        frame_size.x = ImGui::CalcItemWidth();
        frame_size.y = ImGui::CalcTextSize(label, NULL, true).y + GImGui->Style.FramePadding.y*2.0f;
        ImGui::BeginChild((label + std::string("_slider")).c_str(), frame_size);
        ImGui::GetCurrentWindow()->ScrollMax.y = 1.0f;
        bool a = ImGui::SliderInt(label, v, v_min, v_max, display_format) || ImGui::AllowScrollwheelStSz<int>(*v, scroll_step, v_min, v_max);
        ImGui::EndChild();
        return a;
    }
    
    bool InputFloatWithScrolling(const char* label, float* v, float step, float step_fast, const char* format, ImGuiInputTextFlags flags) {
        ImVec2 frame_size;
        frame_size.x = ImGui::CalcItemWidth();
        frame_size.y = ImGui::GetFrameHeight();
        ImGui::BeginChild((label + std::string("_input")).c_str(), frame_size);
        ImGui::GetCurrentWindow()->ScrollMax.y = 1.0f;
        bool a = ImGui::InputFloat(label, v, step, step_fast, format, flags) || ImGui::AllowScrollwheelStSzNoLimit<float>(*v, (ImGui::GetIO().KeyCtrl && step_fast ? step_fast : step));
        ImGui::EndChild();
        return a;
    }
}
