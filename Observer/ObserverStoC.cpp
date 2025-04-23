#include "ObserverStoC.h"
#include "ObserverPlugin.h"
#include "ObserverUtils.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/ChatMgr.h>

ObserverStoC::ObserverStoC(ObserverPlugin* owner_plugin) : owner(owner_plugin) {
}

void ObserverStoC::RegisterCallbacks() {
    // register the callbacks (from ObserverStoC using GWCA) 
    // including: 
    // - GenericValueTarget
    // - GenericValue
    // - GenericModifier
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValueTarget>(
        &GenericValueTarget_Entry,
        [this](const GW::HookStatus*, const GW::Packet::StoC::GenericValueTarget* packet) -> void {
            if (!owner || !owner->enabled) return; // if we dont have owner defined (plugin) or that the plugin is not enabled, return

            if (packet->Value_id == GW::Packet::StoC::GenericValueID::skill_activated) {
                this->handleSkillActivated(packet->target, packet->caster, packet->value);
            } else if (packet->Value_id == GW::Packet::StoC::GenericValueID::skill_finished) {
                this->handleSkillFinished(packet->target, packet->caster, packet->value);
            } else if (packet->Value_id == GW::Packet::StoC::GenericValueID::attack_skill_activated) {
                this->handleAttackSkillActivated(packet->target, packet->caster, packet->value);
            }
        });
    
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValue>(
        &GenericValue_Entry,
        [this](const GW::HookStatus*, const GW::Packet::StoC::GenericValue* packet) -> void {
            if (!owner || !owner->enabled) return; // if we dont have owner defined (plugin) or that the plugin is not enabled, return
            
            switch (packet->value_id) {
                case GW::Packet::StoC::GenericValueID::skill_stopped:
                    this->handleSkillStopped(packet->agent_id, packet->value);
                    break;
                case GW::Packet::StoC::GenericValueID::interrupted:
                    this->handleInterrupted(packet->agent_id, packet->value);
                    break;
                case GW::Packet::StoC::GenericValueID::instant_skill_activated:
                    this->handleInstantSkillActivated(packet->agent_id, packet->value);
                    break;
                case GW::Packet::StoC::GenericValueID::attack_skill_stopped:
                    this->handleAttackSkillStopped(packet->agent_id, packet->value);
                    break;
                case GW::Packet::StoC::GenericValueID::attack_skill_finished:
                    this->handleAttackSkillFinished(packet->agent_id, packet->value);
                    break;
            }
        });
        
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericModifier>(
        &GenericModifier_Entry,
        [this](const GW::HookStatus*, const GW::Packet::StoC::GenericModifier* packet) -> void {
            if (!owner || !owner->enabled) return; // if we dont have owner defined (plugin) or that the plugin is not enabled, return
            
            if (packet->type == GW::Packet::StoC::GenericValueID::damage || 
                packet->type == GW::Packet::StoC::GenericValueID::critical ||
                packet->type == GW::Packet::StoC::GenericValueID::armorignoring) {
                this->handleDamage(packet->cause_id, packet->target_id, packet->value, packet->type);
            }
            else if (packet->type == GW::Packet::StoC::GenericValueID::knocked_down) {
                this->handleKnockdown(packet->cause_id, packet->target_id);
            }
        });
}

void ObserverStoC::RemoveCallbacks() {
    // remove every callbacks
    GW::StoC::RemoveCallback<GW::Packet::StoC::GenericValueTarget>(&GenericValueTarget_Entry);
    GW::StoC::RemoveCallback<GW::Packet::StoC::GenericValue>(&GenericValue_Entry);
    GW::StoC::RemoveCallback<GW::Packet::StoC::GenericModifier>(&GenericModifier_Entry);
}

void ObserverStoC::handleSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id) {
    if (!owner->log_skill_activations) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Skill Activated: ID %u by %u on %u", skill_id, caster_id, target_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleSkillFinished(uint32_t caster_id, uint32_t target_id, uint32_t skill_id) {
    if (!owner->log_skill_finishes) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Skill Finished: ID %u by %u on %u", skill_id, caster_id, target_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleAttackSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id) {
    if (!owner->log_attack_skills) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Attack Skill Activated: ID %u by %u on %u", skill_id, caster_id, target_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleSkillStopped(uint32_t caster_id, uint32_t value) {
    if (!owner->log_skill_finishes) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Skill Stopped: Agent %u, Value %u", caster_id, value);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleInterrupted(uint32_t caster_id, uint32_t value) {
    if (!owner->log_interrupts) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Interrupted: Agent %u interrupted skill %u", caster_id, value);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleInstantSkillActivated(uint32_t caster_id, uint32_t skill_id) {
    if (!owner->log_instant_skills) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Instant Skill Used: ID %d, Caster %d", skill_id, caster_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleAttackSkillStopped(uint32_t caster_id, uint32_t value) {
    if (!owner->log_attack_skills) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Attack Skill Stopped: Agent %d, Value %d", caster_id, value);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleAttackSkillFinished(uint32_t caster_id, uint32_t value) {
    if (!owner->log_attack_skills) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Attack Skill Finished: Agent %d, Value %d", caster_id, value);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleDamage(uint32_t caster_id, uint32_t target_id, float value, uint32_t damage_type) {
    if (!owner->log_damage) return;
    
    const wchar_t* damage_type_str = 
        damage_type == static_cast<uint32_t>(GW::Packet::StoC::GenericValueID::critical) ? L"Critical" : 
        (damage_type == static_cast<uint32_t>(GW::Packet::StoC::GenericValueID::armorignoring) ? L"Armor-ignoring" : L"Normal");
    
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Damage (%ls): %f from %d to %d", damage_type_str, value, caster_id, target_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleKnockdown(uint32_t caster_id, uint32_t target_id) {
    if (!owner->log_knockdowns) return;
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Knockdown: Agent %d knocked down agent %d", caster_id, target_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}