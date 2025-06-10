#include "LordDamageWindow.h"
#include "../ObserverPlugin.h"
#include "../ObserverMatchData.h"
#include "../ObserverMatch.h"
#include <GWCA/Managers/MapMgr.h>
#include <imgui.h>
#include <algorithm>
#include <sstream>

void LordDamageWindow::Draw(ObserverPlugin& obs_plugin, bool& is_visible) {
    if (!is_visible) {
        return;
    }

    MatchInfo* match_info = nullptr;
    std::map<uint32_t, AgentInfo> agents_copy;
    std::map<uint16_t, GuildInfo> guilds_copy;
    
    if (obs_plugin.match_handler) {
        match_info = &obs_plugin.match_handler->GetMatchInfo();
        agents_copy = match_info->GetAgentsInfoCopy();
        guilds_copy = match_info->GetGuildsInfoCopy();
    }
    
    std::map<uint32_t, std::vector<AgentInfo>> teams;
    for (const auto& [agent_id, agent_info] : agents_copy) {
        if (agent_info.team_id != 0 &&
            (agent_info.type == AgentType::PLAYER || agent_info.type == AgentType::HERO || agent_info.type == AgentType::HENCHMAN)) {
            teams[agent_info.team_id].push_back(agent_info);
        }
    }
    
    bool can_display_teams = false;
    if (teams.size() >= 2) {
        int teams_with_players = 0;
        for (const auto& [team_id, team_agents] : teams) {
            bool has_player = false;
            for (const auto& agent : team_agents) {
                if (agent.type == AgentType::PLAYER) {
                    has_player = true;
                    break;
                }
            }
            if (has_player) {
                teams_with_players++;
            }
        }
        if (teams_with_players >= 2) {
            can_display_teams = true;
        }
    }

    ImGui::SetNextWindowSize(ImVec2(450.0f, 300.0f), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Lord Damage Counter##lord_damage_window", &is_visible, ImGuiWindowFlags_AlwaysAutoResize)) {
        
        ImGui::Separator();
        ImGui::Text("Guild Lord Damage Tracking");
        ImGui::Separator();
        ImGui::Spacing();
        
        if (!can_display_teams && obs_plugin.match_handler && !obs_plugin.match_handler->IsObserving()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Match ended - Showing final results");
            ImGui::Separator();
        } else if (!can_display_teams) {
            ImGui::TextDisabled("Waiting for match data or opponent information...");
            ImGui::End();
            return;
        }          if (ImGui::BeginTable("lord_damage_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            
            ImGui::TableSetupColumn("Guild", ImGuiTableColumnFlags_WidthFixed, 200.0f);            
            ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Damage", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();
            
            for (const auto& [team_id, team_agents] : teams) {
                if (team_id == 0) continue;
                
                long damage = ObserverMatchData::GetTeamLordDamage(team_id);
                
                std::string team_name = "Team " + std::to_string(team_id);
                std::string guild_tag = "";
                
                std::map<uint16_t, int> guild_counts;
                for(const auto& agent : team_agents) {
                    if (agent.type == AgentType::PLAYER && agent.guild_id != 0) {
                        guild_counts[agent.guild_id]++;
                    }
                }
                
                uint16_t most_common_guild_id = 0;
                int highest_count = 0;
                for (const auto& [guild_id, count] : guild_counts) {
                    if (count > highest_count) {
                        highest_count = count;
                        most_common_guild_id = guild_id;
                    }
                }
                
                if (most_common_guild_id != 0) {
                    auto it = guilds_copy.find(most_common_guild_id);
                    if (it != guilds_copy.end()) {
                        team_name = obs_plugin.WStringToString(it->second.name);
                        guild_tag = obs_plugin.WStringToString(it->second.tag);
                    }
                }
                
                DrawTeamDamageRow(team_name, guild_tag, damage, GetTeamColor(team_id));
            }
            
            ImGui::EndTable();
        }        
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Reset Damage##reset_lord_damage")) {
            ObserverMatchData::ResetLordDamage();
        }
        

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
    if (!guild_tag.empty()) {
        ImGui::Text("[%s]", guild_tag.c_str());
    } else {
        ImGui::Text("-");
    }
    
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%ld", damage);
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
        ImGui::BulletText("Only damage dealt to Guild Lords is counted");
        ImGui::BulletText("Enchantments and weapon spells on Guild Lord");
        ImGui::Text("   reduce team damage by 50 points");
        ImGui::BulletText("Life steal is counted as Lord damage");
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Note:");
        ImGui::Text("Displayed damage represents damage dealt");
        ImGui::Text("by the OPPOSING team to each team's Guild Lord.");
    }
}
