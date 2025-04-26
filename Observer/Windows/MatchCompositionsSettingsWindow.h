#pragma once

#include <imgui.h>

class ObserverPlugin;

class MatchCompositionsSettingsWindow {
public:
    MatchCompositionsSettingsWindow() = default;
    ~MatchCompositionsSettingsWindow() = default;

    void Draw(ObserverPlugin& obs_plugin, bool& is_visible);

private:
}; 