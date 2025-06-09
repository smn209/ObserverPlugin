#include "ObserverStoC.h"
#include "ObserverPlugin.h"
#include "ObserverMatch.h" 

#include <GWCA/Constants/Constants.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/MapMgr.h> 
#include <cstdio> 
#include <string> 

// define markers for categorization (declarations are in ObserverStoC.h)
const wchar_t* MARKER_SKILL_EVENT = L"[SKL] ";
const size_t MARKER_SKILL_EVENT_LEN = wcslen(MARKER_SKILL_EVENT);
const wchar_t* MARKER_ATTACK_SKILL_EVENT = L"[ASK] ";
const size_t MARKER_ATTACK_SKILL_EVENT_LEN = wcslen(MARKER_ATTACK_SKILL_EVENT);
const wchar_t* MARKER_BASIC_ATTACK_EVENT = L"[ATK] ";
const size_t MARKER_BASIC_ATTACK_EVENT_LEN = wcslen(MARKER_BASIC_ATTACK_EVENT);
const wchar_t* MARKER_COMBAT_EVENT = L"[CMB] ";
const size_t MARKER_COMBAT_EVENT_LEN = wcslen(MARKER_COMBAT_EVENT);
const wchar_t* MARKER_AGENT_EVENT = L"[AGT] ";
const size_t MARKER_AGENT_EVENT_LEN = wcslen(MARKER_AGENT_EVENT);
const wchar_t* MARKER_JUMBO_EVENT = L"[JMB] ";
const size_t MARKER_JUMBO_EVENT_LEN = wcslen(MARKER_JUMBO_EVENT);

// convert JumboMessage value to a simple party index string for logging
const wchar_t* JumboValueToPartyStr(uint32_t value) {
    switch (value) {
        case GW::Packet::StoC::JumboMessageValue::PARTY_ONE: return L"Party 1";
        case GW::Packet::StoC::JumboMessageValue::PARTY_TWO: return L"Party 2";
        default: return L"Unknown Party";
    }
}

// ==================== Public Methods ====================

ObserverStoC::ObserverStoC(ObserverPlugin* owner_plugin) : owner(owner_plugin) {
}

ObserverStoC::~ObserverStoC() {
    cleanupAgentActions();
}

void ObserverStoC::RegisterCallbacks() {
    // GenericModifier (0x57) (Damage, Knockdown, etc.)
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::GenericModifier>(
        &GenericModifier_Entry,
        [this](const GW::HookStatus*, const GW::Packet::StoC::GenericModifier* packet) -> void {
            if (!owner) return; 

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
            if (!owner) return; // Only check if owner exists

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
            if (!owner) return; // Only check if owner exists

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
            if (!owner) return; // Only check if owner exists

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
            if (!owner) return; // Only check if owner exists

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
            if (!owner) return; // Only check if owner exists
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

// ==================== Private Helper Methods ====================

void ObserverStoC::cleanupAgentActions() {
    // helper function to iterate through the map and delete allocated action info pointers
    for (auto const& [caster_id, action_info_ptr] : agent_active_action) {
        if (action_info_ptr) {
            delete action_info_ptr;
        }
    }
    agent_active_action.clear();
}

void ObserverStoC::logActionActivation(uint32_t caster_id, uint32_t target_id, uint32_t skill_id,
                                        bool no_target, const wchar_t* action_identifier,
                                        const wchar_t* category_marker)
{
    if (!owner || !owner->match_handler) return; // ensure owner and match_handler exist

    // logs the start of an action (skill activation or basic attack start)
    // handles the potential swap of caster/target ids depending on packet type
    // also stores the action details in agent_active_action map for later reference (e.g., completion)

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

    // add skills used to match info
    if (stored_skill_id != 0) { 
        owner->match_handler->GetMatchInfo().AddSkillUsed(actual_caster_id, stored_skill_id);
    }

    // format log message using the new semicolon format
    wchar_t message_buffer[128];
    if (wcscmp(action_identifier, L"ATTACK_STARTED") == 0) {
         swprintf(message_buffer, sizeof(message_buffer)/sizeof(wchar_t), L"%ls;%u;%u", action_identifier, actual_caster_id, actual_target_id);
    } else { // skill activation (attack skill or normal skill)
        swprintf(message_buffer, sizeof(message_buffer)/sizeof(wchar_t), L"%ls;%u;%u;%u", action_identifier, stored_skill_id, actual_caster_id, actual_target_id);
    }

    std::wstring log_entry = category_marker;
    log_entry += message_buffer;
    owner->AddLogEntry(log_entry.c_str()); // add the entry with the marker

    // write to chat only if enabled and the specific log type is enabled
    if (owner->stoc_status) {
        // check specific log type before writing to chat
        if ((stored_skill_id == 0 && owner->log_basic_attack_starts) || // attack_started
            (wcscmp(action_identifier, L"SKILL_ACTIVATED") == 0 && owner->log_skill_activations) ||
            (wcscmp(action_identifier, L"ATTACK_SKILL_ACTIVATED") == 0 && owner->log_attack_skill_activations))
        {
            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, message_buffer);
        }
    }
}

void ObserverStoC::logActionCompletion(uint32_t caster_id, const wchar_t* action_identifier,
                                      uint32_t value_id,
                                      const wchar_t* category_marker)
{
    if (!owner) return;
    wchar_t message_buffer[128];

    uint32_t skill_id = 0; // default to 0 if not found
    uint32_t target_id = 0; // default to 0 if not found

    auto it = agent_active_action.find(caster_id);
    if (it != agent_active_action.end()) {
        // found stored action info for this caster
        ActiveActionInfo* action_info = it->second;
        skill_id = action_info->skill_id;
        target_id = action_info->target_id;

        // clean up the stored action info
        delete action_info;
        agent_active_action.erase(it);
    }
    // note: for stops/interrupts, we proceed even if not found, logging only the caster_id

    // format message based on the action type identifier
    if (wcscmp(action_identifier, L"SKILL_FINISHED") == 0 ||
        wcscmp(action_identifier, L"ATTACK_SKILL_FINISHED") == 0 ||
        wcscmp(action_identifier, L"ATTACK_FINISHED") == 0)
    {
        // finishes include skill and target info (if available)
        swprintf(message_buffer, sizeof(message_buffer)/sizeof(wchar_t), L"%ls;%u;%u;%u", action_identifier, caster_id, skill_id, target_id);
    } else {
        // stops and interrupts now include skill/target info if available
        swprintf(message_buffer, sizeof(message_buffer)/sizeof(wchar_t), L"%ls;%u;%u;%u", action_identifier, caster_id, skill_id, target_id);
    }

    std::wstring log_entry = category_marker;
    log_entry += message_buffer;
    owner->AddLogEntry(log_entry.c_str()); // add the entry with the marker

    // write to chat only if enabled and the specific log type is enabled
    if (owner->stoc_status) {
        bool should_log_chat = false;
        switch(value_id) { // use the original packet value_id to check the toggle
            case GW::Packet::StoC::GenericValueID::melee_attack_finished: should_log_chat = owner->log_basic_attack_finishes; break;
            case GW::Packet::StoC::GenericValueID::attack_stopped:        should_log_chat = owner->log_basic_attack_stops; break;
            case GW::Packet::StoC::GenericValueID::interrupted:             should_log_chat = owner->log_interrupts; break;
            case GW::Packet::StoC::GenericValueID::attack_skill_finished: should_log_chat = owner->log_attack_skill_finishes; break;
            case GW::Packet::StoC::GenericValueID::attack_skill_stopped:    should_log_chat = owner->log_attack_skill_stops; break;
            case GW::Packet::StoC::GenericValueID::skill_finished:          should_log_chat = owner->log_skill_finishes; break;
            case GW::Packet::StoC::GenericValueID::skill_stopped:           should_log_chat = owner->log_skill_stops; break;
        }

        if (should_log_chat) {
            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, message_buffer);
        }
    }
}

// ==================== Packet Dispatch Handlers ====================

void ObserverStoC::handleGenericPacket(const uint32_t value_id, const uint32_t caster_id,
                                       const uint32_t target_id, const float value, const bool)
{
    // dispatches packets containing float values (damage, knockdown) to specific handlers
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
    // dispatches packets containing uint32_t values (skills, attacks, interrupts) to specific handlers
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

// ==================== Specific Event Handlers ====================

// ---- Skill Handlers ----
void ObserverStoC::handleSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id, bool no_target) {
    if (!owner) return;
    // format: skill_activated;skill_id;caster_id;target_id
    logActionActivation(caster_id, target_id, skill_id, no_target, L"SKILL_ACTIVATED", MARKER_SKILL_EVENT);
}

void ObserverStoC::handleSkillFinished(uint32_t caster_id) {
    if (!owner) return;
    // format: skill_finished;caster_id;skill_id;target_id
    logActionCompletion(caster_id, L"SKILL_FINISHED", GW::Packet::StoC::GenericValueID::skill_finished, MARKER_SKILL_EVENT);
}

void ObserverStoC::handleSkillStopped(uint32_t caster_id) {
    if (!owner) return;
    // format: skill_stopped;caster_id
    logActionCompletion(caster_id, L"SKILL_STOPPED", GW::Packet::StoC::GenericValueID::skill_stopped, MARKER_SKILL_EVENT);
}

// ---- Attack Skill Handlers ----
void ObserverStoC::handleAttackSkillActivated(uint32_t caster_id, uint32_t target_id, uint32_t skill_id, bool no_target) {
    if (!owner) return;
    // format: attack_skill_activated;skill_id;caster_id;target_id
    logActionActivation(caster_id, target_id, skill_id, no_target, L"ATTACK_SKILL_ACTIVATED", MARKER_ATTACK_SKILL_EVENT);
}

void ObserverStoC::handleAttackSkillFinished(uint32_t caster_id) {
    if (!owner) return;
    // format: attack_skill_finished;caster_id;skill_id;target_id
    logActionCompletion(caster_id, L"ATTACK_SKILL_FINISHED", GW::Packet::StoC::GenericValueID::attack_skill_finished, MARKER_ATTACK_SKILL_EVENT);
}

void ObserverStoC::handleAttackSkillStopped(uint32_t caster_id) {
    if (!owner) return;
    // format: attack_skill_stopped;caster_id
    logActionCompletion(caster_id, L"ATTACK_SKILL_STOPPED", GW::Packet::StoC::GenericValueID::attack_skill_stopped, MARKER_ATTACK_SKILL_EVENT);
}

// ---- Instant Skill Handler ----
void ObserverStoC::handleInstantSkillActivated(uint32_t caster_id, uint32_t skill_id) {
    if (!owner || !owner->match_handler) return;

    // instant skills don't store state in agent_active_action. target is usually the caster.
    wchar_t message_buffer[128];
    // format: instant_skill_used;skill_id;caster_id;target_id (target=caster)
    swprintf(message_buffer, sizeof(message_buffer)/sizeof(wchar_t), L"INSTANT_SKILL_USED;%u;%u;%u", skill_id, caster_id, caster_id);

    // add skills used to match info
    if (skill_id != 0) { // check skill_id validity
        owner->match_handler->GetMatchInfo().AddSkillUsed(caster_id, skill_id);
    }

    std::wstring log_entry = MARKER_SKILL_EVENT;
    log_entry += message_buffer;
    owner->AddLogEntry(log_entry.c_str());

    // only display in chat if enabled
    if (owner->stoc_status && owner->log_instant_skills) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, message_buffer);
    }
}

// ---- Basic Attack Handlers ----
void ObserverStoC::handleAttackStarted(uint32_t caster_id, uint32_t target_id, bool no_target) {
    if (!owner) return;
    // format: attack_started;caster_id;target_id
    // use attack_started value_id as a placeholder skill_id to trigger basic attack storage
    logActionActivation(caster_id, target_id, static_cast<uint32_t>(GW::Packet::StoC::GenericValueID::attack_started), no_target, L"ATTACK_STARTED", MARKER_BASIC_ATTACK_EVENT);
}

void ObserverStoC::handleAttackFinished(uint32_t caster_id) {
    if (!owner) return;
    // format: attack_finished;caster_id;skill_id;target_id (skill_id will be 0)
    logActionCompletion(caster_id, L"ATTACK_FINISHED", GW::Packet::StoC::GenericValueID::melee_attack_finished, MARKER_BASIC_ATTACK_EVENT);
}

void ObserverStoC::handleAttackStopped(uint32_t caster_id) {
    if (!owner) return;
    // format: attack_stopped;caster_id
    logActionCompletion(caster_id, L"ATTACK_STOPPED", GW::Packet::StoC::GenericValueID::attack_stopped, MARKER_BASIC_ATTACK_EVENT);
}

// ---- Combat Event Handlers ----
void ObserverStoC::handleInterrupted(uint32_t caster_id) {
    if (!owner) return;
    // format: interrupted;caster_id
    logActionCompletion(caster_id, L"INTERRUPTED", GW::Packet::StoC::GenericValueID::interrupted, MARKER_COMBAT_EVENT);
}

void ObserverStoC::handleDamage(uint32_t caster_id, uint32_t target_id, float value, uint32_t damage_type) {
    if (!owner) return;

    // map gwca damage type id to a simple integer if needed, or just log the raw id.
    // using raw id for simplicity: 1=normal, 2=crit, 3=armorignoring

    wchar_t message_buffer[128];
    // format: damage;caster_id;target_id;value;damage_type_id
    swprintf(message_buffer, 128, L"DAMAGE;%u;%u;%f;%u", caster_id, target_id, value, damage_type);

    std::wstring log_entry = MARKER_COMBAT_EVENT;
    log_entry += message_buffer;
    owner->AddLogEntry(log_entry.c_str());

    // write to chat only if enabled
    if (owner->stoc_status && owner->log_damage) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, message_buffer);
    }
}

void ObserverStoC::handleKnockdown(uint32_t cause_id, uint32_t target_id) {
    if (!owner) return;

    wchar_t message_buffer[128];

    // format: knocked_down;target_id;cause_id
    // note: cause_id might not always be the direct cause, but it's the agent id associated with the packet.
    swprintf(message_buffer, sizeof(message_buffer)/sizeof(wchar_t), L"KNOCKED_DOWN;%u;%u", target_id, cause_id);

    std::wstring log_entry = MARKER_COMBAT_EVENT;
    log_entry += message_buffer;
    owner->AddLogEntry(log_entry.c_str());

    // write to chat only if enabled
    if (owner->stoc_status && owner->log_knockdowns) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, message_buffer);
    }
}

// ---- Agent Event Handlers ----
void ObserverStoC::handleAgentMovement(uint32_t agent_id, float x, float y, uint16_t plane) {
    if (!owner) return;

    wchar_t message_buffer[128];
    // format: game_smsg_agent_move_to_point;agent_id;x;y;plane
    swprintf(message_buffer, sizeof(message_buffer)/sizeof(wchar_t), L"GAME_SMSG_AGENT_MOVE_TO_POINT;%u;%.2f;%.2f;%u", agent_id, x, y, plane);

    std::wstring log_entry = MARKER_AGENT_EVENT;
    log_entry += message_buffer;
    owner->AddLogEntry(log_entry.c_str());

    // write to chat only if enabled
    if (owner->stoc_status && owner->log_movement) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, message_buffer);
    }
}

// ---- Jumbo Message Handler ----
void ObserverStoC::handleJumboMessage(const GW::Packet::StoC::JumboMessage* packet) {
    if (!owner || !owner->match_handler) return; 

    wchar_t message_buffer[128]; // for the visible message part
    wchar_t log_buffer[192]; // larger buffer for marker + message
    bool should_log_to_chat = false; // flag to determine if this specific type should be logged to chat

    // format: game_smsg_jumbo_message;type;value
    swprintf(message_buffer, sizeof(message_buffer)/sizeof(wchar_t), L"GAME_SMSG_JUMBO_MESSAGE;%u;%u", packet->type, packet->value);

    bool is_victory_message = false;

    // check corresponding log flag for chat output
    switch (packet->type) {
        case GW::Packet::StoC::JumboMessageType::BASE_UNDER_ATTACK:           should_log_to_chat = owner->log_jumbo_base_under_attack; break;
        case GW::Packet::StoC::JumboMessageType::GUILD_LORD_UNDER_ATTACK:     should_log_to_chat = owner->log_jumbo_guild_lord_under_attack; break;
        case GW::Packet::StoC::JumboMessageType::CAPTURED_SHRINE:             should_log_to_chat = owner->log_jumbo_captured_shrine; break;
        case GW::Packet::StoC::JumboMessageType::CAPTURED_TOWER:              should_log_to_chat = owner->log_jumbo_captured_tower; break;
        case GW::Packet::StoC::JumboMessageType::PARTY_DEFEATED:              should_log_to_chat = owner->log_jumbo_party_defeated; break;
        case GW::Packet::StoC::JumboMessageType::MORALE_BOOST:                should_log_to_chat = owner->log_jumbo_morale_boost; break;
        case GW::Packet::StoC::JumboMessageType::VICTORY:
            should_log_to_chat = owner->log_jumbo_victory;
            is_victory_message = true;
            break;
        case GW::Packet::StoC::JumboMessageType::FLAWLESS_VICTORY:
            should_log_to_chat = owner->log_jumbo_flawless_victory;
            is_victory_message = true;
            break;
        default:                                                            should_log_to_chat = owner->log_jumbo_unknown; break;
    }

    // capture match end info if it's a victory message
    if (is_victory_message) {
        uint32_t winner_id = packet->value; // value indicates the winning party (raw ID)

        owner->HandleMatchEndSignal(winner_id);
    }

    // prepend marker and add to internal log
    // use the helper function defined at the top of the file
    swprintf(log_buffer, sizeof(log_buffer)/sizeof(wchar_t), L"%ls%ls (%ls)", MARKER_JUMBO_EVENT, message_buffer, JumboValueToPartyStr(packet->value)); // example usage if needed
    owner->AddLogEntry(log_buffer); // log the raw message as before

    // write to chat only if enabled and the specific type is toggled on
    if (owner->stoc_status && should_log_to_chat) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, message_buffer);
    }
}