#pragma once

#include <imgui.h>

class ObserverPlugin;

class LivePartyInfoWindow {
public:
    void Draw(ObserverPlugin& plugin, bool& is_visible);
}; 