#include "LivePartyInfoWindow.h"
#include "../ObserverPlugin.h"
#include "../ObserverMatchData.h" 

#include <vector>
#include <string>
#include <sstream>
#include <algorithm> 
#include <mutex> 

void LivePartyInfoWindow::Draw(ObserverPlugin& plugin, bool& is_visible)
{
    if (!is_visible) return;

    ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Observer Plugin - Live Party Info", &is_visible))
    {
        ImGui::Indent();
        if (plugin.match_handler) {
            std::lock_guard<std::mutex> lock(plugin.match_handler->GetMatchInfo().agents_info_mutex);
            const auto& agents_info = plugin.match_handler->GetMatchInfo().agents_info;

            std::vector<const AgentInfo*> agent_list;
            agent_list.reserve(agents_info.size());
            for(const auto& pair : agents_info) {
                agent_list.push_back(&pair.second);
            }
            
            std::sort(agent_list.begin(), agent_list.end(), [](const AgentInfo* a, const AgentInfo* b) {
                if (a->party_id != b->party_id) return a->party_id < b->party_id;
                if (a->type != b->type) return static_cast<int>(a->type) < static_cast<int>(b->type);
                return a->agent_id < b->agent_id;
            });

            if (agent_list.empty()) {
                ImGui::TextDisabled("No party data captured yet.");
            } else {
                auto AgentTypeToString = [](AgentType type) -> const char* {
                    switch (type) {
                        case AgentType::PLAYER: return "Player";
                        case AgentType::HERO: return "Hero";
                        case AgentType::HENCHMAN: return "Henchman";
                        case AgentType::OTHER: return "Other";
                        default: return "Unknown";
                    }
                };

                uint32_t current_party_id = (uint32_t)-1; 
                AgentType current_type = (AgentType)-1;
                bool party_node_open = false;

                for (const auto* agent_ptr : agent_list) {
                     const AgentInfo& agent = *agent_ptr;
                    if (agent.party_id != current_party_id) {
                        if (party_node_open) {
                            ImGui::Unindent(); 
                            ImGui::TreePop(); 
                        }
                        current_party_id = agent.party_id;
                        party_node_open = ImGui::TreeNode((void*)(intptr_t)current_party_id, "Party %u", current_party_id);
                        current_type = (AgentType)-1; 
                    }

                    if (party_node_open) {
                        if (agent.type != current_type) {
                            if (current_type != (AgentType)-1) {
                                ImGui::Unindent();
                            }
                            current_type = agent.type;
                            size_t count = 0;
                            for(const auto* other_agent_ptr : agent_list) {
                                if (other_agent_ptr->party_id == current_party_id && other_agent_ptr->type == current_type) {
                                    count++;
                                }
                            }
                            ImGui::Text("  %ss (%zu):", AgentTypeToString(current_type), count);
                            ImGui::Indent();
                        }

                        std::stringstream ss_display;
                        ss_display << agent.agent_id
                                << " (L" << agent.level
                                << " T" << agent.team_id << ")"
                                << " (G" << agent.guild_id << ")"
                                << " (" << agent.primary_profession
                                << "/" << agent.secondary_profession << ")";
                        if (agent.type == AgentType::PLAYER) {
                            ss_display << " [Player#: " << agent.player_number << "]";
                        }
                        if (!agent.encoded_name.empty()) {
                            std::string narrow_name_display;
                            narrow_name_display.reserve(agent.encoded_name.length());
                            for (wchar_t wc : agent.encoded_name) {
                                if (wc >= 32 && wc <= 126) {
                                    narrow_name_display += static_cast<char>(wc);
                                }
                            }
                            ss_display << " Name: " << narrow_name_display;
                        }

                        if (!agent.used_skill_ids.empty()) {
                            ss_display << " Skills: [";
                            bool first_skill = true;
                            for (uint32_t skill_id : agent.used_skill_ids) {
                                if (!first_skill) ss_display << ", ";
                                ss_display << skill_id;
                                first_skill = false;
                            }
                            ss_display << "]";
                        }
                        ImGui::TextUnformatted(ss_display.str().c_str());
                    }
                }
                if (party_node_open) { 
                    ImGui::Unindent(); 
                    ImGui::TreePop(); 
                }
            }
        } else {
            ImGui::TextDisabled("Match handler not available.");
        }
        ImGui::Unindent();
    }
    ImGui::End();
} 