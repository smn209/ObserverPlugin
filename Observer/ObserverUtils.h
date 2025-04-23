#pragma once

class ObserverPlugin;

class ObserverUtils {
public:
    ObserverUtils(ObserverPlugin* owner_plugin);
    ~ObserverUtils() = default;
    
    void writeChatMessage(const wchar_t* message);

private:
    ObserverPlugin* plugin = nullptr;
};