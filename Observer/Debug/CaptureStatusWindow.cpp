#include "CaptureStatusWindow.h"
#include "../ObserverPlugin.h"

void CaptureStatusWindow::Draw(ObserverPlugin& plugin, bool& is_visible)
{
    if (!is_visible) return;

    ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Observer Plugin - Capture Status", &is_visible))
    {
        ImGui::Indent();
        bool stoc_capturing = plugin.match_handler && plugin.match_handler->IsObserving();
        ImGui::Text("StoC Events Capture:"); ImGui::SameLine();
        if (stoc_capturing) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
        } else {
            ImGui::TextDisabled("Inactive");
        }
         if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Indicates if StoC events are being captured internally.\n(Triggered by entering observer mode)");
        }

        bool loop_running = plugin.loop_handler && plugin.loop_handler->IsRunning();
        ImGui::Text("Agents States Capture:"); ImGui::SameLine();
        if (loop_running) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Running");
        } else {
            ImGui::TextDisabled("Stopped");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Indicates if the Agents Loop thread is currently capturing agents states snapshots.\n(Triggered by entering observer mode)");
        }
        ImGui::Unindent();
    }
    ImGui::End();
} 