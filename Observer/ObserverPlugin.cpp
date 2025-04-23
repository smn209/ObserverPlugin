#include "ObserverPlugin.h"
#include "ObserverStoC.h"
#include "ObserverUtils.h"

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

ObserverPlugin::ObserverPlugin()
{
    stoc_handler = new ObserverStoC(this);
    utils = new ObserverUtils(this);
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static ObserverPlugin instance; // create a static instance of the plugin
    return &instance; // return the instance
}

void ObserverPlugin::LoadSettings(
    const wchar_t* folder
)
{
    // settings loaded from the settings file
    ToolboxPlugin::LoadSettings(folder);
    PLUGIN_LOAD_BOOL(enabled);
    PLUGIN_LOAD_BOOL(log_skill_activations);
    PLUGIN_LOAD_BOOL(log_skill_finishes);
    PLUGIN_LOAD_BOOL(log_attack_skills);
    PLUGIN_LOAD_BOOL(log_interrupts);
    PLUGIN_LOAD_BOOL(log_instant_skills);
    PLUGIN_LOAD_BOOL(log_damage);
    PLUGIN_LOAD_BOOL(log_knockdowns);
    ToolboxPlugin::SaveSettings(folder);
}

void ObserverPlugin::SaveSettings(
    const wchar_t* folder
)
{
    // save the settings to the settings file
    PLUGIN_SAVE_BOOL(enabled);
    PLUGIN_SAVE_BOOL(log_skill_activations);
    PLUGIN_SAVE_BOOL(log_skill_finishes);
    PLUGIN_SAVE_BOOL(log_attack_skills);
    PLUGIN_SAVE_BOOL(log_interrupts);
    PLUGIN_SAVE_BOOL(log_instant_skills);
    PLUGIN_SAVE_BOOL(log_damage);
    PLUGIN_SAVE_BOOL(log_knockdowns);
    ToolboxPlugin::SaveSettings(folder);
}

void ObserverPlugin::DrawSettings()
{
    if (!toolbox_handle)
    {
        return;
    } // if the toolbox handle is not valid, return (case possible : plugin not loaded)
    
    ImGui::Checkbox("Enable Skill Logging", &enabled);
    ImGui::SameLine(); 
    ImGui::Text("(Master Toggle)");
    
    if (enabled)
    {
        ImGui::Indent();
        ImGui::Text("Log Options:");
        ImGui::Checkbox("Skill Activations", &log_skill_activations);
        ImGui::Checkbox("Skill Finishes", &log_skill_finishes);
        ImGui::Checkbox("Attack Skills", &log_attack_skills);
        ImGui::Checkbox("Interrupts", &log_interrupts);
        ImGui::Checkbox("Instant Skills", &log_instant_skills);
        ImGui::Checkbox("Damage Events", &log_damage);
        ImGui::Checkbox("Knockdowns", &log_knockdowns);
        ImGui::Unindent();
    }
}

void ObserverPlugin::Initialize(
    ImGuiContext* ctx, 
    const ImGuiAllocFns allocator_fns, 
    const HMODULE toolbox_dll
)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll); // initialize the plugin (from ToolboxPlugin)
    GW::Chat::WriteChat( 
        GW::Chat::CHANNEL_MODERATOR, 
        L"Observer Plugin initialized! (by simon1337482)"
    ); // initialized plugin message
    stoc_handler->RegisterCallbacks(); // register the callbacks (from ObserverStoC)
}

void ObserverPlugin::SignalTerminate()
{
    // this occurs when the plugin is terminated (from ToolboxPlugin)
    ToolboxPlugin::SignalTerminate(); // signal the termination (from ToolboxPlugin)
    stoc_handler->RemoveCallbacks(); // remove the callbacks (from ObserverStoC)
    
    delete stoc_handler; // delete the stoc handler
    delete utils; // delete the utils
}

bool ObserverPlugin::CanTerminate()
{
    // plugin can always terminate 
    return true;
}

void ObserverPlugin::Draw(
    IDirect3DDevice9* /*pDevice*/
)
{
    // this seems to be called every frame (from ToolboxPlugin)
}