#pragma once

#include <GWCA/Packets/StoC.h>

namespace GW {
    namespace Packet {
        namespace StoC {
            struct OpposingPartyGuild : Packet<OpposingPartyGuild>
            {
                uint32_t team_id;
                uint32_t unk2;
                uint32_t unk3;
                uint32_t unk4;
                uint32_t unk5;
                uint32_t unk6;
                uint32_t unk7;
                uint32_t unk8;
                uint32_t rank;
                uint32_t rating;
                wchar_t guild_name[32];
                wchar_t guild_tag[6];
            };
            template<> constexpr uint32_t Packet<OpposingPartyGuild>::STATIC_HEADER = 429;
        }
    }
}
