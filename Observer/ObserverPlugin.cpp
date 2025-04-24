#include "ObserverPlugin.h"
#include "ObserverStoC.h"
#include "ObserverMatch.h"
#include "ObserverCapture.h"
#include "ObserverLoop.h"

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
#include <filesystem>
#include <fstream>
#include <ctime>
#include <cstdio>
#include <stringapiset.h> // For MultiByteToWideChar

ObserverPlugin::ObserverPlugin()
{
    stoc_handler = new ObserverStoC(this);
    match_handler = new ObserverMatch(stoc_handler);
    match_handler->SetOwnerPlugin(this);
    capture_handler = new ObserverCapture();
    loop_handler = new ObserverLoop(this);

    // generate an initial default folder name based on current time
    GenerateDefaultFolderName();

    // configure base UI plugin behavior
    can_show_in_main_window = true;
    can_close = true;
    show_menubutton = true;

    // register instance load callback to detect observer mode
    match_handler->RegisterCallbacks();
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
    if (capture_handler) {
        delete capture_handler;
        capture_handler = nullptr;
    }
    if (loop_handler) {
        delete loop_handler;
        loop_handler = nullptr;
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
    
    PLUGIN_LOAD_BOOL(auto_export_on_match_end);
    PLUGIN_LOAD_BOOL(auto_reset_name_on_match_end);
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
    
    PLUGIN_SAVE_BOOL(auto_export_on_match_end);
    PLUGIN_SAVE_BOOL(auto_reset_name_on_match_end);
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
    // initialize base ui plugin first
    ToolboxUIPlugin::Initialize(ctx, allocator_fns, toolbox_dll); 
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer Plugin initialized!"); 
    // register instance load callback to detect observer mode
    match_handler->RegisterCallbacks();
}

void ObserverPlugin::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate(); // signal base first
    if (match_handler) {
        match_handler->RemoveCallbacks(); // this will now also handle removing stoc callbacks if needed
    }
    if (loop_handler) {
        loop_handler->Stop(); // ensure agent loop is stopped
    }
}

// main draw function, called every frame
void ObserverPlugin::Draw(
    IDirect3DDevice9* /*pDevice*/
)
{
    // check visibility via base class
    bool* is_visible_ptr = GetVisiblePtr(); 
    if (!is_visible_ptr || !(*is_visible_ptr)) return; // exit if window not visible
    
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    
    // draw the observer window content
    if (ImGui::Begin(Name(), is_visible_ptr, GetWinFlags())) {
        // display observer status based on match handler flag
        bool is_observing = match_handler && match_handler->IsObserving();
        
        if (is_observing) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OBSERVER MODE ACTIVE");
        } else {
            ImGui::TextDisabled("NOT OBSERVING");
        }
        
        ImGui::Separator();

        if (ImGui::TreeNodeEx("Export Match", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            if (ImGui::Button("Export")) {
                if (strlen(export_folder_name) > 0) {
                    // convert char folder name to wchar_t
                    wchar_t wfoldername[sizeof(export_folder_name)]; // use stack buffer of same size
                    size_t converted = 0;
                    errno_t err = mbstowcs_s(&converted, wfoldername, sizeof(export_folder_name), export_folder_name, _TRUNCATE);

                    if (err == 0) {
                        ExportLogsToFolder(wfoldername);
                    } else {
                         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Error converting folder name for export.");
                    }
                } else {
                     GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Please enter a Match Name for export.");
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Exports all captured StoC events and Agent Loop snapshots for the current match to the specified 'captures/<Match Name>/' folder.");
            }
            
            // export folder name input 
            ImGui::InputText("Match Name", export_folder_name, sizeof(export_folder_name));
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Sets the name for the subfolder within 'captures/' where logs will be saved.");
            }

            ImGui::SameLine();
            // generate default folder name button
            if (ImGui::Button("Generate##FolderName")) { 
                GenerateDefaultFolderName(); // reset to default time-based name
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Generates a default Match Name based on the current date and time.");
            }

            // add Checkboxes for auto actions 
            ImGui::Checkbox("Auto Export on Match End", &auto_export_on_match_end);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If checked, automatically exports logs when observer mode ends.");
            }
            ImGui::Checkbox("Auto Reset Match Name on Match End", &auto_reset_name_on_match_end);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If checked, automatically generates a new default match name when observer mode ends.");
            }

            ImGui::Unindent();
            ImGui::TreePop(); 
        }

        // note about data persistence
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); 
        ImGui::TextWrapped("Note: Captured match data (StoC events & Agent states) remains in memory after observer mode ends. You can still export the previous match using the 'Export' button above. Data will be cleared automatically when a new observer session begins.");
        ImGui::PopStyleColor();
        ImGui::Separator();

        if (ImGui::TreeNodeEx("Capture Status", ImGuiTreeNodeFlags_DefaultOpen)) 
        {
            ImGui::Indent();

            // StoC Events Capture Status
            bool stoc_capturing = match_handler && match_handler->IsObserving();
            ImGui::Text("StoC Events Capture:"); ImGui::SameLine();
            if (stoc_capturing) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
            } else {
                ImGui::TextDisabled("Inactive");
            }
             if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Indicates if StoC events are being captured internally.\n(Triggered by entering observer mode)");
            }

            // agents states capture status
            bool loop_running = loop_handler && loop_handler->IsRunning();
            ImGui::Text("Agents States Capture:"); ImGui::SameLine(); // Renamed label
            if (loop_running) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Running");
            } else {
                ImGui::TextDisabled("Stopped");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Indicates if the Agents Loop thread is currently capturing agents states snapshots.\n(Triggered by entering observer mode)");
            }

            ImGui::Unindent();
            ImGui::TreePop(); 
        }
        
        ImGui::Separator();

        ImGui::Checkbox("Enable StoC logging chat", &stoc_status); 
        ImGui::SameLine(); 
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Master toggle for displaying StoC log messages in the game chat.\nAll events are still recorded internally for export regardless of this setting.");
        }
        
        if (stoc_status)
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
                AddLogCheckbox("Activations##Skill", &log_skill_activations, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillActivated\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Finishes##Skill", &log_skill_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillFinished\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Stops##Skill", &log_skill_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleSkillStopped\nMarker: [SKL]\nFile: skill_events.txt");
                AddLogCheckbox("Instant Activations##Skill", &log_instant_skills, "Chat Log Toggle\nHandler: ObserverStoC::handleInstantSkillActivated\nMarker: [SKL]\nFile: skill_events.txt");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Attack Skill Events")) {
                 AddLogCheckbox("Activations##AttackSkill", &log_attack_skill_activations, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillActivated\nMarker: [ASK]\nFile: attack_skill_events.txt");
                 AddLogCheckbox("Finishes##AttackSkill", &log_attack_skill_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillFinished\nMarker: [ASK]\nFile: attack_skill_events.txt");
                 AddLogCheckbox("Stops##AttackSkill", &log_attack_skill_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackSkillStopped\nMarker: [ASK]\nFile: attack_skill_events.txt");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Basic Attack Events")) {
                AddLogCheckbox("Starts##BasicAttack", &log_basic_attack_starts, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackStarted\nMarker: [ATK]\nFile: basic_attack_events.txt");
                AddLogCheckbox("Finishes##BasicAttack", &log_basic_attack_finishes, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackFinished\nMarker: [ATK]\nFile: basic_attack_events.txt");
                AddLogCheckbox("Stops##BasicAttack", &log_basic_attack_stops, "Chat Log Toggle\nHandler: ObserverStoC::handleAttackStopped\nMarker: [ATK]\nFile: basic_attack_events.txt");
               ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Combat Events")) {
                AddLogCheckbox("Damage", &log_damage, "Chat Log Toggle\nHandler: ObserverStoC::handleDamage\nMarker: [CMB]\nFile: combat_events.txt");
                AddLogCheckbox("Interrupts", &log_interrupts, "Chat Log Toggle\nHandler: ObserverStoC::handleInterrupted\nMarker: [CMB]\nFile: combat_events.txt");
                AddLogCheckbox("Knockdowns", &log_knockdowns, "Chat Log Toggle\nHandler: ObserverStoC::handleKnockdown\nMarker: [CMB]\nFile: combat_events.txt");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Agent Events")) {
                AddLogCheckbox("Movement", &log_movement, "Chat Log Toggle\nHandler: ObserverStoC::handleAgentMovement\nMarker: [AGT]\nFile: agent_events.txt");
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Jumbo Messages")) {
                AddLogCheckbox("Base Under Attack##Jumbo", &log_jumbo_base_under_attack, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 0\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Guild Lord Under Attack##Jumbo", &log_jumbo_guild_lord_under_attack, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 1\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Captured Shrine##Jumbo", &log_jumbo_captured_shrine, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 3\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Captured Tower##Jumbo", &log_jumbo_captured_tower, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 5\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Party Defeated##Jumbo", &log_jumbo_party_defeated, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 6\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Morale Boost##Jumbo", &log_jumbo_morale_boost, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 9\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Victory##Jumbo", &log_jumbo_victory, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 16\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Flawless Victory##Jumbo", &log_jumbo_flawless_victory, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nType: 17\nMarker: [JMB]\nFile: jumbo_messages.txt");
                AddLogCheckbox("Unknown Types##Jumbo", &log_jumbo_unknown, "Chat Log Toggle\nHandler: ObserverStoC::handleJumboMessage\nUnknown Types\nMarker: [JMB]\nFile: jumbo_messages.txt");
                ImGui::TreePop();
            }

            ImGui::Unindent();
        }
    }
    ImGui::End();
}

void ObserverPlugin::HandleMatchEnd() {
    // function called by ObserverMatch when observer mode ends
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer mode ended.");

    // check if auto-export is enabled
    if (auto_export_on_match_end) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-export triggered...");
        if (strlen(export_folder_name) > 0) {
            // convert char folder name to wchar_t
            wchar_t wfoldername[sizeof(export_folder_name)]; // use stack buffer of same size
            size_t converted = 0;
            errno_t err = mbstowcs_s(&converted, wfoldername, sizeof(export_folder_name), export_folder_name, _TRUNCATE); // convert char folder name to wchar_t

            if (err == 0) {
                if (ExportLogsToFolder(wfoldername)) { // export logs to folder
                     wchar_t msg[512];
                     swprintf_s(msg, L"Successfully auto-exported logs to 'captures/%ls'.", wfoldername); 
                     GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg);
                } else {
                     wchar_t msg[512];
                     swprintf_s(msg, L"Auto-export failed. Check previous errors. Folder was 'captures/%ls'.", wfoldername);
                     GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg);
                }
            } else {
                 GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Error converting folder name for auto-export.");
            }
        } else {
             GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-export skipped: Match Name is empty.");
        }
    } else {
         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-export disabled.");
    }

    // check if auto-reset name is enabled
    if (auto_reset_name_on_match_end) {
        GenerateDefaultFolderName(); // generate default folder name
        wchar_t msg[512];
        swprintf_s(msg, L"Auto-resetting Match Name to '%hs'.", export_folder_name);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg);
    } else {
         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-reset name disabled.");
    }
}