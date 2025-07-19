#pragma once

#include <ToolboxUIPlugin.h>
#include <GWCA/Constants/Constants.h>
#include <string>
#include <imgui.h>

class ObserverPlugin;

class LordDamageWindow {
public:
    LordDamageWindow() = default;
    ~LordDamageWindow() = default;

    void Draw(ObserverPlugin& obs_plugin, bool& is_visible);

private:
    void DrawTeamDamageRow(const std::string& team_name, const std::string& guild_tag, long damage, const ImColor& color);
    void DrawInfoSection();
    ImColor GetTeamColor(uint32_t team_id) const;
};
