#pragma once

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>
#include "ObserverPackets.h"
#include <cstdint>
#include <unordered_map>

class ObserverPlugin;

extern const wchar_t* MARKER_SKILL_EVENT;
extern const size_t MARKER_SKILL_EVENT_LEN;
extern const wchar_t* MARKER_ATTACK_SKILL_EVENT;
extern const size_t MARKER_ATTACK_SKILL_EVENT_LEN;
extern const wchar_t* MARKER_BASIC_ATTACK_EVENT;
extern const size_t MARKER_BASIC_ATTACK_EVENT_LEN;
extern const wchar_t* MARKER_COMBAT_EVENT;
extern const size_t MARKER_COMBAT_EVENT_LEN;
extern const wchar_t* MARKER_AGENT_EVENT;
extern const size_t MARKER_AGENT_EVENT_LEN;
extern const wchar_t* MARKER_JUMBO_EVENT;
extern const size_t MARKER_JUMBO_EVENT_LEN;
extern const wchar_t* MARKER_LORD_EVENT;
extern const size_t MARKER_LORD_EVENT_LEN;

// struct to store active action details (skill and target)
// similar concept to ObserverModule::TargetAction but simplified for logging needs
struct ActiveActionInfo {
    uint32_t skill_id = 0;
    uint32_t target_id = 0;
};

// handles server-to-client (StoC) packet callbacks for the observer plugin
class ObserverStoC {
public:
    ObserverStoC(ObserverPlugin* owner_plugin);
    ~ObserverStoC(); 
    
    void RegisterCallbacks();
    void RemoveCallbacks();

private:
    ObserverPlugin* owner; // pointer back to the main plugin instance
    
    // map to track the currently active action (skill/target) for each agent
    std::unordered_map<uint32_t, ActiveActionInfo*> agent_active_action;
    
    // hook entries for packet callbacks
    GW::HookEntry GenericValueTarget_Entry;
    GW::HookEntry GenericValue_Entry;
    GW::HookEntry GenericModifier_Entry;
    GW::HookEntry GenericFloat_Entry;
    GW::HookEntry AgentMovement_Entry;
    GW::HookEntry JumboMessage_Entry;
    GW::HookEntry OpposingPartyGuild_Entry;
    
    // common handlers dispatch generic packet data based on value_id
    void handleGenericPacket(uint32_t value_id, uint32_t caster_id, uint32_t target_id, uint32_t value, bool no_target);
    void handleGenericPacket(uint32_t value_id, uint32_t caster_id, uint32_t target_id, float value, bool no_target);
    
    // specific handlers for different game events derived from generic packets
    void handleSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id, bool no_target);
    void handleSkillFinished(uint32_t caster_id);
    void handleAttackSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id, bool no_target);
    void handleSkillStopped(uint32_t caster_id);
    void handleInterrupted(uint32_t caster_id);
    void handleInstantSkillActivated(uint32_t caster_id, uint32_t skill_id);
    void handleAttackSkillStopped(uint32_t caster_id);
    void handleAttackSkillFinished(uint32_t caster_id);
    void handleAttackStarted(uint32_t caster_id, uint32_t target_id, bool no_target);
    void handleAttackStopped(uint32_t caster_id);
    void handleAttackFinished(uint32_t caster_id);
    void handleDamage(uint32_t caster_id, uint32_t target_id, float value, uint32_t damage_type);
    void handleLordDamage(uint32_t caster_id, uint32_t target_id, float value, uint32_t damage_type, uint32_t attacking_team, long damage, long damage_before, long damage_after);
    void handleKnockdown(uint32_t cause_id, uint32_t target_id);
    void handleAgentMovement(uint32_t agent_id, float x, float y, uint16_t plane);
    void handleJumboMessage(const GW::Packet::StoC::JumboMessage* packet);
    void handleDamagePacket(uint32_t caster_id, uint32_t target_id, float value, uint32_t damage_type);
    void handleValueTargetPacket(GW::Packet::StoC::GenericValueTarget* packet);

    // private helper functions for logging and cleanup
    void logActionActivation(uint32_t caster_id, uint32_t target_id, uint32_t skill_id,
                             bool no_target, const wchar_t* action_desc_format,
                             const wchar_t* category_marker);
    void logActionCompletion(uint32_t caster_id, const wchar_t* action_identifier, 
                             uint32_t value_id, 
                             const wchar_t* category_marker);
    void cleanupAgentActions(); 
};