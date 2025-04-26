#pragma once

#include <imgui.h>

class ObserverPlugin;

class StoCLogWindow {
public:
    void Draw(ObserverPlugin& plugin, bool& is_visible);
}; 