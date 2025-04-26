#include "MatchCompositionsSettingsWindow.h"
#include "../ObserverPlugin.h" 

#include <imgui.h>

void MatchCompositionsSettingsWindow::Draw(ObserverPlugin& /*obs_plugin*/, bool& is_visible) {
    if (!is_visible) return;

    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Match Compositions Settings", &is_visible)) {
        ImGui::TextDisabled("No settings yet.");
    }
    ImGui::End();
} 