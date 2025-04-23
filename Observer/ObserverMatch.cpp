#include "../Base/stl.h" // needed for some types
#include "ObserverMatch.h"

#include <GWCA/Managers/StoCMgr.h>     
#include <GWCA/Managers/ChatMgr.h>    
#include <GWCA/Packets/StoC.h>      

ObserverMatch::ObserverMatch() {
}

// registers the callback for InstanceLoadInfo packets
void ObserverMatch::RegisterCallbacks() {
    GW::StoC::RegisterPacketCallback<GW::Packet::StoC::InstanceLoadInfo>(
        &InstanceLoadInfo_Entry,
        [this](const GW::HookStatus* status, const GW::Packet::StoC::InstanceLoadInfo* packet) -> void {
            HandleInstanceLoadInfo(status, packet);
        }
    );
}

// removes the callback for InstanceLoadInfo packets
void ObserverMatch::RemoveCallbacks() {
    GW::StoC::RemoveCallback<GW::Packet::StoC::InstanceLoadInfo>(&InstanceLoadInfo_Entry);
}

// handles the InstanceLoadInfo packet when received
void ObserverMatch::HandleInstanceLoadInfo(const GW::HookStatus* /*status*/, const GW::Packet::StoC::InstanceLoadInfo* packet) {
    if (!packet) return;

    is_observing = (packet->is_observer != 0);

    if (is_observing) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Entered Observer Mode instance.");
    } else {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Entered non-Observer Mode instance.");
    }
} 