#include "ObserverPlugin.h"
#include "ObserverStoC.h"
#include "ObserverCapture.h"
#include "ObserverLoop.h"
#include "ObserverMatchData.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/ChatMgr.h>
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
#include <stringapiset.h>
#include <string>
#include <cstdint>

#include <windows.h>
#include <stdio.h>
#include <vector>      
#include <algorithm>   
#include <iomanip> 
#include <sstream>
#include <mutex>

#include <GWCA/GameEntities/Guild.h>
#include <GWCA/Context/CharContext.h>

#include <Modules/Resources.h>
#include <Utils/TextUtils.h>

#include "Windows/MatchCompositionsSettingsWindow.h"

ObserverPlugin::ObserverPlugin() :
    obs_show_match_ids(true),
    obs_show_map_id(true),
    obs_show_age(true),
    obs_show_type(true),
    obs_show_count(true),
    obs_show_match_unknowns(true),
    obs_show_team1_name_dup(true),
    obs_show_h007c_array(false), 
    obs_show_team_id(true),
    obs_show_team_name(true),
    obs_show_team_unknowns(true),
    obs_show_cape_colors(true),
    obs_show_cape_design(true),
    auto_reset_name_on_match_end(true),
    show_match_compositions_window(false),
    show_match_compositions_settings_window(false) 
{
    stoc_handler = new ObserverStoC(this);
    match_handler = new ObserverMatch(stoc_handler);
    match_handler->SetOwnerPlugin(this);
    capture_handler = new ObserverCapture();
    loop_handler = new ObserverLoop(this, match_handler);

    match_compositions_settings_window_ = new MatchCompositionsSettingsWindow();

    // generate an initial default folder name based on current time
    GenerateDefaultFolderName();

    // configure base UI plugin behavior
    can_show_in_main_window = true;
    can_close = true;
    show_menubutton = true;

    // register instance load callback to detect observer mode
    match_handler->RegisterCallbacks();

    PLUGIN_LOAD_BOOL(obs_show_match_ids);
    PLUGIN_LOAD_BOOL(obs_show_map_id);
    PLUGIN_LOAD_BOOL(obs_show_age);
    PLUGIN_LOAD_BOOL(obs_show_type);
    PLUGIN_LOAD_BOOL(obs_show_count);
    PLUGIN_LOAD_BOOL(obs_show_match_unknowns);
    PLUGIN_LOAD_BOOL(obs_show_team1_name_dup);
    PLUGIN_LOAD_BOOL(obs_show_h007c_array);
    PLUGIN_LOAD_BOOL(obs_show_team_id);
    PLUGIN_LOAD_BOOL(obs_show_team_name);
    PLUGIN_LOAD_BOOL(obs_show_team_unknowns);
    PLUGIN_LOAD_BOOL(obs_show_cape_colors);
    PLUGIN_LOAD_BOOL(obs_show_cape_design);
    PLUGIN_LOAD_BOOL(show_match_compositions_window);
    PLUGIN_LOAD_BOOL(show_match_compositions_settings_window);
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
    if (match_compositions_settings_window_) {
        delete match_compositions_settings_window_;
        match_compositions_settings_window_ = nullptr;
    }
}

void ObserverPlugin::LoadSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    PLUGIN_LOAD_BOOL(show_capture_status_window);
    PLUGIN_LOAD_BOOL(show_live_party_info_window);
    PLUGIN_LOAD_BOOL(show_live_guild_info_window);
    PLUGIN_LOAD_BOOL(show_available_matches_window);
    PLUGIN_LOAD_BOOL(show_stoc_log_window);
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

    PLUGIN_LOAD_BOOL(obs_show_match_ids);
    PLUGIN_LOAD_BOOL(obs_show_map_id);
    PLUGIN_LOAD_BOOL(obs_show_age);
    PLUGIN_LOAD_BOOL(obs_show_type);
    PLUGIN_LOAD_BOOL(obs_show_count);
    PLUGIN_LOAD_BOOL(obs_show_match_unknowns);
    PLUGIN_LOAD_BOOL(obs_show_team1_name_dup);
    PLUGIN_LOAD_BOOL(obs_show_h007c_array);
    PLUGIN_LOAD_BOOL(obs_show_team_id);
    PLUGIN_LOAD_BOOL(obs_show_team_name);
    PLUGIN_LOAD_BOOL(obs_show_team_unknowns);
    PLUGIN_LOAD_BOOL(obs_show_cape_colors);
    PLUGIN_LOAD_BOOL(obs_show_cape_design);
    PLUGIN_LOAD_BOOL(show_match_compositions_window);
    PLUGIN_LOAD_BOOL(show_match_compositions_settings_window);
}

void ObserverPlugin::SaveSettings(const wchar_t* folder)
{
    ToolboxUIPlugin::SaveSettings(folder);
    PLUGIN_SAVE_BOOL(show_capture_status_window);
    PLUGIN_SAVE_BOOL(show_live_party_info_window);
    PLUGIN_SAVE_BOOL(show_live_guild_info_window);
    PLUGIN_SAVE_BOOL(show_available_matches_window);
    PLUGIN_SAVE_BOOL(show_stoc_log_window);
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

    PLUGIN_SAVE_BOOL(obs_show_match_ids);
    PLUGIN_SAVE_BOOL(obs_show_map_id);
    PLUGIN_SAVE_BOOL(obs_show_age);
    PLUGIN_SAVE_BOOL(obs_show_type);
    PLUGIN_SAVE_BOOL(obs_show_count);
    PLUGIN_SAVE_BOOL(obs_show_match_unknowns);
    PLUGIN_SAVE_BOOL(obs_show_team1_name_dup);
    PLUGIN_SAVE_BOOL(obs_show_h007c_array);
    PLUGIN_SAVE_BOOL(obs_show_team_id);
    PLUGIN_SAVE_BOOL(obs_show_team_name);
    PLUGIN_SAVE_BOOL(obs_show_team_unknowns);
    PLUGIN_SAVE_BOOL(obs_show_cape_colors);
    PLUGIN_SAVE_BOOL(obs_show_cape_design);
    PLUGIN_SAVE_BOOL(show_match_compositions_window);
    PLUGIN_SAVE_BOOL(show_match_compositions_settings_window);
}

// draws the generic UI settings in the main settings panel
void ObserverPlugin::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings(); // draw base settings (visibility, etc.)
}

const char* ObserverPlugin::Name() const {
    return "Observer Plugin";
}

void ObserverPlugin::AddLogEntry(const wchar_t* entry) {
    if (capture_handler) capture_handler->AddLogEntry(entry);
}

void ObserverPlugin::GenerateDefaultFolderName() {
    auto t = std::time(nullptr);
    std::tm tm; // use a separate tm struct
    errno_t err = localtime_s(&tm, &t); // use localtime_s
    if (err == 0) { // check for errors
        std::ostringstream oss;
        oss << "Match_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
        strncpy_s(export_folder_name, sizeof(export_folder_name), oss.str().c_str(), _TRUNCATE);
    } else {
        // handle error, e.g., use a default fixed name
        strncpy_s(export_folder_name, sizeof(export_folder_name), "Match_Default", _TRUNCATE);
    }
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
        
        // --- Observer Status --- 
        ImGui::Separator();
        bool is_observing = match_handler && match_handler->IsObserving();
        if (is_observing) {
            float windowWidth = ImGui::GetWindowSize().x;
            float textWidth = ImGui::CalcTextSize("OBSERVER MODE ACTIVE").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OBSERVER MODE ACTIVE");
        } else {
            float windowWidth = ImGui::GetWindowSize().x;
            float textWidth = ImGui::CalcTextSize("NOT OBSERVING").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextDisabled("NOT OBSERVING");
        }
        ImGui::Separator();
        ImGui::Spacing();

        // --- Export Section --- 
        if (ImGui::TreeNodeEx("Export Match", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f); 
            ImGui::InputText("##MatchNameInput", export_folder_name, sizeof(export_folder_name));
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Sets the name for the subfolder within 'captures/' where logs will be saved.\nLogs are saved under captures/<Match Name>/");
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Generate")) { 
                GenerateDefaultFolderName();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Generates a default Match Name based on the current date and time.");
            }
            ImGui::SameLine();
             if (ImGui::Button("Export")) {
                if (strlen(export_folder_name) > 0) {
                    std::wstring wfoldername = StringToWString(export_folder_name);
                    if (!wfoldername.empty()) {
                        if (match_handler) {
                           match_handler->ExportLogsToFolder(wfoldername.c_str());
                        } else {
                            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Error: Match handler not available.");
                        }
                    } else {
                         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Error converting folder name.");
                    }
                } else {
                     GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Please enter a Match Name for export.");
                }
            }
             if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Exports all captured StoC events and Agent Loop snapshots for the current match to the specified 'captures/<Match Name>/' folder.");
            }

            ImGui::Spacing();
            // Auto actions
            ImGui::Checkbox("Auto Export on Match End", &auto_export_on_match_end);
             if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If checked, automatically exports logs when observer mode ends using the current Match Name.");
            }
            ImGui::Checkbox("Auto Reset Match Name on Match End", &auto_reset_name_on_match_end);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If checked, automatically generates a new default match name when observer mode ends.");
            }

            ImGui::Unindent();
            ImGui::TreePop(); 
        }
        ImGui::Separator();
        ImGui::Spacing();

        // --- Windows Toggles --- 
        if (ImGui::TreeNode("Windows")) {
             ImGui::Checkbox("Match Compositions", &show_match_compositions_window);
             if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shows the match compositions window (Teams, Players, Skills).");
             ImGui::Checkbox("Match Compositions Settings", &show_match_compositions_settings_window);
             if (ImGui::IsItemHovered()) ImGui::SetTooltip("Opens settings for the Match Compositions window.");
             ImGui::TreePop();
        }
        ImGui::Separator();
        ImGui::Spacing();

        // --- Debug Windows Toggles --- 
        if (ImGui::TreeNode("Debug Windows")) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("This section provides access to various windows displaying detailed real-time information useful for debugging the observer plugin, understanding game state, or analyzing captured match data.");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Columns(2, "DebugWinToggles", false);
            ImGui::Checkbox("Capture Status", &show_capture_status_window);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shows the status of internal StoC event and Agent state capture.");
            ImGui::Checkbox("Live Party Info", &show_live_party_info_window);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Displays live information about agents (players, heroes, henchmen) currently detected in the instance, grouped by party.");
            ImGui::Checkbox("Live Guild Info", &show_live_guild_info_window);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Displays live information about guilds detected in the instance.");
            ImGui::NextColumn();
            ImGui::Checkbox("Available Matches", &show_available_matches_window);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lists observer matches available in the current outpost, with display options.");
            ImGui::Checkbox("StoC Logs Chat", &show_stoc_log_window);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Provides options to toggle the display of various StoC event logs in the game chat.");
            ImGui::Columns(1);
            ImGui::TreePop();
        }
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); 
        ImGui::TextWrapped("Note: Captured match data (StoC events & Agent states) remains in memory after observer mode ends. You can still export the previous match using the 'Export' button above. Data will be cleared automatically when a new observer session begins.");
        ImGui::PopStyleColor();
        
    }
    ImGui::End();

    if (show_capture_status_window) {
        capture_status_window.Draw(*this, show_capture_status_window);
    }
    if (show_live_party_info_window) {
        live_party_info_window.Draw(*this, show_live_party_info_window);
    }
    if (show_live_guild_info_window) {
        live_guild_info_window.Draw(*this, show_live_guild_info_window);
    }
    if (show_available_matches_window) {
        available_matches_window.Draw(*this, show_available_matches_window);
    }
    if (show_stoc_log_window) {
        stoc_log_window.Draw(*this, show_stoc_log_window);
    }

    if (show_match_compositions_settings_window && match_compositions_settings_window_) {
        match_compositions_settings_window_->Draw(*this, show_match_compositions_settings_window);
    }

    if (show_match_compositions_window) {
        match_compositions_window_.Draw(*this, show_match_compositions_window); 
    }
}

std::string ObserverPlugin::WStringToString(const std::wstring_view str) {
    if (str.empty()) {
        return "";
    }
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) { // fallback to ANSI if UTF8 fails (like original TextUtils)
         size_needed = WideCharToMultiByte(CP_ACP, 0, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
    }
    if (size_needed <= 0) return ""; // failed conversion

    std::string dest(size_needed, 0);
    int bytes_converted = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), dest.data(), size_needed, nullptr, nullptr);
     if (bytes_converted <= 0) { // fal lback to ANSI if UTF8 fails
        bytes_converted = WideCharToMultiByte(CP_ACP, 0, str.data(), static_cast<int>(str.size()), dest.data(), size_needed, nullptr, nullptr);
     }
    if (bytes_converted <= 0) return ""; // failed conversion

    return dest;
}

std::wstring ObserverPlugin::StringToWString(const std::string_view str) {
    if (str.empty()) {
        return L"";
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), static_cast<int>(str.size()), nullptr, 0);
     if (size_needed <= 0) { // fallback to ANSI if UTF8 fails (like original TextUtils)
         size_needed = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str.data(), static_cast<int>(str.size()), nullptr, 0);
     }
    if (size_needed <= 0) return L""; // failed conversion

    std::wstring dest(size_needed, 0);
    int chars_converted = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), dest.data(), size_needed);
     if (chars_converted <= 0) { // fallback to ANSI if UTF8 fails
         chars_converted = MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int>(str.size()), dest.data(), size_needed);
     }
    if (chars_converted <= 0) return L""; // failed conversion

    return dest;
}
