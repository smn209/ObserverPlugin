#pragma once

#include <imgui.h>

class ObserverPlugin;

class AvailableMatchesWindow {
public:
    void Draw(ObserverPlugin& plugin, bool& is_visible);
}; 