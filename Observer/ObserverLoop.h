#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>
#include <optional>

class ObserverPlugin;
class ObserverMatch;
struct MatchInfo;
struct AgentInfo;
namespace GW { 
    struct Agent; 
    struct PartyInfo; 
    struct PlayerPartyMember; 
    struct HeroPartyMember; 
    struct HenchmanPartyMember; 
    struct AgentLiving;
} 

struct AgentState {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rotation_angle = 0.0f;

    uint32_t weapon_id = 0;
    uint32_t model_id = 0;
    uint32_t gadget_id = 0;

    bool is_alive = false;
    bool is_dead = false;
    float health_pct = 0.0f;
    bool is_knocked = false;
    uint32_t max_hp = 0;

    bool has_condition = false;
    bool has_deep_wound = false;
    bool has_bleeding = false;
    bool has_crippled = false;
    bool has_blind = false; 
    bool has_poison = false;
    bool has_hex = false;
    bool has_degen_hex = false;

    bool has_enchantment = false;
    bool has_weapon_spell = false;

    bool is_holding = false;
    bool is_casting = false;    uint32_t skill_id = 0;
    uint8_t weapon_item_type = 0;
    uint8_t offhand_item_type = 0;
    uint16_t weapon_item_id = 0;
    uint16_t offhand_item_id = 0;
    float move_x = 0.0f;
    float move_y = 0.0f;
    uint16_t visual_effects = 0;
    uint8_t team_id = 0;
    uint16_t weapon_type = 0;
    float weapon_attack_speed = 0.0f;
    float attack_speed_modifier = 0.0f;
    uint8_t dagger_status = 0;
    float hp_pips = 0.0f;
    uint32_t model_state = 0;
    uint32_t animation_code = 0;
    uint32_t animation_id = 0;
    float animation_speed = 0.0f;
    float animation_type = 0.0f;
    uint32_t in_spirit_range = 0;
    uint16_t agent_model_type = 0;
    uint32_t item_id = 0;
    uint32_t item_extra_type = 0;
    uint32_t gadget_extra_type = 0;bool operator==(const AgentState& other) const {
        return x == other.x && y == other.y && z == other.z &&
               rotation_angle == other.rotation_angle &&
               weapon_id == other.weapon_id &&
               model_id == other.model_id &&
               gadget_id == other.gadget_id &&
               is_alive == other.is_alive && is_dead == other.is_dead &&
               health_pct == other.health_pct &&
               is_knocked == other.is_knocked && max_hp == other.max_hp &&
               has_condition == other.has_condition && has_deep_wound == other.has_deep_wound &&
               has_bleeding == other.has_bleeding && has_crippled == other.has_crippled &&
               has_blind == other.has_blind && has_poison == other.has_poison &&
               has_hex == other.has_hex && has_degen_hex == other.has_degen_hex &&
               has_enchantment == other.has_enchantment && has_weapon_spell == other.has_weapon_spell &&
               is_holding == other.is_holding && is_casting == other.is_casting &&
               skill_id == other.skill_id &&
               weapon_item_type == other.weapon_item_type && offhand_item_type == other.offhand_item_type &&
               weapon_item_id == other.weapon_item_id && offhand_item_id == other.offhand_item_id &&
               move_x == other.move_x && move_y == other.move_y &&
               visual_effects == other.visual_effects && team_id == other.team_id &&
               weapon_type == other.weapon_type &&
               weapon_attack_speed == other.weapon_attack_speed && attack_speed_modifier == other.attack_speed_modifier &&
               dagger_status == other.dagger_status && hp_pips == other.hp_pips &&
               model_state == other.model_state && animation_code == other.animation_code &&
               animation_id == other.animation_id && animation_speed == other.animation_speed &&
               animation_type == other.animation_type && in_spirit_range == other.in_spirit_range &&
               agent_model_type == other.agent_model_type &&
               item_id == other.item_id && item_extra_type == other.item_extra_type &&
               gadget_extra_type == other.gadget_extra_type;
    }

    bool operator!=(const AgentState& other) const {
        return !(*this == other);
    }
};

// logs agent state periodically during observer mode
class ObserverLoop {
public:
    ObserverLoop(ObserverPlugin* owner_plugin, ObserverMatch* match_handler);
    ~ObserverLoop();

    void Start(); // starts the background logging thread
    void Stop();  // signals the background thread to stop and joins it

    
    bool ExportAgentLogs(const wchar_t* folder_name); // exports accumulated agent logs into separate gzip files per agent
    void ClearAgentLogs(); // clears all accumulated agent logs

    // checks if the background loop is currently running
    bool IsRunning() const;

private:
    void RunLoop(); 
    AgentState GetAgentState(GW::Agent* agent); 
    void UpdatePartiesInformations(); 
    void MaybeUpdateGuildInfo(uint16_t guild_id, MatchInfo& match_info);
    void PopulateLivingAgentDetails(GW::AgentLiving* living, AgentInfo& info); 

    ObserverPlugin* owner_ = nullptr; // pointer back to the main plugin instance
    ObserverMatch* match_handler_ = nullptr; // pointer to ObserverMatch to update MatchInfo
    std::thread loop_thread_;        // the background thread handle
    std::atomic<bool> run_loop_;     // flag to control the loop execution
    
    mutable std::mutex log_mutex_;           // mutex to protect access to agent_logs_ and last_log_entry_
    std::map<uint32_t, std::vector<std::pair<uint32_t, AgentState>>> agent_logs_; // store pairs of (timestamp_ms, state)
    std::map<uint32_t, AgentState> last_agent_state_; // store last state struct
}; 