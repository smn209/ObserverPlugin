#pragma once

#include <ToolboxUIPlugin.h> 
#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>
#include <vector>
#include <string>
#include <ctime> 
#include <iomanip> 
#include <sstream>

#include "ObserverMatch.h"
#include "ObserverCapture.h"

class ObserverStoC;

// main plugin class coordinating observer functionality and UI
class ObserverPlugin : public ToolboxUIPlugin {
public:
    ObserverPlugin();
    ~ObserverPlugin() override;

    const char* Name() const override { return "Observer Plugin"; }

    // plugin lifecycle functions
    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    void Draw(IDirect3DDevice9* pDevice) override;

    // settings load/save/draw
    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;
    void DrawSettings() override; // draws generic UI settings in the main panel

    // handlers for different aspects of observer mode
    ObserverStoC* stoc_handler = nullptr;
    ObserverMatch* match_handler = nullptr;
    ObserverCapture* capture_handler = nullptr;

    // proxy methods for log capture
    void AddLogEntry(const wchar_t* entry) { 
        if (capture_handler) capture_handler->AddLogEntry(entry); 
    }
    
    void ClearLogs() { 
        if (capture_handler) capture_handler->ClearLogs(); 
    }
    
    bool ExportLogsToFolder(const wchar_t* folder_name) {
        return capture_handler ? capture_handler->ExportLogsToFolder(folder_name) : false;
    }

    bool stoc_status = false; // master toggle 
    bool log_skill_activations = false;
    bool log_attack_skill_activations = false;
    bool log_instant_skills = false;
    bool log_basic_attack_starts = false;
    bool log_basic_attack_stops = false;
    bool log_skill_finishes = false;
    bool log_skill_stops = false;
    bool log_interrupts = false;
    bool log_attack_skill_stops = false;
    bool log_attack_skill_finishes = false;
    bool log_basic_attack_finishes = false;
    bool log_damage = false;
    bool log_knockdowns = false;
    bool log_movement = false;

    bool log_jumbo_base_under_attack = true;
    bool log_jumbo_guild_lord_under_attack = true;
    bool log_jumbo_captured_shrine = true;
    bool log_jumbo_captured_tower = true;
    bool log_jumbo_party_defeated = true;
    bool log_jumbo_morale_boost = true;
    bool log_jumbo_victory = true;
    bool log_jumbo_flawless_victory = true;
    bool log_jumbo_unknown = true;

private:
    // visibility state is handled by plugin_visible in ToolboxUIPlugin base class
    // renamed buffer for the log export folder/match name input field
    char export_folder_name[256];   

    // helper to generate default folder name
    void GenerateDefaultFolderName() {
        auto t = std::time(nullptr);
        std::tm tm; // use a separate tm struct
        errno_t err = localtime_s(&tm, &t); // use localtime_s
        if (err == 0) { // check for errors
            std::ostringstream oss;
            oss << "Match_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
            strncpy_s(export_folder_name, oss.str().c_str(), sizeof(export_folder_name) - 1);
            export_folder_name[sizeof(export_folder_name) - 1] = '\0'; // ensure null termination
        } else {
            // handle error, e.g., use a default fixed name
            strncpy_s(export_folder_name, "Match_Default", sizeof(export_folder_name) - 1);
            export_folder_name[sizeof(export_folder_name) - 1] = '\0';
        }
    }
};