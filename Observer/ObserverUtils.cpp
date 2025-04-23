#include "ObserverUtils.h"
#include "ObserverPlugin.h"

#include <GWCA/Managers/ChatMgr.h>

ObserverUtils::ObserverUtils(ObserverPlugin* owner_plugin) : plugin(owner_plugin) {
}

void ObserverUtils::writeChatMessage(const wchar_t* message) {
    // most likely the function always called from ObserverPlugin when we want to write a message to the chat
    // so we just call the function from the GWCA
    // we are able later to filter if we wants here
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, message);
}