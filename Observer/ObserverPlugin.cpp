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
    match_handler = new ObserverMatch(stoc_handler);
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

void ObserverPlugin::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    PLUGIN_LOAD_BOOL(stoc_status);
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

    PLUGIN_LOAD_BOOL(log_jumbo_base_under_attack);
    PLUGIN_LOAD_BOOL(log_jumbo_guild_lord_under_attack);
    PLUGIN_LOAD_BOOL(log_jumbo_captured_shrine);
    PLUGIN_LOAD_BOOL(log_jumbo_captured_tower);
    PLUGIN_LOAD_BOOL(log_jumbo_party_defeated);
    PLUGIN_LOAD_BOOL(log_jumbo_morale_boost);
    PLUGIN_LOAD_BOOL(log_jumbo_victory);
    PLUGIN_LOAD_BOOL(log_jumbo_flawless_victory);
    PLUGIN_LOAD_BOOL(log_jumbo_unknown);
}

void ObserverPlugin::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    PLUGIN_SAVE_BOOL(stoc_status);
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

    PLUGIN_SAVE_BOOL(log_jumbo_base_under_attack);
    PLUGIN_SAVE_BOOL(log_jumbo_guild_lord_under_attack);
    PLUGIN_SAVE_BOOL(log_jumbo_captured_shrine);
    PLUGIN_SAVE_BOOL(log_jumbo_captured_tower);
    PLUGIN_SAVE_BOOL(log_jumbo_party_defeated);
    PLUGIN_SAVE_BOOL(log_jumbo_morale_boost);
    PLUGIN_SAVE_BOOL(log_jumbo_victory);
    PLUGIN_SAVE_BOOL(log_jumbo_flawless_victory);
    PLUGIN_SAVE_BOOL(log_jumbo_unknown);
}

// draws the generic UI settings in the main settings panel
void ObserverPlugin::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings(); // draw base settings (visibility, etc.)
}

void ObserverPlugin::Initialize(
    ImGuiContext* ctx, 
    const ImGuiAllocFns allocator_fns, 
    const HMODULE toolbox_dll
)
{
    ToolboxUIPlugin::Initialize(ctx, allocator_fns, toolbox_dll); // initialize base first
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer Plugin initialized!"); 
    match_handler->RegisterCallbacks(); // register packet callbacks for match handler
}

void ObserverPlugin::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate(); // signal base first
    if (match_handler) {
        match_handler->RemoveCallbacks(); // this will now also handle removing stoc callbacks if needed
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
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OBSERVER MODE ACTIVE");
        } else {
            ImGui::TextDisabled("NOT OBSERVING");
        }
        ImGui::Separator();

        ImGui::Checkbox("Enable StoC Logging", &stoc_status);
        ImGui::SameLine(); 
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Master toggle for all logging options below.");
        }
        
        if (stoc_status)
        {
            ImGui::Separator();
            if (ImGui::CollapsingHeader("StoC Options"))
            {
                ImGui::Indent();

                auto AddLogCheckbox = [](const char* label, bool* v, const char* tooltip_text) {
                    ImGui::Checkbox(label, v);
                    ImGui::SameLine();
                    ImGui::TextDisabled("(?)");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip(tooltip_text);
                    }
                };

                if (ImGui::TreeNode("Skill Events")) {
                    AddLogCheckbox("Activations##Skill", &log_skill_activations, "Handler: ObserverStoC::handleSkillActivated\nPacket/Value ID: 0x56/0x55 with value 1 (skill_activated)");
                    AddLogCheckbox("Finishes##Skill", &log_skill_finishes, "Handler: ObserverStoC::handleSkillFinished\nPacket/Value ID: 0x55 with value 2 (skill_finished)");
                    AddLogCheckbox("Stops##Skill", &log_skill_stops, "Handler: ObserverStoC::handleSkillStopped\nPacket/Value ID: 0x56 with value 2 (skill_stopped)");
                    AddLogCheckbox("Instant Activations##Skill", &log_instant_skills, "Handler: ObserverStoC::handleInstantSkillActivated\nPacket/Value ID: 0x56 with value 5 (instant_skill_activated)");
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Attack Skill Events")) {
                     AddLogCheckbox("Activations##AttackSkill", &log_attack_skill_activations, "Handler: ObserverStoC::handleAttackSkillActivated\nPacket/Value ID: 0x56/0x55 with value 3 (attack_skill_activated)");
                     AddLogCheckbox("Finishes##AttackSkill", &log_attack_skill_finishes, "Handler: ObserverStoC::handleAttackSkillFinished\nPacket/Value ID: 0x55 with value 4 (attack_skill_finished)");
                     AddLogCheckbox("Stops##AttackSkill", &log_attack_skill_stops, "Handler: ObserverStoC::handleAttackSkillStopped\nPacket/Value ID: 0x56 with value 4 (attack_skill_stopped)");
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Basic Attack Events")) {
                    AddLogCheckbox("Starts##BasicAttack", &log_basic_attack_starts, "Handler: ObserverStoC::handleAttackStarted\nPacket/Value ID: 0x56/0x55 with value 13 (attack_started)");
                    AddLogCheckbox("Finishes##BasicAttack", &log_basic_attack_finishes, "Handler: ObserverStoC::handleAttackFinished\nPacket/Value ID: 0x55 with value 14 (melee_attack_finished)");
                    AddLogCheckbox("Stops##BasicAttack", &log_basic_attack_stops, "Handler: ObserverStoC::handleAttackStopped\nPacket/Value ID: 0x56 with value 14 (attack_stopped)");
                   ImGui::TreePop();
                }
                
                if (ImGui::TreeNode("Combat Events")) {
                    AddLogCheckbox("Damage", &log_damage, "Handler: ObserverStoC::handleDamage\nPacket/Type ID: 0x57 with type 1 (damage), 2 (critical), 3 (armorignoring)");
                    AddLogCheckbox("Interrupts", &log_interrupts, "Handler: ObserverStoC::handleInterrupted\nPacket/Value ID: 0x55 with value 15 (interrupted)");
                    AddLogCheckbox("Knockdowns", &log_knockdowns, "Handler: ObserverStoC::handleKnockdown\nPacket/Type ID: 0x57 with type 4 (knocked_down)");
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Agent Events")) {
                    AddLogCheckbox("Movement", &log_movement, "Handler: ObserverStoC::handleAgentMovement\nPacket Header: GAME_SMSG_AGENT_MOVE_TO_POINT (0x29)");
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Jumbo Messages")) {
                    AddLogCheckbox("Base Under Attack##Jumbo", &log_jumbo_base_under_attack,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Type: 0");
                    AddLogCheckbox("Guild Lord Under Attack##Jumbo", &log_jumbo_guild_lord_under_attack,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Type: 1");
                    AddLogCheckbox("Captured Shrine##Jumbo", &log_jumbo_captured_shrine,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Type: 3");
                    AddLogCheckbox("Captured Tower##Jumbo", &log_jumbo_captured_tower,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Type: 5");
                    AddLogCheckbox("Party Defeated##Jumbo", &log_jumbo_party_defeated,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Type: 6");
                    AddLogCheckbox("Morale Boost##Jumbo", &log_jumbo_morale_boost,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Type: 9");
                    AddLogCheckbox("Victory##Jumbo", &log_jumbo_victory,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Type: 16");
                    AddLogCheckbox("Flawless Victory##Jumbo", &log_jumbo_flawless_victory,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Type: 17");
                    AddLogCheckbox("Unknown Types##Jumbo", &log_jumbo_unknown,
                                   "Handler: ObserverStoC::handleJumboMessage\nPacket Header: 0x18F, Unknown Types");
                    ImGui::TreePop();
                }

                ImGui::Unindent();
            }
        }
    }
    ImGui::End();
}