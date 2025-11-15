#include "ObserverMatchData.h"
#include "TextUtils.h"

#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <imgui.h>
#include <vector> 

namespace ObserverData {

    static std::map<uint32_t, std::string> match_type_name = {
        {0, "Special Events"},
        {1, "HA HoH"},
        {4, "GvG"},
        {5, "HA"},
        {9, "GvG AT"},
    };

    std::string SafeWcharToString(const wchar_t* wstr) {
        if (!wstr) return "N/A";
        
        std::wstring ws(wstr);
        std::string narrow_str;
        narrow_str.reserve(ws.length());
        for (wchar_t wc : ws) {
            if (wc >= 32 && wc <= 126) {
                narrow_str += static_cast<char>(wc);
            }
        }
        return narrow_str.empty() ? "N/A (empty/non-ASCII)" : narrow_str;
    }

    void DrawObserverTeam (uint32_t team_index, const TeamDetails* team,
                          bool show_id, bool show_name, bool show_unknowns,
                          bool show_cape_colors, bool show_cape_design) {
        (void)team_index; 
        if (!team) return;

        std::string team_name_str = SafeWcharToString(team->name);

        if (show_id) {
            if (!team->id || team_name_str == "N/A" || team_name_str.empty() || !show_name) {
                 ImGui::Text("  Team ID: %u (%s)", team->id, TeamIdToString(team->id));
            } else {
                 ImGui::Text("  Team ID: %u (%s)", team->id, TeamIdToString(team->id)); 
            }
        }
        if (show_name) {
            if (team_name_str == "N/A" || team_name_str.empty()) {
                if (!show_id) ImGui::Text("  Name: N/A"); 
            } else {
                 ImGui::Text("  Name: %s", team_name_str.c_str());
            }
        }


        if (show_unknowns) {
            ImGui::Text("    Unknowns: h0004=0x%X, h0008=0x%X", team->h0004, team->h0008);
        }

        if (show_cape_colors || show_cape_design) {
            const GW::CapeDesign& cape = team->tabard;
            if (show_cape_colors) {
                ImGui::Text("    Cape: BG(0x%X), Detail(0x%X), Emblem(0x%X)",
                            cape.cape_bg_color, cape.cape_detail_color, cape.cape_emblem_color);
            }
            if (show_cape_design) {
                ImGui::Text("            Shape(%u), Detail(%u), Emblem(%u), Trim(%u)",
                            cape.cape_shape, cape.cape_detail, cape.cape_emblem, cape.cape_trim);
            }
        }
    }

    std::string GetMatchTypeName(uint32_t type) {
        auto it = match_type_name.find(type);
        if (it != match_type_name.end()) {
            return it->second;
        }
        return "Unknown Type (" + std::to_string(type) + ")";
    }

    const char* MatchTypeToString(uint32_t type) {
        switch (type) {
            case 9: return "GvG AT";
            case 4: return "GvG";
            case 5: return "HA";
            case 1: return "HA HoH";
            case 0: return "Special Event";
            default: return "Unknown";
        }
    }    const char* TeamIdToString(uint32_t team_id) {
        switch (team_id) {
            case 0: return "Gray";
            case 1: return "Blue";
            case 2: return "Red";
            default: return "Unknown";
        }
    }

}

namespace ObserverMatchData {

    static LordDamageData lord_damage_data;    
    void InitializeLordDamage() {
        std::lock_guard<std::mutex> lock(lord_damage_data.mutex);
        lord_damage_data.team_damage.clear();
    }

    void AddTeamLordDamage(uint32_t team_id, long damage) {
        std::lock_guard<std::mutex> lock(lord_damage_data.mutex);
        lord_damage_data.team_damage[team_id] += damage;
        if (lord_damage_data.team_damage[team_id] < 0) {
            lord_damage_data.team_damage[team_id] = 0;
        }
    }

    void ResetLordDamage() {
        std::lock_guard<std::mutex> lock(lord_damage_data.mutex);
        for (auto& pair : lord_damage_data.team_damage) {
            pair.second = 0;
        }
    }

    long GetTeamLordDamage(uint32_t team_id) {
        std::lock_guard<std::mutex> lock(lord_damage_data.mutex);
        auto it = lord_damage_data.team_damage.find(team_id);
        return (it != lord_damage_data.team_damage.end()) ? it->second : 0L;
    }

    static TeamKillCountData team_kill_count_data;
    void InitializeTeamKillCount() {
        std::lock_guard<std::mutex> lock(team_kill_count_data.mutex);
        team_kill_count_data.team_kills.clear();
    }

    void AddTeamKill(uint32_t team_id) {
        std::lock_guard<std::mutex> lock(team_kill_count_data.mutex);
        team_kill_count_data.team_kills[team_id] += 1;
    }

    void ResetTeamKillCount() {
        std::lock_guard<std::mutex> lock(team_kill_count_data.mutex);
        for (auto& pair : team_kill_count_data.team_kills) {
            pair.second = 0;
        }
    }

    uint32_t GetTeamKillCount(uint32_t team_id) {
        std::lock_guard<std::mutex> lock(team_kill_count_data.mutex);
        auto it = team_kill_count_data.team_kills.find(team_id);
        return (it != team_kill_count_data.team_kills.end()) ? it->second : 0U;
    }

}