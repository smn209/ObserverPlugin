#include "ObserverPlugin.h"
#include "ObserverStoC.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>

#include <Utils/GuiUtils.h>
#include <imgui.h>

ObserverPlugin::ObserverPlugin()
{
    stoc_handler = new ObserverStoC(this);
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static ObserverPlugin instance; // create a static instance of the plugin
    return &instance; // return the instance
}

void ObserverPlugin::LoadSettings(
    const wchar_t* folder
)
{
    // settings loaded from the settings file
    ToolboxPlugin::LoadSettings(folder);
    PLUGIN_LOAD_BOOL(enabled);
    PLUGIN_LOAD_BOOL(log_skill_activations);
    PLUGIN_LOAD_BOOL(log_skill_finishes);
    PLUGIN_LOAD_BOOL(log_skill_stops);
    PLUGIN_LOAD_BOOL(log_attack_skill_activations);
    PLUGIN_LOAD_BOOL(log_attack_skill_finishes);
    PLUGIN_LOAD_BOOL(log_attack_skill_stops);
    PLUGIN_LOAD_BOOL(log_basic_attack_starts);
    PLUGIN_LOAD_BOOL(log_basic_attack_finishes);
    PLUGIN_LOAD_BOOL(log_basic_attack_stops);
    PLUGIN_LOAD_BOOL(log_interrupts);
    PLUGIN_LOAD_BOOL(log_instant_skills);
    PLUGIN_LOAD_BOOL(log_damage);
    PLUGIN_LOAD_BOOL(log_knockdowns);
    PLUGIN_LOAD_BOOL(log_movement);
    ToolboxPlugin::SaveSettings(folder);
}

void ObserverPlugin::SaveSettings(
    const wchar_t* folder
)
{
    // save the settings to the settings file
    PLUGIN_SAVE_BOOL(enabled);
    PLUGIN_SAVE_BOOL(log_skill_activations);
    PLUGIN_SAVE_BOOL(log_skill_finishes);
    PLUGIN_SAVE_BOOL(log_skill_stops);
    PLUGIN_SAVE_BOOL(log_attack_skill_activations);
    PLUGIN_SAVE_BOOL(log_attack_skill_finishes);
    PLUGIN_SAVE_BOOL(log_attack_skill_stops);
    PLUGIN_SAVE_BOOL(log_basic_attack_starts);
    PLUGIN_SAVE_BOOL(log_basic_attack_finishes);
    PLUGIN_SAVE_BOOL(log_basic_attack_stops);
    PLUGIN_SAVE_BOOL(log_interrupts);
    PLUGIN_SAVE_BOOL(log_instant_skills);
    PLUGIN_SAVE_BOOL(log_damage);
    PLUGIN_SAVE_BOOL(log_knockdowns);
    PLUGIN_SAVE_BOOL(log_movement);
    ToolboxPlugin::SaveSettings(folder);
}

void ObserverPlugin::DrawSettings()
{
    if (!toolbox_handle)
    {
        return;
    } // if the toolbox handle is not valid, return (case possible : plugin not loaded)
    
    ImGui::Checkbox("Enable Skill Logging", &enabled);
    ImGui::SameLine(); 
    ImGui::Text("(Master Toggle)");
    
    if (enabled)
    {
        ImGui::Indent();
        ImGui::Text("Log Options:");
        
        ImGui::Checkbox("Skill Activations", &log_skill_activations);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleSkillActivated\nPacket/Value ID: 0x56/0x55 with value 1 (skill_activated)");
        }
        ImGui::Checkbox("Skill Finishes", &log_skill_finishes);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleSkillFinished\nPacket/Value ID: 0x55 with value 2 (skill_finished)");
        }
        ImGui::Checkbox("Skill Stops", &log_skill_stops);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleSkillStopped\nPacket/Value ID: 0x56 with value 2 (skill_stopped)");
        }
        ImGui::Checkbox("Attack Skill Activations", &log_attack_skill_activations);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleAttackSkillActivated\nPacket/Value ID: 0x56/0x55 with value 3 (attack_skill_activated)");
        }
        ImGui::Checkbox("Attack Skill Finishes", &log_attack_skill_finishes);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleAttackSkillFinished\nPacket/Value ID: 0x55 with value 4 (attack_skill_finished)");
        }
        ImGui::Checkbox("Attack Skill Stops", &log_attack_skill_stops);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleAttackSkillStopped\nPacket/Value ID: 0x56 with value 4 (attack_skill_stopped)");
        }
        ImGui::Checkbox("Basic Attack Starts", &log_basic_attack_starts);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleAttackStarted\nPacket/Value ID: 0x56/0x55 with value 13 (attack_started)");
        }
        ImGui::Checkbox("Basic Attack Finishes", &log_basic_attack_finishes);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleAttackFinished\nPacket/Value ID: 0x55 with value 14 (melee_attack_finished)");
        }
        ImGui::Checkbox("Basic Attack Stops", &log_basic_attack_stops);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleAttackStopped\nPacket/Value ID: 0x56 with value 14 (attack_stopped)");
        }
        ImGui::Checkbox("Interrupts", &log_interrupts);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleInterrupted\nPacket/Value ID: 0x55 with value 15 (interrupted)");
        }
        ImGui::Checkbox("Instant Skills", &log_instant_skills);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleInstantSkillActivated\nPacket/Value ID: 0x56 with value 5 (instant_skill_activated)");
        }
        ImGui::Checkbox("Damage Events", &log_damage);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleDamage\nPacket/Type ID: 0x57 with type 1 (damage), 2 (critical), 3 (armorignoring)");
        }
        ImGui::Checkbox("Knockdowns", &log_knockdowns);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleKnockdown\nPacket/Type ID: 0x57 with type 4 (knocked_down)");
        }
        ImGui::Checkbox("Agent Movement", &log_movement);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Handler: ObserverStoC::handleAgentMovement\nPacket Header: GAME_SMSG_AGENT_MOVE_TO_POINT (0x29)");
        }
        ImGui::Unindent();
    }
}

void ObserverPlugin::Initialize(
    ImGuiContext* ctx, 
    const ImGuiAllocFns allocator_fns, 
    const HMODULE toolbox_dll
)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll); // initialize the plugin (from ToolboxPlugin)
    GW::Chat::WriteChat( 
        GW::Chat::CHANNEL_MODERATOR, 
        L"Observer Plugin initialized! (by simon1337482)"
    ); // initialized plugin message
    stoc_handler->RegisterCallbacks(); // register the callbacks (from ObserverStoC)
}

void ObserverPlugin::SignalTerminate()
{
    // this occurs when the plugin is terminated (from ToolboxPlugin)
    ToolboxPlugin::SignalTerminate(); // signal the termination (from ToolboxPlugin)
    stoc_handler->RemoveCallbacks(); // remove the callbacks (from ObserverStoC)
    
    delete stoc_handler; // delete the stoc handler
}

bool ObserverPlugin::CanTerminate()
{
    // plugin can always terminate 
    return true;
}

void ObserverPlugin::Draw(
    IDirect3DDevice9* /*pDevice*/
)
{
    // this seems to be called every frame (from ToolboxPlugin)
}