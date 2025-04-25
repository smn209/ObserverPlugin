#include "../Base/stl.h" 
#include "ObserverMatch.h"
#include "ObserverStoC.h" 
#include "ObserverPlugin.h"
#include "ObserverCapture.h" 
#include "ObserverLoop.h"    

#include <GWCA/Managers/StoCMgr.h>     
#include <GWCA/Managers/ChatMgr.h>    
#include <GWCA/Packets/StoC.h>      
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/PartyContext.h>
#include <GWCA/Managers/AgentMgr.h>   

#include <filesystem> 
#include <fstream>    
#include <string>     
#include <sstream>    
#include <vector>     
#include <algorithm>  
#include <windows.h>  

std::string EscapeWideStringForJSON(const std::wstring& wstr);

ObserverMatch::ObserverMatch(ObserverStoC* stoc_handler)
    : stoc_handler_(stoc_handler)
{
    current_match_info_.Reset();
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
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Entered Observer Mode instance. Clearing old logs, resetting match info, and registering StoC callbacks.");

        // reset match info and set map ID for the new match
        current_match_info_.Reset();
        current_match_info_.map_id = packet->map_id;
        current_match_info_.end_time_formatted = L"";
        current_match_info_.end_time_ms = 0;
        current_match_info_.winner_party_id = 0;

        // clear StoC/Agent logs if this is a new observer match
        if (owner_plugin) {
            this->ClearLogs();
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
            this->HandleMatchEnd();
        }
        // Note: We keep current_match_info_ as is when exiting,
        // so the last match's info can still be exported.
        // it will be reset upon entering the *next* observer instance.
    } else if (is_observing) {
        if (current_match_info_.map_id != packet->map_id) {
             GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer Mode map change detected (unexpected?). Resetting match info.");
             current_match_info_.Reset();
             current_match_info_.map_id = packet->map_id;
             if (owner_plugin) {
                this->ClearLogs();
             }
        }
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Still in Observer Mode instance.");
    } else {
        // still not observing
        // GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Still in non-Observer Mode instance."); 
    }
}

void ObserverMatch::SetMatchEndInfo(uint32_t end_time_ms, uint32_t raw_winner_id) {
    uint32_t party_id_to_store = 0;
    if (raw_winner_id == GW::Packet::StoC::JumboMessageValue::PARTY_ONE) {
        party_id_to_store = 1;
    } else if (raw_winner_id == GW::Packet::StoC::JumboMessageValue::PARTY_TWO) {
        party_id_to_store = 2;
    } else {
        wchar_t warning_msg[256];
        swprintf_s(warning_msg, L"Warning: SetMatchEndInfo called with unexpected raw winner ID: %u. Storing 0.", raw_winner_id);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, warning_msg);
        party_id_to_store = 0;
    }

    current_match_info_.end_time_ms = end_time_ms;
    current_match_info_.winner_party_id = party_id_to_store;

    // format the timestamp like in ObserverCapture::AddLogEntry
    uint32_t total_seconds = end_time_ms / 1000;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    uint32_t milliseconds = end_time_ms % 1000;
    wchar_t formatted_time[32];
    swprintf(formatted_time, 32, L"%02u:%02u.%03u", minutes, seconds, milliseconds);

    current_match_info_.end_time_formatted = formatted_time;

    wchar_t msg[256];
    swprintf_s(msg, L"Match end info captured by ObserverMatch: Time=[%ls], Winner Party=%u", formatted_time, current_match_info_.winner_party_id);
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg);
}

void MatchInfo::UpdateAgentInfo(const AgentInfo& info) {
    if (info.agent_id == 0) return; 

    std::lock_guard<std::mutex> lock(agents_info_mutex);
    // find the agent in the agents_info map
    auto it = agents_info.find(info.agent_id);
    if (it != agents_info.end()) {
        // agent exists, preserve existing skills and update other fields
        std::set<uint32_t> existing_skills = it->second.used_skill_ids;
        it->second = info; // update the basic info
        it->second.used_skill_ids = std::move(existing_skills); // restore skills
    } else {
        // new agent, insert directly
        agents_info[info.agent_id] = info;
    }
}

std::map<uint32_t, AgentInfo> MatchInfo::GetAgentsInfoCopy() const {
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    return agents_info; // return a copy of the map
}

void MatchInfo::AddSkillUsed(uint32_t agent_id, uint32_t skill_id) {
    if (agent_id == 0 || skill_id == 0) return; // ignore invalid IDs

    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        // agent found, insert the skill ID into their set
        it->second.used_skill_ids.insert(skill_id);
    }
    // if agent not found, do nothing. agent info should be populated by ObserverLoop first.
}

void MatchInfo::UpdateGuildInfo(const GuildInfo& info) {
    if (info.guild_id == 0) return; // don't store info for invalid guild IDs

    std::lock_guard<std::mutex> lock(guilds_info_mutex);

    guilds_info[info.guild_id] = info;
}

std::map<uint16_t, GuildInfo> MatchInfo::GetGuildsInfoCopy() const {
    std::lock_guard<std::mutex> lock(guilds_info_mutex);
    return guilds_info; // return a copy of the map
}


void ObserverMatch::ClearLogs() {
    if (owner_plugin && owner_plugin->capture_handler) {
        owner_plugin->capture_handler->ClearLogs();
    }
    if (owner_plugin && owner_plugin->loop_handler) {
        owner_plugin->loop_handler->ClearAgentLogs();
    }
}

bool ObserverMatch::ExportLogsToFolder(const wchar_t* folder_name) {
    bool any_success = false;
    bool infos_success = false;
    bool stoc_success = false;
    bool agent_success = false;

    try {
        std::filesystem::path base_dir = "captures";
        std::filesystem::path match_dir = base_dir / folder_name;

        // ensure the base directory exists
        std::filesystem::create_directories(match_dir);

        // export infos.json using current_match_info_
        const MatchInfo& match_info = this->GetMatchInfo(); 
        std::map<uint32_t, AgentInfo> agents_info = match_info.GetAgentsInfoCopy();
        std::filesystem::path info_file = match_dir / "infos.json";
        std::ofstream outfile(info_file);
        if (outfile.is_open()) {
             outfile << "{\n";
                outfile << "  \"map_id\": " << match_info.map_id;
                if (match_info.end_time_ms > 0) {
                    outfile << ",\n";
                    outfile << "  \"match_end_time_ms\": " << match_info.end_time_ms;

                    std::string utf8_formatted_time;
                    try {
                         int size_needed = WideCharToMultiByte(CP_UTF8, 0, match_info.end_time_formatted.c_str(), (int)match_info.end_time_formatted.size(), NULL, 0, NULL, NULL);
                         if (size_needed > 0) {
                              utf8_formatted_time.resize(size_needed, 0);
                              WideCharToMultiByte(CP_UTF8, 0, match_info.end_time_formatted.c_str(), (int)match_info.end_time_formatted.size(), &utf8_formatted_time[0], size_needed, NULL, NULL);
                         }
                    } catch (...) {
                         utf8_formatted_time = "[conversion_error]";
                    }
                    outfile << ",\n";
                    outfile << "  \"match_end_time_formatted\": \"" << utf8_formatted_time << "\"";
                }
                if (match_info.winner_party_id > 0) {
                    outfile << ",\n";
                    outfile << "  \"winner_party_id\": " << match_info.winner_party_id;
                }

                auto AgentTypeToJSONString = [](AgentType type) -> const char* {
                    switch (type) {
                        case AgentType::PLAYER: return "PLAYER";
                        case AgentType::HERO: return "HERO";
                        case AgentType::HENCHMAN: return "HENCHMAN";
                        case AgentType::OTHER: return "OTHER";
                        default: return "UNKNOWN";
                    }
                };

                // create aa vector and sort for structured JSON output
                std::vector<AgentInfo> sorted_agent_list;
                sorted_agent_list.reserve(agents_info.size());
                for(const auto& pair : agents_info) {
                    sorted_agent_list.push_back(pair.second);
                }
                std::sort(sorted_agent_list.begin(), sorted_agent_list.end(), [](const AgentInfo& a, const AgentInfo& b) {
                    if (a.party_id != b.party_id) return a.party_id < b.party_id;
                    if (a.type != b.type) return static_cast<int>(a.type) < static_cast<int>(b.type); // Enum comparison
                    return a.agent_id < b.agent_id;
                });

                // add structured parties section
                outfile << ",\n";
                outfile << "  \"parties\": {";
                if (!sorted_agent_list.empty()) {
                    outfile << "\n";

                    uint32_t current_party_id = (uint32_t)-1;
                    AgentType current_type = (AgentType)-1;
                    bool first_party = true;
                    bool first_type_in_party = true;

                    for (size_t i = 0; i < sorted_agent_list.size(); ++i) {
                        const auto& agent = sorted_agent_list[i];

                        // start new party object
                        if (agent.party_id != current_party_id) {
                            // close previous type array and party object if needed
                            if (!first_party) {
                                outfile << " ]"; // close last type array
                                outfile << "\n    }"; // close last party object
                            }

                            if (!first_party) outfile << ",\n";
                            outfile << "    \"" << agent.party_id << "\": {";
                            current_party_id = agent.party_id;
                            first_party = false;
                            first_type_in_party = true;
                            current_type = (AgentType)-1; // reset type for new party
                        }

                        // start new type array
                        if (agent.type != current_type) {
                             // close previous type array if needed
                            if (!first_type_in_party) {
                                outfile << " ]";
                            }
                            if (!first_type_in_party) outfile << ",";
                            outfile << "\n      \"" << AgentTypeToJSONString(agent.type) << "\": ["; // start array (no trailing space)
                            current_type = agent.type;
                            first_type_in_party = false;
                        }

                        // determine if a comma is needed before this agent
                        bool needs_comma = false;
                        if (i > 0 && sorted_agent_list[i-1].party_id == agent.party_id && sorted_agent_list[i-1].type == agent.type) {
                            // needs comma if previous agent was in the same party and of the same type
                            needs_comma = true;
                        }
                        if (needs_comma) {
                             outfile << ", ";
                        }

                        // convert and escape name specifically for JSON output using the helper
                        std::string escaped_name_json = EscapeWideStringForJSON(agent.encoded_name);

                        outfile << "{\"id\": " << agent.agent_id
                                << ", \"primary\": " << agent.primary_profession
                                << ", \"secondary\": " << agent.secondary_profession
                                << ", \"level\": " << agent.level
                                << ", \"team_id\": " << agent.team_id
                                << ", \"player_number\": " << agent.player_number
                                << ", \"guild_id\": " << agent.guild_id
                                << ", \"encoded_name\": " << escaped_name_json
                                << ", \"used_skills\": [";
                        // add used skills to the array
                        bool first_skill = true;
                        for (uint32_t skill_id : agent.used_skill_ids) {
                            if (!first_skill) outfile << ",";
                            outfile << skill_id;
                            first_skill = false;
                        }
                        outfile << "] }"; // close used_skills array and agent object

                        // close last structures if this is the last agent overall
                        if (i == sorted_agent_list.size() - 1) {
                             outfile << " ]"; // close last type array
                            outfile << "\n    }"; // close last party object
                        }
                    }
                    outfile << "\n  ";
                }
                outfile << "}\n";

                outfile << ",\n";
                outfile << "  \"guilds\": {";
                std::map<uint16_t, GuildInfo> guilds_info = match_info.GetGuildsInfoCopy();
                if (!guilds_info.empty()) {
                    outfile << "\n";
                    bool first_guild = true;
                    for (const auto& [guild_id, guild_info] : guilds_info) {
                        if (!first_guild) outfile << ",\n";
                        std::string escaped_name = EscapeWideStringForJSON(guild_info.name);
                        std::string escaped_tag = EscapeWideStringForJSON(guild_info.tag);
                        outfile << "    \"" << guild_id << "\": {"
                                << "\"id\": " << guild_info.guild_id
                                << ", \"name\": " << escaped_name
                                << ", \"tag\": " << escaped_tag
                                << ", \"rank\": " << guild_info.rank
                                << ", \"features\": " << guild_info.features
                                << ", \"rating\": " << guild_info.rating
                                << ", \"faction\": " << guild_info.faction
                                << ", \"faction_points\": " << guild_info.faction_points
                                << ", \"qualifier_points\": " << guild_info.qualifier_points
                                << ", \"cape\": { "
                                << "\"bg_color\": " << guild_info.cape.cape_bg_color << ", "
                                << "\"detail_color\": " << guild_info.cape.cape_detail_color << ", "
                                << "\"emblem_color\": " << guild_info.cape.cape_emblem_color << ", "
                                << "\"shape\": " << guild_info.cape.cape_shape << ", "
                                << "\"detail\": " << guild_info.cape.cape_detail << ", "
                                << "\"emblem\": " << guild_info.cape.cape_emblem << ", "
                                << "\"trim\": " << guild_info.cape.cape_trim
                                << " }"
                                << " }";
                        first_guild = false;
                    }
                    outfile << "\n  ";
                }
                outfile << "}\n"; 

                outfile << "}\n";
                outfile.close();
                infos_success = !outfile.fail(); // check for errors after closing
            } else {
                std::wstring error_msg = L"Failed to open infos.json for writing: ";
                error_msg += info_file.wstring();
                GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, error_msg.c_str());
            }
        
        // export StoC logs via owner_plugin
        if (owner_plugin && owner_plugin->capture_handler) {
            stoc_success = owner_plugin->capture_handler->ExportLogsToFolder(folder_name);
        } else {
            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Capture handler invalid (via owner plugin), cannot export StoC logs.");
        }

        // export Loop logs via owner_plugin
        if (owner_plugin && owner_plugin->loop_handler) {
            agent_success = owner_plugin->loop_handler->ExportAgentLogs(folder_name);
        } else {
            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Loop handler invalid (via owner plugin), cannot export Agent logs.");
        }

        any_success = infos_success || stoc_success || agent_success;

    } catch (const std::filesystem::filesystem_error& e) {
        std::string error_msg_str = "Filesystem error during export: ";
        error_msg_str += e.what();
        wchar_t werror_msg[512] = {0};
        MultiByteToWideChar(CP_UTF8, 0, error_msg_str.c_str(), -1, werror_msg, 512);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, werror_msg);
        any_success = false;
    } catch (const std::exception& e) {
        std::string error_msg_str = "Generic error during export: ";
        error_msg_str += e.what();
        wchar_t werror_msg[512] = {0};
        MultiByteToWideChar(CP_UTF8, 0, error_msg_str.c_str(), -1, werror_msg, 512);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, werror_msg);
        any_success = false;
    } catch (...) {
         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"An unexpected error occurred during export.");
         any_success = false;
    }

    // overall success message
    if (any_success) {
        wchar_t msg[512];
        swprintf_s(msg, L"Export attempt finished for '%ls'. Infos: %ls, StoC: %ls, Agents: %ls.", 
                   folder_name, 
                   infos_success ? L"OK" : L"FAIL", 
                   stoc_success ? L"OK" : L"FAIL",
                   agent_success ? L"OK" : L"FAIL");
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg);
    } else {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Export completely failed.");
    }
    
    return any_success;
}

void ObserverMatch::HandleMatchEnd() {
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer mode ended (handled by ObserverMatch).");

    if (owner_plugin && owner_plugin->loop_handler) {
        owner_plugin->loop_handler->Stop();
    }

    if (owner_plugin && owner_plugin->auto_export_on_match_end) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-export triggered...");
        if (strlen(owner_plugin->export_folder_name) > 0) {
            wchar_t wfoldername[sizeof(owner_plugin->export_folder_name)];
            size_t converted = 0;
            errno_t err = mbstowcs_s(&converted, wfoldername, sizeof(owner_plugin->export_folder_name), owner_plugin->export_folder_name, _TRUNCATE);

            if (err == 0) {
                if (this->ExportLogsToFolder(wfoldername)) { 
                     wchar_t msg[512];
                     swprintf_s(msg, L"Successfully auto-exported logs to 'captures/%ls'.", wfoldername); 
                     GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg);
                } else {
                     wchar_t msg[512];
                     swprintf_s(msg, L"Auto-export failed. Check previous errors. Folder was 'captures/%ls'.", wfoldername);
                     GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg);
                }
            } else {
                 GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Error converting folder name for auto-export.");
            }
        } else {
             GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-export skipped: Match Name is empty.");
        }
    } else {
         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-export disabled.");
    }

    if (owner_plugin && owner_plugin->auto_reset_name_on_match_end) {
        owner_plugin->GenerateDefaultFolderName(); 
        wchar_t msg[512];
        swprintf_s(msg, L"Auto-resetting Match Name to '%hs'.", owner_plugin->export_folder_name); 
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg);
    } else {
         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-reset name disabled.");
    }
}
