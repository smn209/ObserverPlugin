#pragma once

#include <imgui.h>

class ObserverPlugin;

class LiveGuildInfoWindow {
public:
    void Draw(ObserverPlugin& plugin, bool& is_visible);
}; 