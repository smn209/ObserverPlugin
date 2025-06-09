#pragma once

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <GWCA/GameEntities/Guild.h>  
class ObserverStoC; 
class ObserverPlugin;

namespace GW { namespace Packet { namespace StoC { struct InstanceLoadInfo; } } }

struct AgentInfo;

enum class AgentType {
    UNKNOWN,
    PLAYER,
    HERO,
    HENCHMAN,
    PARTY_COMPLETE, // for party members that are not players (means heroes, henchmen, etc.)
    OTHER // NPCs (Guild Lord, Bodyguard, Flag?, etc.)
};

struct AgentInfo {
    uint32_t agent_id = 0;
    uint32_t party_id = 0; 
    AgentType type = AgentType::UNKNOWN;
    uint32_t primary_profession = 0;
    uint32_t secondary_profession = 0;
    uint32_t level = 0;
    uint32_t team_id = 0; 
    uint32_t player_number = 0; 
    std::wstring encoded_name; 
    uint16_t guild_id = 0; 
    std::vector<uint32_t> used_skill_ids;
    uint32_t model_id = 0;
    uint32_t gadget_id = 0;
    std::string skill_template_code; 
};

struct GuildInfo {
    uint16_t guild_id = 0;
    std::wstring name = L"";
    std::wstring tag = L"";
    uint32_t rank = 0;
    uint32_t features = 0;
    uint32_t rating = 0;
    uint32_t faction = 0; // 0=kurzick, 1=luxon
    uint32_t faction_points = 0;
    uint32_t qualifier_points = 0;

    GW::CapeDesign cape{}; 

};

struct MatchInfo {
    uint32_t map_id = 0;
    uint32_t end_time_ms = 0;
    std::wstring end_time_formatted = L""; // store formatted time [mm:ss.ms]
    std::wstring match_duration = L""; // store adjusted formatted time [mm:ss] (minus 1 min)
    std::wstring match_original_duration = L""; // store original duration without adjustment [mm:ss]
    uint32_t winner_party_id = 0; // 0 = not set, 1 = party 1, 2 = party 2

    std::map<uint32_t, AgentInfo> agents_info;
    mutable std::mutex agents_info_mutex; 
    std::map<uint16_t, GuildInfo> guilds_info;
    mutable std::mutex guilds_info_mutex;

    void Reset();

    void ClearAgentInfoMap();

    void ClearGuildInfoMap();

    void UpdateAgentInfo(const AgentInfo& info);
    std::map<uint32_t, AgentInfo> GetAgentsInfoCopy() const;
    void AddSkillUsed(uint32_t agent_id, uint32_t skill_id); 
    void SortAgentSkills(uint32_t agent_id);
    void UpdateAgentSkillTemplate(uint32_t agent_id); 

    void UpdateGuildInfo(const GuildInfo& info);
    std::map<uint16_t, GuildInfo> GetGuildsInfoCopy() const;
};

class ObserverMatch {
public:
    ObserverMatch(ObserverStoC* stoc_handler);
    ~ObserverMatch() = default;

    void RegisterCallbacks();
    void RemoveCallbacks();
    void SetOwnerPlugin(ObserverPlugin* plugin);

    [[nodiscard]] bool IsObserving() const;
    [[nodiscard]] MatchInfo& GetMatchInfo();

    void SetMatchEndInfo(uint32_t end_time_ms, uint32_t raw_winner_id);

    void ClearLogs();
    bool ExportLogsToFolder(const wchar_t* folder_name);
    void HandleMatchEnd();
    void UpdateAgentSkillTemplates();

private:
    void HandleInstanceLoadInfo(const GW::HookStatus* status, const GW::Packet::StoC::InstanceLoadInfo* packet);

    GW::HookEntry InstanceLoadInfo_Entry; // manages the hook for instance load info packets
    bool is_observing = false;            // flag to indicate if the current map is an observer mode instance
    ObserverStoC* stoc_handler_ = nullptr; // pointer to the StoC handler
    ObserverPlugin* owner_plugin = nullptr; // pointer to the owner plugin

    MatchInfo current_match_info_; // holds info for the current/last observed match
}; 