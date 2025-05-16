#pragma once

#include <ToolboxUIPlugin.h> 
#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>
#include <vector>
#include <string>
#include <ctime> 
#include <iomanip> 
#include <sstream>

#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <Modules/Resources.h>

#include <GWCA/GameEntities/Skill.h>
#include <d3d9.h>

#include "ObserverMatch.h"
#include "ObserverCapture.h"
#include "ObserverLoop.h"
#include "Debug/CaptureStatusWindow.h"
#include "Debug/LivePartyInfoWindow.h"
#include "Debug/LiveGuildInfoWindow.h"
#include "Debug/AvailableMatchesWindow.h"
#include "Debug/StoCLogWindow.h"
#include "Windows/MatchCompositionsWindow.h"

class MatchCompositionsSettingsWindow;

class ObserverStoC;

// main plugin class coordinating observer functionality and UI
class ObserverPlugin : public ToolboxUIPlugin {
public:
    ObserverPlugin();
    ~ObserverPlugin() override;

    const char* Name() const override;
    [[nodiscard]] const char* Icon() const override { return ICON_FA_FLAG; }

    // plugin lifecycle functions
    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    void Draw(IDirect3DDevice9* pDevice) override;

    // settings load/save/draw
    void LoadSettings(const wchar_t* folder) override;
    void SaveSettings(const wchar_t* folder) override;
    void DrawSettings() override; // draws generic UI settings in the main panel

    // handlers for different aspects of observer mode
    ObserverStoC* stoc_handler = nullptr;
    ObserverMatch* match_handler = nullptr;
    ObserverCapture* capture_handler = nullptr;
    ObserverLoop* loop_handler = nullptr;

    // proxy methods for log capture
    void AddLogEntry(const wchar_t* entry);
    
    // Main window controls
    bool auto_export_on_match_end = false;
    bool auto_reset_name_on_match_end = false;
    char export_folder_name[128]; // buffer for folder name input

    // Debug window visibility
    bool show_capture_status_window = false;
    bool show_live_party_info_window = false;
    bool show_live_guild_info_window = false;
    bool show_available_matches_window = false;
    bool show_stoc_log_window = false; 

    // StoC logging settings (used by StoCLog section in debug window)
    bool stoc_status = false; // master toggle 
    bool log_skill_activations = true;
    bool log_attack_skill_activations = true;
    bool log_instant_skills = true;
    bool log_basic_attack_starts = false;
    bool log_basic_attack_stops = false;
    bool log_skill_finishes = true;
    bool log_skill_stops = true;
    bool log_interrupts = true;
    bool log_attack_skill_stops = true;
    bool log_attack_skill_finishes = true;
    bool log_basic_attack_finishes = true;
    bool log_damage = false;
    bool log_knockdowns = true;
    bool log_movement = false;
    bool log_jumbo_base_under_attack = true;
    bool log_jumbo_guild_lord_under_attack = true;
    bool log_jumbo_captured_shrine = true;
    bool log_jumbo_captured_tower = true;
    bool log_jumbo_party_defeated = true;
    bool log_jumbo_morale_boost = true;
    bool log_jumbo_victory = true;
    bool log_jumbo_flawless_victory = true;
    bool log_jumbo_unknown = false;

    // Available Matches display settings (used by Available Matches section in debug window)
    bool obs_show_match_ids = true;
    bool obs_show_map_id = true;
    bool obs_show_age = true;
    bool obs_show_type = true;
    bool obs_show_count = true;
    bool obs_show_match_unknowns = true; 
    bool obs_show_team1_name_dup = true;
    bool obs_show_h007c_array = false; 
    bool obs_show_team_id = true;
    bool obs_show_team_name = true;
    bool obs_show_team_unknowns = true; 
    bool obs_show_cape_colors = true;  
    bool obs_show_cape_design = true; 

    // match Details Window toggle
    bool show_match_compositions_window = false;

    // match Details Settings Window toggle
    bool show_match_compositions_settings_window = false;

    void GenerateDefaultFolderName();

    // public helper functions for use by other classes in the plugin
    std::string WStringToString(const std::wstring_view str);
    std::wstring StringToWString(const std::string_view str);

private:
    CaptureStatusWindow capture_status_window;
    LivePartyInfoWindow live_party_info_window;
    LiveGuildInfoWindow live_guild_info_window;
    AvailableMatchesWindow available_matches_window; 
    StoCLogWindow stoc_log_window;                 
    MatchCompositionsWindow match_compositions_window_;
    MatchCompositionsSettingsWindow* match_compositions_settings_window_ = nullptr; 
};