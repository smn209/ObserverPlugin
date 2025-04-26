#pragma once

#include <cstdint>
#include <string>

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
        uint32_t h007C[10];
    };


    std::string SafeWcharToString(const wchar_t* wstr);

    void DrawObserverTeam(uint32_t team_index, const TeamDetails* team,
                          bool show_id, bool show_name, bool show_unknowns,
                          bool show_cape_colors, bool show_cape_design);

    std::string GetMatchTypeName(uint32_t type);

    const char* MatchTypeToString(uint32_t type);

    const char* TeamIdToString(uint32_t team_id);


} 