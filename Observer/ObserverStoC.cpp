#include "ObserverStoC.h"
#include "ObserverPlugin.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <cstdio> // for swprintf

// owner_plugin : pointer to the ObserverPlugin instance that created this handler
ObserverStoC::ObserverStoC(ObserverPlugin* owner_plugin) : owner(owner_plugin) {
}

// destructor cleans up dynamically allocated ActiveActionInfo objects 
ObserverStoC::~ObserverStoC() {
    cleanupAgentActions();
}

void ObserverStoC::RegisterCallbacks() {
    // GenericModifier (0x57) (Damage, Knockdown, etc.)
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericModifier>(
        &GenericModifier_Entry,
        [this](const GW::HookStatus*, const GW::Packet::StoC::GenericModifier* packet) -> void {
            if (!owner || !owner->stoc_status) return;

            const uint32_t value_id = packet->type;
            const uint32_t caster_id = packet->cause_id;
            const uint32_t target_id = packet->target_id;
            const float value = packet->value;
            constexpr bool no_target = false; // this packet type always has a target
            handleGenericPacket(value_id, caster_id, target_id, value, no_target);
        }
    );

    // GenericValueTarget (0x55) (Skill Activated, Skill Finished, etc.)
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValueTarget>(
        &GenericValueTarget_Entry,
        [this](const GW::HookStatus*, const GW::Packet::StoC::GenericValueTarget* packet) -> void {
            if (!owner || !owner->stoc_status) return;

            const uint32_t value_id = packet->Value_id;
            const uint32_t caster_id = packet->caster;
            const uint32_t target_id = packet->target;
            const uint32_t value = packet->value;
            constexpr bool no_target = false; // this packet type always has a target
            handleGenericPacket(value_id, caster_id, target_id, value, no_target);
        }
    );

    // GenericValue (0x56) (Skill Stopped, Skill Activated, etc.)
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericValue>(
        &GenericValue_Entry,
        [this](const GW::HookStatus*, const GW::Packet::StoC::GenericValue* packet) -> void {
            if (!owner || !owner->stoc_status) return;

            const uint32_t value_id = packet->value_id;
            const uint32_t caster_id = packet->agent_id;
            constexpr uint32_t target_id = 0; // no target in this packet type
            const uint32_t value = packet->value;
            constexpr bool no_target = true;
            handleGenericPacket(value_id, caster_id, target_id, value, no_target);
        }
    );

    // GenericFloat (0x58) 
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericFloat>(
        &GenericFloat_Entry, 
        [this](const GW::HookStatus*, const GW::Packet::StoC::GenericFloat* packet) -> void {
            if (!owner || !owner->stoc_status) return;

            const uint32_t value_id = packet->type;
            const uint32_t caster_id = packet->agent_id;
            constexpr uint32_t target_id = 0; // no target in this packet type
            const float value = packet->value;
            constexpr bool no_target = true;
            handleGenericPacket(value_id, caster_id, target_id, value, no_target);
        }
    );
    
    // Agent Position Updates (MOVE_TO_POINT 0x29)
    GW::StoC::RegisterPacketCallback(
        &AgentMovement_Entry, 
        GAME_SMSG_AGENT_MOVE_TO_POINT,
        [this](const GW::HookStatus*, GW::Packet::StoC::PacketBase* pak) -> void {
            if (!owner || !owner->stoc_status || !owner->log_movement) return;
            
            // position packet structure : (maybe more infos in GWCA)

            // uint32_t header; (already in PacketBase)
            // uint32_t agent_id; (DWORD)
            // float x; (Vec2 - x coordinate)
            // float y; (Vec2 - y coordinate)
            // uint16_t word1; (plane)
            // uint16_t word2; (unknown data)

            uint32_t* data = (uint32_t*)pak;
            uint32_t agent_id = data[1];
            float x = *(float*)(&data[2]);
            float y = *(float*)(&data[3]);
            uint16_t* word_data = (uint16_t*)(&data[4]);
            uint16_t plane = word_data[0];
            
            handleAgentMovement(agent_id, x, y, plane);
        }
    );

    // JumboMessage (0x18F)
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::JumboMessage>(
        &JumboMessage_Entry, [this](const GW::HookStatus*, const GW::Packet::StoC::JumboMessage* packet) -> void {
            if (!owner || !owner->stoc_status) return;
            handleJumboMessage(packet);
        });
}

void ObserverStoC::RemoveCallbacks() {
    // remove all callbacks and clean up map pointers (active action info)
    GW::StoC::RemoveCallback<GW::Packet::StoC::GenericValueTarget>(&GenericValueTarget_Entry);
    GW::StoC::RemoveCallback<GW::Packet::StoC::GenericValue>(&GenericValue_Entry);
    GW::StoC::RemoveCallback<GW::Packet::StoC::GenericModifier>(&GenericModifier_Entry);
    GW::StoC::RemoveCallback<GW::Packet::StoC::GenericFloat>(&GenericFloat_Entry);
    GW::StoC::RemoveCallback(GAME_SMSG_AGENT_MOVE_TO_POINT, &AgentMovement_Entry);
    GW::StoC::RemoveCallback<GW::Packet::StoC::JumboMessage>(&JumboMessage_Entry);

    cleanupAgentActions(); // clean up map pointers before clearing the map itself
}

void ObserverStoC::cleanupAgentActions() { 
    // helper function to clean up all ActiveActionInfo objects
    for (auto const& [caster_id, action_info_ptr] : agent_active_action) {
        if (action_info_ptr) {
            delete action_info_ptr;
        }
    }
    agent_active_action.clear();
}

void ObserverStoC::handleGenericPacket(const uint32_t value_id, const uint32_t caster_id,
                                       const uint32_t target_id, const float value, const bool)
{
    // handle incoming packets with float values
    switch (value_id) {
        case GW::Packet::StoC::GenericValueID::damage:
        case GW::Packet::StoC::GenericValueID::critical:
        case GW::Packet::StoC::GenericValueID::armorignoring:
            handleDamage(caster_id, target_id, value, value_id);
            break;

        case GW::Packet::StoC::GenericValueID::knocked_down:
            handleKnockdown(caster_id, target_id);
            break;
    }
}

void ObserverStoC::handleGenericPacket(const uint32_t value_id, const uint32_t caster_id,
                                       const uint32_t target_id, const uint32_t value, const bool no_target)
{
    // handle incoming packets with uint32_t values
    switch (value_id) {
        case GW::Packet::StoC::GenericValueID::melee_attack_finished:
            handleAttackFinished(caster_id);
            break;

        case GW::Packet::StoC::GenericValueID::attack_stopped:
            handleAttackStopped(caster_id);
            break;

        case GW::Packet::StoC::GenericValueID::attack_started:
            handleAttackStarted(caster_id, target_id, no_target);
            break;

        case GW::Packet::StoC::GenericValueID::interrupted:
            handleInterrupted(caster_id);
            break;

        case GW::Packet::StoC::GenericValueID::attack_skill_finished:
            handleAttackSkillFinished(caster_id);
            break;

        case GW::Packet::StoC::GenericValueID::instant_skill_activated:
            handleInstantSkillActivated(caster_id, value);
            break;

        case GW::Packet::StoC::GenericValueID::attack_skill_stopped:
            handleAttackSkillStopped(caster_id);
            break;

        case GW::Packet::StoC::GenericValueID::attack_skill_activated:
            handleAttackSkillActivated(caster_id, target_id, value, no_target);
            break;

        case GW::Packet::StoC::GenericValueID::skill_finished:
            handleSkillFinished(caster_id);
            break;

        case GW::Packet::StoC::GenericValueID::skill_stopped:
            handleSkillStopped(caster_id);
            break;

        case GW::Packet::StoC::GenericValueID::skill_activated:
            handleSkillActivated(caster_id, target_id, value, no_target);
            break;
    }
}

void ObserverStoC::logActionActivation(uint32_t caster_id, uint32_t target_id, uint32_t skill_id,
                                        bool no_target, const wchar_t* action_desc_format)
{
    // helper for logging action activations (skills, attacks)
    // resolve actual caster and target based on packet type context
    uint32_t actual_caster_id;
    uint32_t actual_target_id;
    if (no_target) { // genericValue, GenericFloat: packet caster is action caster
        actual_caster_id = caster_id;
        actual_target_id = 0; // no target info in these packets
    } else { // genericValueTarget, GenericModifier: packet caster is action target, packet target is action caster
        actual_caster_id = target_id;
        actual_target_id = caster_id;
    }

    // clean up any previous action stored for this caster
    auto it = agent_active_action.find(actual_caster_id);
    if (it != agent_active_action.end()) {
        delete it->second; // free memory of the old action info
        agent_active_action.erase(it);
    }

    // store new action info
    // use skill_id 0 to represent basic attacks (triggered by attack_started value_id)
    uint32_t stored_skill_id = (skill_id == static_cast<uint32_t>(GW::Packet::StoC::GenericValueID::attack_started)) ? 0 : skill_id;
    ActiveActionInfo* new_action = new ActiveActionInfo{stored_skill_id, actual_target_id};
    agent_active_action[actual_caster_id] = new_action;

    // log to chat
    wchar_t buffer[128];
    if (stored_skill_id == 0) { // basic attack started uses a different format string
         swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), action_desc_format, actual_caster_id, actual_target_id);
    } else { // skill activation
        swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), action_desc_format, stored_skill_id, actual_caster_id, actual_target_id);
    }
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}


void ObserverStoC::logActionCompletion(uint32_t caster_id, const wchar_t* completion_desc_format, const wchar_t* unknown_desc_format)
{
    // helper for logging action completions (finished, stopped, interrupted)
    wchar_t buffer[128];
    auto it = agent_active_action.find(caster_id);
    if (it != agent_active_action.end()) {
        // found stored action info for this caster
        ActiveActionInfo* action_info = it->second;
        uint32_t skill_id = action_info->skill_id;
        uint32_t target_id = action_info->target_id;

        // format message based on whether it was a basic attack (skill_id 0) or a skill
        if (skill_id == 0) { // basic attack completion/stop
            swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), completion_desc_format, caster_id, target_id);
        } else { // skill completion/stop/interrupt
            swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), completion_desc_format, skill_id, caster_id, target_id);
        }

        // clean up the stored action info
        delete action_info;
        agent_active_action.erase(it);
    } else {
        // action info not found (e.g., stop packet without preceding start), log with unknown skill/target format
        swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), unknown_desc_format, caster_id);
    }
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleAgentMovement(uint32_t agent_id, float x, float y, uint16_t plane) {
    if (!owner || !owner->stoc_status || !owner->log_movement) return;
    
    wchar_t buffer[128];
    swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"Agent Movement: ID %u to position (%.2f, %.2f) on plane %u", agent_id, x, y, plane);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id, bool no_target) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_skill_activations) return;
    logActionActivation(caster_id, target_id, skill_id, no_target, L"Skill Activated: ID %u by %u on %u");
}

void ObserverStoC::handleAttackSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id, bool no_target) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_attack_skill_activations) return;
    logActionActivation(caster_id, target_id, skill_id, no_target, L"Attack Skill Activated: ID %u by %u on %u");
}

void ObserverStoC::handleInstantSkillActivated(uint32_t caster_id, uint32_t skill_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_instant_skills) return;
    // instant skills don't store state in agent_active_action and target is usually the caster.
    wchar_t buffer[128];
    swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"Instant Skill Used: ID %u by %u on %u", skill_id, caster_id, caster_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleAttackStarted(uint32_t caster_id, uint32_t target_id, bool no_target) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_basic_attack_starts) return;
    // use attack_started value_id as a placeholder skill_id to trigger basic attack logging format in helper
    logActionActivation(caster_id, target_id, static_cast<uint32_t>(GW::Packet::StoC::GenericValueID::attack_started), no_target, L"Attack Started: by %u on %u");
}

void ObserverStoC::handleAttackStopped(uint32_t caster_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_basic_attack_stops) return;
    logActionCompletion(caster_id, L"Attack Stopped: by %u on %u", L"Attack Stopped: by %u");
}

void ObserverStoC::handleSkillFinished(uint32_t caster_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_skill_finishes) return;
    logActionCompletion(caster_id, L"Skill Finished: ID %u by %u on %u", L"Skill Finished: (Unknown Skill) by %u");
}

void ObserverStoC::handleSkillStopped(uint32_t caster_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_skill_stops) return;
    logActionCompletion(caster_id, L"Skill Stopped: ID %u by %u on %u", L"Skill Stopped: (Unknown Skill) by %u");
}

void ObserverStoC::handleInterrupted(uint32_t caster_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_interrupts) return;
    logActionCompletion(caster_id, L"Interrupted: Skill %u by %u on %u", L"Interrupted: (Unknown Skill) by %u");
}

void ObserverStoC::handleAttackSkillStopped(uint32_t caster_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_attack_skill_stops) return;
    logActionCompletion(caster_id, L"Attack Skill Stopped: ID %u by %u on %u", L"Attack Skill Stopped: (Unknown Skill) by %u");
}

void ObserverStoC::handleAttackSkillFinished(uint32_t caster_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_attack_skill_finishes) return;
    logActionCompletion(caster_id, L"Attack Skill Finished: ID %u by %u on %u", L"Attack Skill Finished: (Unknown Skill) by %u");
}

void ObserverStoC::handleAttackFinished(uint32_t caster_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_basic_attack_finishes) return;
    logActionCompletion(caster_id, L"Attack Finished: by %u on %u", L"Attack Finished: by %u");
}

void ObserverStoC::handleDamage(uint32_t caster_id, uint32_t target_id, float value, uint32_t damage_type) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_damage) return;
    
    const wchar_t* damage_type_str = L"Normal";
    if (damage_type == static_cast<uint32_t>(GW::Packet::StoC::GenericValueID::critical)) {
        damage_type_str = L"Critical";
    } else if (damage_type == static_cast<uint32_t>(GW::Packet::StoC::GenericValueID::armorignoring)) {
        damage_type_str = L"Armor-ignoring";
    }
    
    wchar_t buffer[128];
    swprintf(buffer, 128, L"Damage (%ls): %f from %u to %u", damage_type_str, value, caster_id, target_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

void ObserverStoC::handleKnockdown(uint32_t cause_id, uint32_t target_id) {
    if (!owner || !owner->stoc_status) return;
    if (!owner->log_knockdowns) return;
    wchar_t buffer[128];
    // note: cause_id might not always be the direct cause, but it's the agent ID associated with the packet.
    swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"Knockdown: Target %u (Cause Agent: %u)", target_id, cause_id); 
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}

// convert JumboMessage value to a simple party index string for logging
const wchar_t* JumboValueToPartyStr(uint32_t value) {
    switch (value) {
        case GW::Packet::StoC::JumboMessageValue::PARTY_ONE: return L"Party 1";
        case GW::Packet::StoC::JumboMessageValue::PARTY_TWO: return L"Party 2";
        default: return L"Unknown Party";
    }
}

void ObserverStoC::handleJumboMessage(const GW::Packet::StoC::JumboMessage* packet) {
    if (!owner || !owner->stoc_status) return; 

    wchar_t buffer[128];
    const wchar_t* type_str = L"Unknown Type";
    const wchar_t* value_str = L""; // value might not apply to all types
    bool should_log = false; // flag to determine if this specific type should be logged

    // use GWCA constants and check corresponding log flag
    switch (packet->type) {
        case GW::Packet::StoC::JumboMessageType::BASE_UNDER_ATTACK:
            if (owner->log_jumbo_base_under_attack) { type_str = L"Base Under Attack"; should_log = true; }
            break;
        case GW::Packet::StoC::JumboMessageType::GUILD_LORD_UNDER_ATTACK:
            if (owner->log_jumbo_guild_lord_under_attack) { type_str = L"Guild Lord Under Attack"; should_log = true; }
            break;
        case GW::Packet::StoC::JumboMessageType::CAPTURED_SHRINE:
            if (owner->log_jumbo_captured_shrine) { type_str = L"Captured Shrine"; value_str = JumboValueToPartyStr(packet->value); should_log = true; }
            break;
        case GW::Packet::StoC::JumboMessageType::CAPTURED_TOWER:
            if (owner->log_jumbo_captured_tower) { type_str = L"Captured Tower"; value_str = JumboValueToPartyStr(packet->value); should_log = true; }
            break;
        case GW::Packet::StoC::JumboMessageType::PARTY_DEFEATED:
            if (owner->log_jumbo_party_defeated) { type_str = L"Party Defeated"; value_str = JumboValueToPartyStr(packet->value); should_log = true; }
            break;
        case GW::Packet::StoC::JumboMessageType::MORALE_BOOST:
            if (owner->log_jumbo_morale_boost) { type_str = L"Morale Boost"; value_str = JumboValueToPartyStr(packet->value); should_log = true; }
            break;
        case GW::Packet::StoC::JumboMessageType::VICTORY:
            if (owner->log_jumbo_victory) { type_str = L"Victory"; value_str = JumboValueToPartyStr(packet->value); should_log = true; }
            break;
        case GW::Packet::StoC::JumboMessageType::FLAWLESS_VICTORY:
            if (owner->log_jumbo_flawless_victory) { type_str = L"Flawless Victory"; value_str = JumboValueToPartyStr(packet->value); should_log = true; }
            break;
        default:
            if (owner->log_jumbo_unknown) { // check if unknown types should be logged
                swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"Jumbo Message: Unknown Type %u, Value %u", packet->type, packet->value);
                GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
            }
            return; // exit for unknown or unlogged types
    }

    // log only if the specific type's flag is enabled
    if (!should_log) {
        return;
    }

    // format message with type and value (if applicable)
    if (wcslen(value_str) > 0) {
        swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"Jumbo Message: %ls - %ls", type_str, value_str);
    } else {
        swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"Jumbo Message: %ls", type_str);
    }

    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, buffer);
}