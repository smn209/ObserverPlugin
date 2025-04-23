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
};