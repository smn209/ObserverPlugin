#pragma once

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Packets/StoC.h>

class ObserverStoC; 

namespace GW { namespace Packet { namespace StoC { struct InstanceLoadInfo; } } }

class ObserverMatch {
public:
    ObserverMatch(ObserverStoC* stoc_handler);
    ~ObserverMatch() = default;

    void RegisterCallbacks();
    void RemoveCallbacks();

    [[nodiscard]] bool IsObserving() const { return is_observing; }

private:
    void HandleInstanceLoadInfo(const GW::HookStatus* status, const GW::Packet::StoC::InstanceLoadInfo* packet);

    GW::HookEntry InstanceLoadInfo_Entry; // hook entry for instance load info packets
    bool is_observing = false;            // tracks if the current instance is observer mode
    ObserverStoC* stoc_handler_ = nullptr; // pointer to the StoC handler
}; 