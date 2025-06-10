#include "LordDamageWindow.h"
#include "../ObserverPlugin.h"
#include "../ObserverMatchData.h"
#include <GWCA/Managers/MapMgr.h>
#include <imgui.h>
#include <algorithm>
#include <sstream>

void LordDamageWindow::Draw(ObserverPlugin& obs_plugin, bool& is_visible) {
    if (!is_visible) {
        return;
    }

    if (!GW::Map::GetIsObserving()) {
        is_visible = false;
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(450.0f, 300.0f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Lord Damage Counter##lord_damage_window", &is_visible, ImGuiWindowFlags_AlwaysAutoResize)) {
        
        ImGui::Separator();
        ImGui::Text("Guild Lord Damage Tracking");
        ImGui::Separator();
        ImGui::Spacing();        if (ImGui::BeginTable("lord_damage_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            
            ImGui::TableSetupColumn("Team", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableSetupColumn("Damage", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();
            
            for (uint32_t team_id = 1; team_id <= 2; ++team_id) {
                auto team_info = ObserverMatchData::GetTeamInfo(team_id);
                long damage = ObserverMatchData::GetTeamLordDamage(team_id);
                
                std::string team_name = obs_plugin.WStringToString(team_info.guild_name);
                if (team_name.empty()) {
                    team_name = "Team " + std::to_string(team_id);
                }
                  std::string guild_tag = obs_plugin.WStringToString(team_info.guild_tag);
                
                DrawTeamDamageRow(team_name, guild_tag, damage, GetTeamColor(team_id));
            }
            
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Reset Damage##reset_lord_damage")) {
            ObserverMatchData::ResetLordDamage();
        }
        
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Observer Mode Active");

        DrawInfoSection();
    }
    ImGui::End();
}

void LordDamageWindow::DrawTeamDamageRow(const std::string& team_name, const std::string& guild_tag, long damage, const ImColor& color) {
    ImGui::TableNextRow();
    
    ImGui::TableSetColumnIndex(0);
    ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)color);
    ImGui::Text("%s", team_name.c_str());
    ImGui::PopStyleColor();
    
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%ld", damage);
    
    ImGui::TableSetColumnIndex(2);
    if (!guild_tag.empty()) {
        ImGui::Text("[%s]", guild_tag.c_str());
    } else {
        ImGui::Text("-");
    }
}

ImColor LordDamageWindow::GetTeamColor(uint32_t team_id) const {
    switch (team_id) {
        case 1: return ImColor::HSV(0.67f, 0.7f, 0.9f);
        case 2: return ImColor::HSV(0.0f, 0.7f, 0.9f);
        default: return ImColor::HSV(0.33f, 0.7f, 0.9f);
    }
}

void LordDamageWindow::DrawInfoSection() {
    if (ImGui::CollapsingHeader("Information")) {
        ImGui::BulletText("This counter only works in observer mode");
        ImGui::BulletText("Only damage dealt to Guild Lords is counted");
        ImGui::BulletText("Enchantments and weapon spells on Guild Lord");
        ImGui::Text("   reduce team damage by 50 points");
        ImGui::BulletText("Life steal is counted as Lord damage");
        ImGui::BulletText("For GvG matches only");
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Note:");
        ImGui::Text("Displayed damage represents damage dealt");
        ImGui::Text("by the OPPOSING team to each team's Guild Lord.");
    }
}
