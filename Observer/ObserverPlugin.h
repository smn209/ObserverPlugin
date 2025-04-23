#pragma once

#include <ToolboxUIPlugin.h>
#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>

class ObserverStoC;
class ObserverUtils;

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

    bool enabled = true;
    bool log_skill_activations = true;
    bool log_skill_finishes = true;
    bool log_attack_skills = true;
    bool log_interrupts = true;
    bool log_instant_skills = true;
    bool log_damage = true;
    bool log_knockdowns = true;

private:
    ObserverStoC* stoc_handler; //
    ObserverUtils* utils;
};