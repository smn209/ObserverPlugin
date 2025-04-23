#pragma once

#include <ToolboxUIPlugin.h> 
#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>

#include "ObserverMatch.h"

class ObserverStoC;

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

    ObserverStoC* stoc_handler = nullptr;
    ObserverMatch* match_handler = nullptr;

    bool enabled = true; // master toggle
    bool log_skill_activations = true;
    bool log_skill_finishes = true;
    bool log_skill_stops = true;
    bool log_attack_skill_activations = true;
    bool log_attack_skill_finishes = true;
    bool log_attack_skill_stops = true;
    bool log_basic_attack_starts = true;
    bool log_basic_attack_finishes = true;
    bool log_basic_attack_stops = true;
    bool log_interrupts = true;
    bool log_instant_skills = true;
    bool log_damage = true;
    bool log_knockdowns = true;
    bool log_movement = true;

private:
    // visibility state is handled by plugin_visible in ToolboxUIPlugin base class
};