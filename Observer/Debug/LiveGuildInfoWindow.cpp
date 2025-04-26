#include "LiveGuildInfoWindow.h"
#include "../ObserverPlugin.h"
#include "../ObserverMatchData.h" 

#include <map>
#include <string>

void LiveGuildInfoWindow::Draw(ObserverPlugin& plugin, bool& is_visible)
{
    if (!is_visible) return;

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Observer Plugin - Live Guild Info", &is_visible))
    {
        ImGui::Indent();
        if (plugin.match_handler) {
            std::map<uint16_t, GuildInfo> guilds_info = plugin.match_handler->GetMatchInfo().GetGuildsInfoCopy();
            if (guilds_info.empty()) {
                ImGui::TextDisabled("No guild data captured yet.");
            } else {
                for (const auto& [guild_id, guild_info] : guilds_info) {
                    std::string narrow_name_display;
                    narrow_name_display.reserve(guild_info.name.length());
                    for (wchar_t wc : guild_info.name) {
                        if (wc >= 32 && wc <= 126) narrow_name_display += static_cast<char>(wc);
                    }
                    std::string narrow_tag_display;
                    narrow_tag_display.reserve(guild_info.tag.length());
                    for (wchar_t wc : guild_info.tag) {
                        if (wc >= 32 && wc <= 126) narrow_tag_display += static_cast<char>(wc);
                    }
                    ImGui::Text("ID: %u, Name: %s, Tag: [%s]", 
                                guild_info.guild_id, narrow_name_display.c_str(), narrow_tag_display.c_str());
                    ImGui::Text("  Rank: %u, Rating: %u, Features: %u", 
                                guild_info.rank, guild_info.rating, guild_info.features);
                    ImGui::Text("  Faction: %s (%u pts), Qualifier Pts: %u",
                                (guild_info.faction == 0 ? "Kurzick" : (guild_info.faction == 1 ? "Luxon" : "Unknown")), 
                                guild_info.faction_points, guild_info.qualifier_points);
                    ImGui::Text("  Cape: BG(0x%X), Detail(0x%X), Emblem(0x%X)", 
                                guild_info.cape.cape_bg_color, guild_info.cape.cape_detail_color, guild_info.cape.cape_emblem_color);
                    ImGui::Text("        Shape(%u), Detail(%u), Emblem(%u), Trim(%u)",
                                guild_info.cape.cape_shape, guild_info.cape.cape_detail, guild_info.cape.cape_emblem, guild_info.cape.cape_trim);
                    ImGui::Separator();
                }
            }
        } else {
             ImGui::TextDisabled("Match handler not available.");
        }
        ImGui::Unindent();
    }
    ImGui::End();
} 