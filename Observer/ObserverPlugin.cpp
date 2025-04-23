#include "ObserverPlugin.h"
#include "ObserverStoC.h"
#include "ObserverMatch.h"

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
    match_handler = new ObserverMatch();
    // configure base UI plugin behavior
    can_show_in_main_window = true;
    can_close = true;
    show_menubutton = true;
}

// destructor needs to be defined if we manually delete handlers
ObserverPlugin::~ObserverPlugin()
{
    if (stoc_handler) {
        delete stoc_handler;
        stoc_handler = nullptr;
    }
    if (match_handler) {
        delete match_handler;
        match_handler = nullptr;
    }
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static ObserverPlugin instance;
    return &instance;
}

void ObserverPlugin::LoadSettings(
    const wchar_t* folder
)
{
    ToolboxUIPlugin::LoadSettings(folder); // load base settings (visibility, etc.)
    
    // load observer specific settings
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
}

void ObserverPlugin::SaveSettings(
    const wchar_t* folder
)
{
    ToolboxUIPlugin::SaveSettings(folder); // save base settings

    // save observer specific settings
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
}

// draws the generic UI settings in the main settings panel
void ObserverPlugin::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings(); // draw base settings (lock position, size, etc.)
}

void ObserverPlugin::Initialize(
    ImGuiContext* ctx, 
    const ImGuiAllocFns allocator_fns, 
    const HMODULE toolbox_dll
)
{
    ToolboxUIPlugin::Initialize(ctx, allocator_fns, toolbox_dll); // initialize base first
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer Plugin initialized!"); 
    stoc_handler->RegisterCallbacks(); // register packet callbacks for main handler
    match_handler->RegisterCallbacks(); // register packet callbacks for match handler
}

void ObserverPlugin::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate(); // signal base first
    if (stoc_handler) { 
        stoc_handler->RemoveCallbacks(); // remove packet callbacks for main handler
    }
    if (match_handler) {
        match_handler->RemoveCallbacks();
    }
}

// main draw function, called every frame
void ObserverPlugin::Draw(
    IDirect3DDevice9* /*pDevice*/
)
{
    bool* is_visible_ptr = GetVisiblePtr(); // get visibility state from base
    if (!is_visible_ptr || !(*is_visible_ptr)) return; // exit if not visible
    
    ImGui::SetNextWindowSize(ImVec2(300, 450), ImGuiCond_FirstUseEver);
    
    // draw the observer window content
    if (ImGui::Begin(Name(), is_visible_ptr, GetWinFlags())) {
        // display observer status
        if (match_handler && match_handler->IsObserving()) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "OBSERVER MODE ACTIVE");
            ImGui::Separator();
        }else{
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "NOT OBSERVING");
        }

        ImGui::Checkbox("Enable Skill Logging", &enabled);
        ImGui::SameLine(); 
        ImGui::TextDisabled("(Master Toggle)");
        
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
    ImGui::End();
}