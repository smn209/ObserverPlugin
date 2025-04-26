#pragma once

#include <imgui.h>

class ObserverPlugin;

class CaptureStatusWindow {
public:
    void Draw(ObserverPlugin& plugin, bool& is_visible);
}; 