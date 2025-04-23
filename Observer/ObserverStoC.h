#pragma once

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>
#include <cstdint>
#include <unordered_map>

class ObserverPlugin;

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
    
    // common handler methods for Generic packets (similar to ObserverModule)
    void handleGenericPacket(uint32_t value_id, uint32_t caster_id, uint32_t target_id, uint32_t value, bool no_target);
    void handleGenericPacket(uint32_t value_id, uint32_t caster_id, uint32_t target_id, float value, bool no_target);
    
    // specific handlers for various events
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
    void handleKnockdown(uint32_t cause_id, uint32_t target_id);
    void handleAgentMovement(uint32_t agent_id, float x, float y, uint16_t plane); 

    // private helper functions for logging and cleanup
    void logActionActivation(uint32_t caster_id, uint32_t target_id, uint32_t skill_id,
                             bool no_target, const wchar_t* action_desc_format);
    void logActionCompletion(uint32_t caster_id, const wchar_t* completion_desc_format,
                             const wchar_t* unknown_desc_format);
    void cleanupAgentActions(); 
};