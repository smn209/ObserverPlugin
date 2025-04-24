#pragma once

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>

class ObserverStoC; 
class ObserverPlugin;

namespace GW { namespace Packet { namespace StoC { struct InstanceLoadInfo; } } }

class ObserverMatch {
public:
    ObserverMatch(ObserverStoC* stoc_handler);
    ~ObserverMatch() = default;

    void RegisterCallbacks();
    void RemoveCallbacks();
    void SetOwnerPlugin(ObserverPlugin* plugin) { owner_plugin = plugin; }

    [[nodiscard]] bool IsObserving() const { return is_observing; }

private:
    void HandleInstanceLoadInfo(const GW::HookStatus* status, const GW::Packet::StoC::InstanceLoadInfo* packet);

    GW::HookEntry InstanceLoadInfo_Entry; // manages the hook for instance load info packets
    bool is_observing = false;            // flag to indicate if the current map is an observer mode instance
    ObserverStoC* stoc_handler_ = nullptr; // pointer to the StoC handler
    ObserverPlugin* owner_plugin = nullptr; // pointer to the owner plugin
}; 