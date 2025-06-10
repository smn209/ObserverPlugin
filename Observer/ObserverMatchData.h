#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <mutex>

#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Guild.h>


namespace ObserverData {

    struct TeamDetails {
        uint32_t id;
        uint32_t h0004;
        uint32_t h0008;
        GW::CapeDesign tabard;
        wchar_t* name;
    };

    struct MatchDetails {
        uint32_t match_id;
        uint32_t match_id_dup;
        uint32_t map_id;
        uint32_t age;
        uint32_t type;
        uint32_t h0014;
        uint32_t h0018;
        uint32_t count;
        TeamDetails team[2];
        wchar_t* team1_name_dup;
        uint32_t h007C[10];    };

    std::string SafeWcharToString(const wchar_t* wstr);

    void DrawObserverTeam(uint32_t team_index, const TeamDetails* team,
                          bool show_id, bool show_name, bool show_unknowns,
                          bool show_cape_colors, bool show_cape_design);

    std::string GetMatchTypeName(uint32_t type);

    const char* MatchTypeToString(uint32_t type);

    const char* TeamIdToString(uint32_t team_id);

} // namespace ObserverData

namespace ObserverMatchData {

    struct TeamInfo {
        std::wstring guild_name;
        std::wstring guild_tag;
        uint32_t rank = 0;
        uint32_t rating = 0;
        uint32_t team_id = 0;
        
        TeamInfo() = default;
        TeamInfo(const std::wstring& name, const std::wstring& tag, uint32_t id) 
            : guild_name(name), guild_tag(tag), team_id(id) {}
    };

    struct LordDamageData {
        std::map<uint32_t, long> team_damage;
        std::map<uint32_t, TeamInfo> team_info;
        mutable std::mutex mutex;
    };

    void InitializeLordDamage();
    void AddTeamLordDamage(uint32_t team_id, long damage);
    void SetTeamInfo(uint32_t team_id, const std::wstring& guild_name, const std::wstring& guild_tag, uint32_t rank = 0, uint32_t rating = 0);
    void ResetLordDamage();
    long GetTeamLordDamage(uint32_t team_id);
    TeamInfo GetTeamInfo(uint32_t team_id);

} // namespace ObserverMatchData