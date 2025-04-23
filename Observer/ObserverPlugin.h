#pragma once

#include <ToolboxUIPlugin.h>
#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>

class ObserverStoC;

class ObserverPlugin : public ToolboxPlugin {
public:
    ObserverPlugin();
    ~ObserverPlugin() override = default;

    const char* Name() const override { return "Observer Plugin"; }

    void LoadSettings(const wchar_t*) override;
    void SaveSettings(const wchar_t*) override;
    [[nodiscard]] bool HasSettings() const override { return true; }
    void DrawSettings() override;
    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
    void Draw(IDirect3DDevice9* pDevice) override;

    ObserverStoC* stoc_handler = nullptr;

    bool enabled = true;
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
};