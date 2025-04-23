#pragma once

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>
#include <cstdint>

class ObserverPlugin;

class ObserverStoC {
public:
    ObserverStoC(ObserverPlugin* owner_plugin);
    ~ObserverStoC() = default;
    
    void RegisterCallbacks();
    void RemoveCallbacks();

private:
    ObserverPlugin* owner;
    
    GW::HookEntry GenericValueTarget_Entry;
    GW::HookEntry GenericValue_Entry;
    GW::HookEntry GenericModifier_Entry;
    
    void handleSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id);
    void handleSkillFinished(uint32_t caster_id, uint32_t target_id, uint32_t skill_id);
    void handleAttackSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id);
    void handleSkillStopped(uint32_t caster_id, uint32_t value);
    void handleInterrupted(uint32_t caster_id, uint32_t value);
    void handleInstantSkillActivated(uint32_t caster_id, uint32_t skill_id);
    void handleAttackSkillStopped(uint32_t caster_id, uint32_t value);
    void handleAttackSkillFinished(uint32_t caster_id, uint32_t value);
    void handleDamage(uint32_t caster_id, uint32_t target_id, float value, uint32_t damage_type);
    void handleKnockdown(uint32_t caster_id, uint32_t target_id);
};