#include "../Base/stl.h" // needed for some types
#include "ObserverMatch.h"
#include "ObserverStoC.h" 
#include "ObserverPlugin.h"

#include <GWCA/Managers/StoCMgr.h>     
#include <GWCA/Managers/ChatMgr.h>    
#include <GWCA/Packets/StoC.h>      

ObserverMatch::ObserverMatch(ObserverStoC* stoc_handler)
    : stoc_handler_(stoc_handler) 
{
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
    // also ensure StoC callbacks are removed if we are currently observing
    if (is_observing && stoc_handler_) {
        stoc_handler_->RemoveCallbacks();
        
        // stop agent loop when leaving observer mode
        if (owner_plugin && owner_plugin->loop_handler) {
            owner_plugin->loop_handler->Stop();
        }
        
        is_observing = false; 
    }
}

// handles the InstanceLoadInfo packet when received
void ObserverMatch::HandleInstanceLoadInfo(const GW::HookStatus* /*status*/, const GW::Packet::StoC::InstanceLoadInfo* packet) {
    if (!packet) return;
    if (!stoc_handler_) return; // don't do anything if handler is null

    bool was_observing = is_observing;
    // determine if current instance is observer mode based on the packet flag
    is_observing = (packet->is_observer != 0);

    // state transition logic: handles entering/exiting observer mode
    if (is_observing && !was_observing) {
        // entered observer mode - clear logs and start fresh
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Entered Observer Mode instance. Clearing old logs and registering StoC callbacks.");
        
        // clear logs if this is a new observer match
        if (owner_plugin) {
            owner_plugin->ClearLogs();
        }
        
        stoc_handler_->RegisterCallbacks();
        
        // start agent loop if enabled
        if (owner_plugin && owner_plugin->loop_handler) {
            owner_plugin->loop_handler->Start();
        }
    } else if (!is_observing && was_observing) {
        // exited observer mode
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Exited Observer Mode instance. Removing StoC callbacks.");
        stoc_handler_->RemoveCallbacks();
        
        // stop agent loop
        if (owner_plugin && owner_plugin->loop_handler) {
            owner_plugin->loop_handler->Stop();
        }
        if (owner_plugin) {
            owner_plugin->HandleMatchEnd();
        }
    } else if (is_observing) {
        // still observing (e.g., map change within observer mode)
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Still in Observer Mode instance.");
    } else {
        // still not observing
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Still in non-Observer Mode instance.");
    }
} 