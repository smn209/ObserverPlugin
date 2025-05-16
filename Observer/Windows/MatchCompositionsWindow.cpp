#include "MatchCompositionsWindow.h"
#include "../ObserverPlugin.h"
#include "../ObserverMatch.h"
#include "../ObserverMatchData.h"
#include "../ResourcesHelper.h"
#include "../TextUtils.h"

#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Managers/AgentMgr.h>
#include <Modules/Resources.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <stringapiset.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Constants/Constants.h>

namespace {
    constexpr float SKILL_ICON_SIZE = 32.0f;
    constexpr float ELITE_BORDER_THICKNESS = 2.0f;
    const ImVec4 ELITE_BORDER_COLOR = ImVec4(1.0f, 0.84f, 0.0f, 1.0f);
    constexpr float PLAYER_ROW_PADDING = 3.0f;
    const float TEAM_HEADER_PADDING = 10.0f;
    const float PROF_ICON_SIZE = 20.0f;
}

void MatchCompositionsWindow::Draw(ObserverPlugin& obs_plugin, bool& is_visible) {
    if (!is_visible || !obs_plugin.match_handler) return;

    MatchInfo& match_info = obs_plugin.match_handler->GetMatchInfo();
    auto agents_copy = match_info.GetAgentsInfoCopy();
    auto guilds_copy = match_info.GetGuildsInfoCopy();

    if (!obs_plugin.match_handler->IsObserving() && agents_copy.empty()) {
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Match Compositions Status", &is_visible)) {
            ImGui::TextDisabled("Waiting for observer mode to start or end...");
        }
        ImGui::End();
        return;
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

    if (!can_display_teams) {
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Match Compositions Status", &is_visible)) {
            ImGui::TextDisabled("Waiting for opponent information...");
        }
        ImGui::End();
        return;
    }

    uint32_t team_index = 0;
    for (auto const& [team_id, team_agents] : teams) {
        if (team_index >= 2) break;

        std::string team_name = "Team " + std::to_string(team_id);
        std::string team_tag = "";
        
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
                team_tag = obs_plugin.WStringToString(it->second.tag);
            }
        }

        std::string window_title = "#" + std::to_string(team_id) + ": " + team_name;
        if (!team_tag.empty()) {
            window_title += " [" + team_tag + "]";
        }
        
        if (match_info.winner_party_id != 0 && team_id == match_info.winner_party_id) {
            window_title += " (WINNER)";
        }

        ImGui::PushID(team_id);
        ImGui::SetNextWindowSize(ImVec2(317, 655), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.55f));

        if (ImGui::Begin(window_title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoResize)) {
            std::vector<AgentInfo> sorted_agents = team_agents;
            std::sort(sorted_agents.begin(), sorted_agents.end(), [](const AgentInfo& a, const AgentInfo& b) {
                if (a.type != b.type) return a.type < b.type;
                return a.player_number < b.player_number;
            });

            if (ImGui::BeginChild("PlayerList", ImVec2(0,0), false)) {
                for (const auto& agent : sorted_agents) {
                    ImGui::Dummy(ImVec2(0.0f, PLAYER_ROW_PADDING));
                    ImGui::BeginGroup();

                    auto prof = static_cast<GW::Constants::Profession>(agent.primary_profession);
                    auto sec_prof = static_cast<GW::Constants::Profession>(agent.secondary_profession);

                    ImGui::Text("%s/%s", GW::Constants::GetProfessionAcronym(prof), GW::Constants::GetProfessionAcronym(sec_prof));
                    ImGui::SameLine(0.0f, 5.0f);

                    const wchar_t* agent_name = ObserverUtils::DecodeAgentName(agent.encoded_name);
                    if (agent_name && agent_name[0] != L'\0' && wcscmp(agent_name, L"<Decoding...>") != 0) {
                        ImGui::Text("%S", agent_name);
                    } else {
                        ImGui::TextDisabled("<Decoding...>");
                    }
                    ImGui::EndGroup();

                    ImGui::Indent();
                    ImGui::BeginGroup();
                    size_t num_skills = agent.used_skill_ids.size();
                    
                    for (int i = 0; i < 8; ++i) {
                        uint32_t skill_id = 0;
                        bool skill_exists = static_cast<size_t>(i) < num_skills;

                        if (skill_exists) {
                           skill_id = agent.used_skill_ids[i];
                           if (skill_id == 0) skill_exists = false;
                        } else {

                        }

                        if (skill_exists) {
                            SkillInfo& skill_info_ref = GetSkillInfo(skill_id);
                            IDirect3DTexture9** tex_ptr_ptr = GetSkillImage(static_cast<GW::Constants::SkillID>(skill_id));

                            if (tex_ptr_ptr && *tex_ptr_ptr) {
                                ImVec4 border_col = (skill_info_ref.info_fetched && skill_info_ref.is_elite) ? ELITE_BORDER_COLOR : ImVec4(0,0,0,0);

                                ImGui::PushID(skill_id + agent.agent_id);
                                ImGui::Image(*tex_ptr_ptr, ImVec2(SKILL_ICON_SIZE, SKILL_ICON_SIZE), ImVec2(0,0), ImVec2(1,1), ImVec4(1,1,1,1), border_col);

                                if (skill_info_ref.info_fetched && skill_info_ref.is_elite) {
                                    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::ColorConvertFloat4ToU32(border_col), 0.0f, 0, ELITE_BORDER_THICKNESS);
                                }
                                ImGui::PopID();

                                if (ImGui::IsItemHovered() && skill_info_ref.info_fetched) {
                                    DrawSkillTooltip(skill_info_ref);
                                }
                            } else {
                                ImGui::Dummy(ImVec2(SKILL_ICON_SIZE, SKILL_ICON_SIZE));
                                if (ImGui::IsItemHovered()) {
                                     ImGui::SetTooltip("Skill ID: %u (Loading...)", skill_id);
                                }
                            }
                        } else {
                            ImGui::Dummy(ImVec2(SKILL_ICON_SIZE, SKILL_ICON_SIZE));
                        }

                        if (i < 7) {
                            ImGui::SameLine(0.0f, 2.0f);
                        }
                    }
                    ImGui::EndGroup();
                    ImGui::Unindent();
                    ImGui::Dummy(ImVec2(0.0f, PLAYER_ROW_PADDING));
                    ImGui::Separator();
                }
            } ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopID();
        team_index++;
    }
}

SkillInfo& MatchCompositionsWindow::GetSkillInfo(uint32_t skill_id) {
    auto it = skill_info_cache_.find(skill_id);
    if (it != skill_info_cache_.end()) {
        return it->second;
    }

    SkillInfo& info = skill_info_cache_[skill_id];
    info.skill_id = skill_id;
    info.info_fetched = false;

    GW::Skill* skill = GW::SkillbarMgr::GetSkillConstantData(static_cast<GW::Constants::SkillID>(skill_id));
    if (skill && static_cast<uint32_t>(skill->skill_id) == skill_id) {
        info.is_elite = skill->IsElite();
        info.is_pve = skill->IsPvE();
        info.activation = static_cast<float>(skill->activation) / 1000.f;
        info.recharge = skill->recharge;
        info.energy_cost = skill->GetEnergyCost();
        info.adrenaline = skill->adrenaline;
        info.health_cost = skill->health_cost;
        info.profession_acronym = GW::Constants::GetProfessionAcronym(static_cast<GW::Constants::Profession>(skill->profession));
        GetSkillTypeString(skill, info);
        GetSkillName(skill, info);
        GetFormattedSkillDescription(skill, info);
        info.info_fetched = true;
    }
    return info;
}

const wchar_t* MatchCompositionsWindow::GetSkillName(GW::Skill* skill, SkillInfo& info) {
    if (!skill) return L"";
    if (!info.name_enc[0] && GW::UI::UInt32ToEncStr(skill->name, info.name_enc, _countof(info.name_enc))) {
        GW::UI::AsyncDecodeStr(info.name_enc, info.name_dec, _countof(info.name_dec));
    }
    return info.name_dec;
}

const wchar_t* MatchCompositionsWindow::GetFormattedSkillDescription(GW::Skill* skill, SkillInfo& info) {
     if (!skill) return L"";

     if (info.desc_formatted[0]) {
          return info.desc_formatted;
      }

     if (!info.desc_enc[0] && GW::UI::UInt32ToEncStr(skill->description, info.desc_enc, _countof(info.desc_enc))) {
         wchar_t buf[128] = {0};
         swprintf_s(
             buf, _countof(buf),
               L"%s\x10A\x104\x101%c\x1\x10B\x104\x101%c\x1\x10C\x104\x101%c\x1",
                info.desc_enc,
                static_cast<wchar_t>(0x100 + (skill->scale0 == skill->scale15 ? skill->scale0 : 991)),
                static_cast<wchar_t>(0x100 + (skill->bonusScale0 == skill->bonusScale15 ? skill->bonusScale0 : 992)),
                static_cast<wchar_t>(0x100 + (skill->duration0 == skill->duration15 ? skill->duration0 : 993))
            );
         wcscpy_s(info.desc_enc, _countof(info.desc_enc), buf);
         GW::UI::AsyncDecodeStr(info.desc_enc, info.desc_dec, _countof(info.desc_dec));
     }

     const wchar_t* raw_description = info.desc_dec;
     if (!raw_description[0] || wcscmp(raw_description, L"<Decoding...>") == 0) {
         return L"";
     }

     const wchar_t* unknown_scale_txt = L"???";
     std::wstring s(raw_description);

     size_t pos;
     while ((pos = s.find(L"991")) != std::wstring::npos) { s.replace(pos, 3, unknown_scale_txt); }
     while ((pos = s.find(L"992")) != std::wstring::npos) { s.replace(pos, 3, unknown_scale_txt); }
     while ((pos = s.find(L"993")) != std::wstring::npos) { s.replace(pos, 3, unknown_scale_txt); }

     if (info.type_str[0]) {
         swprintf_s(info.desc_formatted, _countof(info.desc_formatted), L"%s. %s", info.type_str, s.c_str());
     } else {
         wcscpy_s(info.desc_formatted, _countof(info.desc_formatted), s.c_str());
     }
     return info.desc_formatted;
 }

const wchar_t* MatchCompositionsWindow::GetSkillTypeString(GW::Skill* skill, SkillInfo& info) {
     if (!skill) return L"";
     if (info.type_str[0]) return info.type_str;

     std::wstring str = skill->IsElite() ? L"Elite " : L"";
     switch (static_cast<GW::Constants::SkillType>(skill->type)) {
         case GW::Constants::SkillType::Stance: str += L"Stance"; break;
         case GW::Constants::SkillType::Hex: str += L"Hex Spell"; break;
         case GW::Constants::SkillType::Spell: str += L"Spell"; break;
         case GW::Constants::SkillType::Enchantment:
             if (skill->special & 0x800000) str += L"Flash ";
             str += L"Enchantment Spell"; break;
         case GW::Constants::SkillType::Signet: str += L"Signet"; break;
         case GW::Constants::SkillType::Well: str += L"Well Spell"; break;
         case GW::Constants::SkillType::Skill: str += L"Touch Skill"; break;
         case GW::Constants::SkillType::Ward: str += L"Ward Spell"; break;
         case GW::Constants::SkillType::Glyph: str += L"Glyph"; break;
         case GW::Constants::SkillType::Attack:
             switch (skill->weapon_req) {
                 case 1: str += L"Axe Attack"; break;
                 case 2: str += L"Bow Attack"; break;
                 case 8:
                     switch (skill->combo) {
                         case 1: str += L"Lead Attack"; break;
                         case 2: str += L"Off-Hand Attack"; break;
                         case 3: str += L"Dual Attack"; break;
                         default: str += L"Dagger Attack"; break;
                     }
                     break;
                 case 16: str += L"Hammer Attack"; break;
                 case 32: str += L"Scythe Attack"; break;
                 case 64: str += L"Spear Attack"; break;
                 case 70: str += L"Ranged Attack"; break;
                 case 128: str += L"Sword Attack"; break;
                 default: str += L"Melee Attack"; break;
             }
             break;
         case GW::Constants::SkillType::Shout: str += L"Shout"; break;
         case GW::Constants::SkillType::Preparation: str += L"Preparation"; break;
         case GW::Constants::SkillType::PetAttack: str += L"Pet Attack"; break;
         case GW::Constants::SkillType::Trap: str += L"Trap"; break;
         case GW::Constants::SkillType::Ritual:
             switch (static_cast<GW::Constants::ProfessionByte>(skill->profession)) {
                 case GW::Constants::ProfessionByte::Ritualist: str += L"Binding Ritual"; break;
                 case GW::Constants::ProfessionByte::Ranger: str += L"Nature Ritual"; break;
                 default: str += L"Ebon Vanguard Ritual"; break;
             }
             break;
         case GW::Constants::SkillType::ItemSpell: str += L"Item Spell"; break;
         case GW::Constants::SkillType::WeaponSpell: str += L"Weapon Spell"; break;
         case GW::Constants::SkillType::Form: str += L"Form"; break;
         case GW::Constants::SkillType::Chant: str += L"Chant"; break;
         case GW::Constants::SkillType::EchoRefrain: str += L"Echo"; break;
         default: str += L"Skill"; break;
     }

     wcscpy_s(info.type_str, _countof(info.type_str), str.c_str());
     return info.type_str;
 }

 void MatchCompositionsWindow::DrawSkillTooltip(const SkillInfo& info) {
     ImGui::BeginTooltip();
     ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

     ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "%S", info.name_dec);
     ImGui::Separator();
     if(info.type_str[0]) ImGui::Text("Type: %S", info.type_str);
     if(info.profession_acronym[0] != '?') ImGui::Text("Profession: %s", info.profession_acronym);
     ImGui::Separator();
     ImGui::TextWrapped("%S", info.desc_formatted);
     ImGui::Separator();

     if(info.activation > 0.0f) ImGui::Text("Activation: %.1fs", info.activation);
     if(info.recharge > 0) ImGui::Text("Recharge: %ds", info.recharge);
     if (info.energy_cost > 0) ImGui::Text("Energy Cost: %d", info.energy_cost);
     if (info.adrenaline > 0) ImGui::Text("Adrenaline Cost: %d", info.adrenaline);
     if (info.health_cost > 0) ImGui::Text("Health Cost: %d%%", info.health_cost);

     if (info.is_pve) {
         ImGui::Separator();
         ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "(PvE Only Skill)");
     }

     ImGui::PopTextWrapPos();
     ImGui::EndTooltip();
 }