#include "AvailableMatchesWindow.h"
#include "../ObserverPlugin.h"
#include "../ObserverMatchData.h" 

#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Constants/Constants.h>
#include <GWCA/Context/CharContext.h>
#include <GWCA/GameContainers/Array.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip> 

void AvailableMatchesWindow::Draw(ObserverPlugin& plugin, bool& is_visible)
{
    if (!is_visible) return;

    ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Observer Plugin - Available Matches", &is_visible)) 
    {
        if (ImGui::TreeNode("Display Options")) {
            ImGui::Columns(2, "ObsDisplayOptions", false);
            ImGui::Checkbox("Match IDs##Obs", &plugin.obs_show_match_ids);
            ImGui::Checkbox("Map ID##Obs", &plugin.obs_show_map_id);
            ImGui::Checkbox("Age##Obs", &plugin.obs_show_age);
            ImGui::Checkbox("Type##Obs", &plugin.obs_show_type);
            ImGui::Checkbox("Count##Obs", &plugin.obs_show_count);
            ImGui::Checkbox("Match Unknowns##Obs", &plugin.obs_show_match_unknowns);
            ImGui::Checkbox("Team 1 Name Dup##Obs", &plugin.obs_show_team1_name_dup);
            ImGui::Checkbox("H007C Array##Obs", &plugin.obs_show_h007c_array);
            ImGui::NextColumn();
            ImGui::Checkbox("Team ID##Obs", &plugin.obs_show_team_id);
            ImGui::Checkbox("Team Name##Obs", &plugin.obs_show_team_name);
            ImGui::Checkbox("Team Unknowns##Obs", &plugin.obs_show_team_unknowns);
            ImGui::Checkbox("Cape Colors##Obs", &plugin.obs_show_cape_colors);
            ImGui::Checkbox("Cape Design##Obs", &plugin.obs_show_cape_design);
            ImGui::Columns(1);
            ImGui::TreePop();
            ImGui::Separator();
        }

        if (GW::Map::GetInstanceType() == GW::Constants::InstanceType::Outpost) {
            GW::CharContext* cc = GW::GetCharContext();
            GW::Array<ObserverData::MatchDetails*>* observer_matchs_ptr = nullptr;
            if (cc) {
                uintptr_t char_context_base = reinterpret_cast<uintptr_t>(cc);
                uintptr_t observer_matchs_address = char_context_base + 0x24C; 
                observer_matchs_ptr = reinterpret_cast<GW::Array<ObserverData::MatchDetails*>*>(observer_matchs_address);
            }

            if (!observer_matchs_ptr || !observer_matchs_ptr->valid()) {
                ImGui::TextDisabled("Character context or observer match data not available.");
            } else {
                std::map<uint32_t, std::vector<ObserverData::MatchDetails*>> type_map;
                for (ObserverData::MatchDetails* om : *observer_matchs_ptr) {
                    if (!om) continue;
                    type_map[om->type].push_back(om);
                }

                if (type_map.empty()) {
                     ImGui::TextDisabled("No observer matches available.");
                }

                for (auto &m : type_map) {
                    std::string type_name = ObserverData::GetMatchTypeName(m.first);
                    if (ImGui::TreeNode(type_name.c_str())) {
                        for (ObserverData::MatchDetails* o : m.second) {
                            if (!o) continue;
                            if (plugin.obs_show_match_ids) ImGui::Text("Match ID: %u (Dup: %u)", o->match_id, o->match_id_dup); ImGui::SameLine();
                            if (plugin.obs_show_map_id) ImGui::Text("Map ID: %u", o->map_id); ImGui::SameLine();
                            if (plugin.obs_show_type) ImGui::Text("Type: %u (%s)", o->type, ObserverData::GetMatchTypeName(o->type).c_str()); ImGui::SameLine();
                            ImGui::NewLine();
                            if (plugin.obs_show_age) ImGui::Text("  Age: %ums", o->age); ImGui::SameLine();
                            if (plugin.obs_show_count) ImGui::Text("Count: %u", o->count); ImGui::SameLine();
                            if (plugin.obs_show_match_unknowns) ImGui::Text("Unknowns: h0014=0x%X, h0018=0x%X", o->h0014, o->h0018); ImGui::SameLine();
                            ImGui::NewLine();
                            if (plugin.obs_show_team1_name_dup) {
                                std::string team1_dup_str = ObserverData::SafeWcharToString(o->team1_name_dup);
                                ImGui::Text("  Team1 Name Dup: %s", team1_dup_str.c_str());
                            }
                            ObserverData::DrawObserverTeam(0, &o->team[0], plugin.obs_show_team_id, plugin.obs_show_team_name, plugin.obs_show_team_unknowns, plugin.obs_show_cape_colors, plugin.obs_show_cape_design);
                            ObserverData::DrawObserverTeam(1, &o->team[1], plugin.obs_show_team_id, plugin.obs_show_team_name, plugin.obs_show_team_unknowns, plugin.obs_show_cape_colors, plugin.obs_show_cape_design);
                            if (plugin.obs_show_h007c_array) {
                                ImGui::TextUnformatted("  h007C Array:");
                                ImGui::Indent();
                                std::stringstream ss_h007c;
                                for (int i = 0; i < 10; ++i) ss_h007c << "[ " << i << "]=0x" << std::hex << o->h007C[i] << (i == 4 ? "\n    " : " ");
                                ImGui::TextUnformatted(ss_h007c.str().c_str());
                                ImGui::Unindent();
                            }
                            ImGui::Separator();
                        }
                        ImGui::TreePop();
                    }
                }
            }
        } else {
            ImGui::TextDisabled("Only available in an outpost instance.");
        }
    }
    ImGui::End();
} 