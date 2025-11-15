#include "../Base/stl.h" 
#include "ObserverMatch.h"
#include "ObserverStoC.h" 
#include "ObserverPlugin.h"
#include "ObserverMatchData.h"
#include "ObserverCapture.h" 
#include "ObserverLoop.h"    
#include "TextUtils.h"

#include <GWCA/Managers/StoCMgr.h>     
#include <GWCA/Managers/ChatMgr.h>    
#include <GWCA/Managers/UIMgr.h>    
#include <GWCA/Packets/StoC.h>      
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/PartyContext.h>
#include <GWCA/Managers/AgentMgr.h>   

#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/Constants/Skills.h>

#include <filesystem> 
#include <fstream>    
#include <string>     
#include <sstream>    
#include <vector>     
#include <algorithm>  
#include <windows.h>  

#include <GWCA/Utilities/Scanner.h>


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
        ObserverMatchData::InitializeLordDamage();
        ObserverMatchData::InitializeTeamKillCount();
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
             ObserverMatchData::InitializeLordDamage();
             ObserverMatchData::InitializeTeamKillCount();
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

    // format the original timestamp like in ObserverCapture::AddLogEntry
    uint32_t total_seconds_orig = end_time_ms / 1000;
    uint32_t minutes_orig = total_seconds_orig / 60;
    uint32_t seconds_orig = total_seconds_orig % 60;
    uint32_t milliseconds_orig = end_time_ms % 1000;
    wchar_t formatted_time[32];
    swprintf(formatted_time, _countof(formatted_time), L"%02u:%02u.%03u", minutes_orig, seconds_orig, milliseconds_orig);
    current_match_info_.end_time_formatted = formatted_time;

    // store original match duration (without adjustment)
    wchar_t original_duration_str[16];
    swprintf(original_duration_str, _countof(original_duration_str), L"%02u:%02u", minutes_orig, seconds_orig);
    current_match_info_.match_original_duration = original_duration_str;

    // calculate and format adjusted match duration
    uint32_t adjusted_duration_ms;
    if (end_time_ms < 60000) { // less than 1 minute
        adjusted_duration_ms = 0;
    } else {
        adjusted_duration_ms = end_time_ms - 60000; // subtract 1 minute (time before door open)
    }
    uint32_t adj_total_seconds = adjusted_duration_ms / 1000;
    uint32_t adj_minutes = adj_total_seconds / 60;
    uint32_t adj_seconds = adj_total_seconds % 60;
    wchar_t adjusted_formatted_str[16];
    swprintf(adjusted_formatted_str, _countof(adjusted_formatted_str), L"%02u:%02u", adj_minutes, adj_seconds);
    current_match_info_.match_duration = adjusted_formatted_str;

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
        std::vector<uint32_t> existing_skills = it->second.used_skill_ids;
        long existing_damage = it->second.total_damage;
        uint32_t existing_attacks_started = it->second.attacks_started;
        uint32_t existing_attacks_finished = it->second.attacks_finished;
        uint32_t existing_attacks_stopped = it->second.attacks_stopped;
        uint32_t existing_skills_activated = it->second.skills_activated;
        uint32_t existing_skills_finished = it->second.skills_finished;
        uint32_t existing_skills_stopped = it->second.skills_stopped;
        uint32_t existing_attack_skills_activated = it->second.attack_skills_activated;
        uint32_t existing_attack_skills_finished = it->second.attack_skills_finished;
        uint32_t existing_attack_skills_stopped = it->second.attack_skills_stopped;
        uint32_t existing_interrupted_count = it->second.interrupted_count;
        uint32_t existing_interrupted_skills_count = it->second.interrupted_skills_count;
        uint32_t existing_cancelled_attacks_count = it->second.cancelled_attacks_count;
        uint32_t existing_cancelled_skills_count = it->second.cancelled_skills_count;
        uint32_t existing_crits_dealt = it->second.crits_dealt;
        uint32_t existing_crits_received = it->second.crits_received;
        uint32_t existing_deaths = it->second.deaths;
        it->second = info;
        it->second.used_skill_ids = std::move(existing_skills);
        it->second.total_damage = existing_damage;
        it->second.attacks_started = existing_attacks_started;
        it->second.attacks_finished = existing_attacks_finished;
        it->second.attacks_stopped = existing_attacks_stopped;
        it->second.skills_activated = existing_skills_activated;
        it->second.skills_finished = existing_skills_finished;
        it->second.skills_stopped = existing_skills_stopped;
        it->second.attack_skills_activated = existing_attack_skills_activated;
        it->second.attack_skills_finished = existing_attack_skills_finished;
        it->second.attack_skills_stopped = existing_attack_skills_stopped;
        it->second.interrupted_count = existing_interrupted_count;
        it->second.interrupted_skills_count = existing_interrupted_skills_count;
        it->second.cancelled_attacks_count = existing_cancelled_attacks_count;
        it->second.cancelled_skills_count = existing_cancelled_skills_count;
        it->second.crits_dealt = existing_crits_dealt;
        it->second.crits_received = existing_crits_received;
        it->second.deaths = existing_deaths;
    } else {
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
        // Check if the skill ID already exists in the vector (mimic set behavior)
        auto& skill_ids = it->second.used_skill_ids;
        if (std::find(skill_ids.begin(), skill_ids.end(), skill_id) == skill_ids.end()) {
            // Skill not found, add it
            skill_ids.push_back(skill_id);
            // Sort skills immediately when a new one is added
            SortAgentSkills(agent_id);
        }
    }
    // if agent not found, do nothing. agent info should be populated by ObserverLoop first.
}

void MatchInfo::SortAgentSkills(uint32_t agent_id) {
    auto it = agents_info.find(agent_id);
    if (it == agents_info.end()) return;

    auto& agent = it->second;
    auto& skills = agent.used_skill_ids;
    
    if (skills.empty()) return;
    
    // determine if the agent has an elite skill
    bool has_elite = false;
    uint32_t elite_profession = 0;
    
    for (uint32_t skill_id : skills) {
        GW::Skill* skill = GW::SkillbarMgr::GetSkillConstantData(static_cast<GW::Constants::SkillID>(skill_id));
        if (skill && skill->IsElite()) {
            has_elite = true;
            elite_profession = static_cast<uint32_t>(skill->profession);
            break;
        }
    }
    
    // second step: sort skills
    std::sort(skills.begin(), skills.end(), [&agent, has_elite, elite_profession](uint32_t skill_id1, uint32_t skill_id2) -> bool {
        // get skill data
        GW::Skill* skill1 = GW::SkillbarMgr::GetSkillConstantData(static_cast<GW::Constants::SkillID>(skill_id1));
        GW::Skill* skill2 = GW::SkillbarMgr::GetSkillConstantData(static_cast<GW::Constants::SkillID>(skill_id2));
        
        if (!skill1 || !skill2) {
            // if one skill has no data, place the one with data first
            return skill1 != nullptr;
        }
        
        uint32_t prof1 = static_cast<uint32_t>(skill1->profession);
        uint32_t prof2 = static_cast<uint32_t>(skill2->profession);
        bool is_elite1 = skill1->IsElite();
        bool is_elite2 = skill2->IsElite();
        
        // case 1: if we have an elite
        if (has_elite) {
            // the elite will always be first
            if (is_elite1 && !is_elite2) return true;
            if (!is_elite1 && is_elite2) return false;
            
            // skills of the same profession as the elite are prioritized
            bool is_elite_prof1 = (prof1 == elite_profession);
            bool is_elite_prof2 = (prof2 == elite_profession);
            if (is_elite_prof1 != is_elite_prof2) {
                return is_elite_prof1;
            }
            
            // same profession, sort by type
            if (skill1->type != skill2->type) {
                return skill1->type < skill2->type;
            }
            
            // same type, sort by ID
            return skill_id1 < skill_id2;
        }
        // case 2: no elite, sort by primary/secondary profession
        else {
            // primary profession first
            bool is_primary1 = (prof1 == agent.primary_profession);
            bool is_primary2 = (prof2 == agent.primary_profession);
            if (is_primary1 != is_primary2) {
                return is_primary1;
            }
            
            // same profession status, sort by type
            if (skill1->type != skill2->type) {
                return skill1->type < skill2->type;
            }
            
            // same type, sort by ID
            return skill_id1 < skill_id2;
        }
    });
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
        this->UpdateAgentSkillTemplates();

        std::filesystem::path base_dir = "captures";
        std::filesystem::path match_dir = base_dir / folder_name;

        // ensure the base directory exists
        std::filesystem::create_directories(match_dir);

        // export infos.json using current_match_info_
        const MatchInfo& match_info = this->GetMatchInfo(); 
        std::map<uint32_t, AgentInfo> agents_info = match_info.GetAgentsInfoCopy();
        std::filesystem::path info_file = match_dir / "infos.json";
        std::ofstream outfile(info_file);
        if (outfile.is_open()) {             outfile << "{\n";            outfile << "  \"map_id\": " << match_info.map_id;
            outfile << ",\n";
                time_t t = std::time(nullptr);
                std::tm tm;
                localtime_s(&tm, &t);
                
                // flux
                std::vector<std::string> monthly_flux = {
                    "Odran's Razor",               // January (tm_mon = 0)
                    "Amateur Hour",                // February
                    "Hidden Talent",               // March
                    "There Can Be Only One",       // April
                    "Meek Shall Inherit",          // May
                    "Jack of All Trades",          // June
                    "Chain Combo",                 // July
                    "Xinrae's Revenge",            // August
                    "Like a Boss (and The Boss)",  // September
                    "Minion Apocalypse",           // October
                    "All In",                      // November
                    "Parting Gift (and Gift of Battle)" // December
                };
                std::string current_flux = "Unknown Flux"; // Default fallback
                if (tm.tm_mon >= 0 && tm.tm_mon < 12) {
                    current_flux = monthly_flux[tm.tm_mon];
                }
                outfile << ",\n";
                outfile << "  \"flux\": \"" << current_flux << "\"";
                
                // date
                outfile << ",\n";
                outfile << "  \"day\": " << tm.tm_mday;
                outfile << ",\n";
                outfile << "  \"month\": " << tm.tm_mon + 1; // tm_mon is 0-11, we need 1-12
                outfile << ",\n";
                outfile << "  \"year\": " << tm.tm_year + 1900; // tm_year is years since 1900
                
                // occasion
                outfile << ",\n";
                outfile << "  \"occasion\": \"General Scrimmage\"";

                // match duration (adjusted - minus 1 min)
                if (!match_info.match_duration.empty()) {
                    std::string utf8_duration;
                    try {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, match_info.match_duration.c_str(), (int)match_info.match_duration.size(), NULL, 0, NULL, NULL);
                        if (size_needed > 0) {
                            utf8_duration.resize(size_needed, 0);
                            WideCharToMultiByte(CP_UTF8, 0, match_info.match_duration.c_str(), (int)match_info.match_duration.size(), &utf8_duration[0], size_needed, NULL, NULL);
                        }
                    } catch (...) {
                        utf8_duration = "[conversion_error]";
                    }
                    outfile << ",\n";
                    outfile << "  \"match_duration\": \"" << utf8_duration << "\"";
                }
                
                // original match duration (without adjustment)
                if (!match_info.match_original_duration.empty()) {
                    std::string utf8_orig_duration;
                    try {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, match_info.match_original_duration.c_str(), (int)match_info.match_original_duration.size(), NULL, 0, NULL, NULL);
                        if (size_needed > 0) {
                            utf8_orig_duration.resize(size_needed, 0);
                            WideCharToMultiByte(CP_UTF8, 0, match_info.match_original_duration.c_str(), (int)match_info.match_original_duration.size(), &utf8_orig_duration[0], size_needed, NULL, NULL);
                        }
                    } catch (...) {
                        utf8_orig_duration = "[conversion_error]";
                    }
                    outfile << ",\n";
                    outfile << "  \"match_original_duration\": \"" << utf8_orig_duration << "\"";
                }
                
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

                // add team kill counts
                outfile << ",\n";
                outfile << "  \"team_kills\": {";
                uint32_t team1_kills = ObserverMatchData::GetTeamKillCount(1);
                uint32_t team2_kills = ObserverMatchData::GetTeamKillCount(2);
                outfile << "\n";
                outfile << "    \"1\": " << team1_kills << ",\n";
                outfile << "    \"2\": " << team2_kills << "\n";
                outfile << "  }";

                outfile << ",\n";
                outfile << "  \"team_damage\": {";
                long team1_damage = match_info.GetTeamDamage(1);
                long team2_damage = match_info.GetTeamDamage(2);
                outfile << "\n";
                outfile << "    \"1\": " << team1_damage << ",\n";
                outfile << "    \"2\": " << team2_damage << "\n";
                outfile << "  }";

                auto AgentTypeToJSONString = [](AgentType type) -> const char* {
                    switch (type) {
                        case AgentType::PLAYER: return "PLAYER";
                        case AgentType::HERO: return "PARTY_COMPLETE";
                        case AgentType::HENCHMAN: return "PARTY_COMPLETE";
                        case AgentType::PARTY_COMPLETE: return "PARTY_COMPLETE";
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

                        // decode and properly format the agent name for JSON
                        std::string decoded_name_json = ObserverUtils::DecodeAgentNameForJSON(agent.encoded_name);
                        
                        outfile << "{\"id\": " << agent.agent_id
                                << ", \"primary\": " << agent.primary_profession
                                << ", \"secondary\": " << agent.secondary_profession
                                << ", \"level\": " << agent.level
                                << ", \"team_id\": " << agent.team_id
                                << ", \"player_number\": " << agent.player_number
                                << ", \"guild_id\": " << agent.guild_id
                                << ", \"model_id\": " << agent.model_id
                                << ", \"gadget_id\": " << agent.gadget_id
                                << ", \"encoded_name\": " << decoded_name_json
                                << ", \"total_damage\": " << agent.total_damage
                                << ", \"attacks_started\": " << agent.attacks_started
                                << ", \"attacks_finished\": " << agent.attacks_finished
                                << ", \"attacks_stopped\": " << agent.attacks_stopped
                                << ", \"skills_activated\": " << agent.skills_activated
                                << ", \"skills_finished\": " << agent.skills_finished
                                << ", \"skills_stopped\": " << agent.skills_stopped
                                << ", \"attack_skills_activated\": " << agent.attack_skills_activated
                                << ", \"attack_skills_finished\": " << agent.attack_skills_finished
                                << ", \"attack_skills_stopped\": " << agent.attack_skills_stopped
                                << ", \"interrupted_count\": " << agent.interrupted_count
                                << ", \"interrupted_skills_count\": " << agent.interrupted_skills_count
                                << ", \"cancelled_attacks_count\": " << agent.cancelled_attacks_count
                                << ", \"cancelled_skills_count\": " << agent.cancelled_skills_count
                                << ", \"crits_dealt\": " << agent.crits_dealt
                                << ", \"crits_received\": " << agent.crits_received
                                << ", \"deaths\": " << agent.deaths;

                        if (!agent.skill_template_code.empty()) {
                            outfile << ", \"skill_template_code\": \"" << agent.skill_template_code << "\"";
                        }
                        
                        outfile << ", \"used_skills\": [";
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
                        std::string escaped_name = ObserverUtils::EscapeWideStringForJSON(guild_info.name);
                        std::string escaped_tag = ObserverUtils::EscapeWideStringForJSON(guild_info.tag);
                        outfile << "    \"" << guild_id << "\": {" << std::endl
                                << "      \"id\": " << guild_info.guild_id << "," << std::endl
                                << "      \"name\": " << escaped_name << "," << std::endl
                                << "      \"tag\": " << escaped_tag << "," << std::endl
                                << "      \"rank\": " << guild_info.rank << "," << std::endl
                                << "      \"features\": " << guild_info.features << "," << std::endl
                                << "      \"rating\": " << guild_info.rating << "," << std::endl
                                << "      \"faction\": " << guild_info.faction << "," << std::endl
                                << "      \"faction_points\": " << guild_info.faction_points << "," << std::endl
                                << "      \"qualifier_points\": " << guild_info.qualifier_points << "," << std::endl
                                << "      \"cape\": { " << std::endl
                                << "          \"bg_color\": " << guild_info.cape.cape_bg_color << "," << std::endl
                                << "          \"detail_color\": " << guild_info.cape.cape_detail_color << "," << std::endl
                                << "          \"emblem_color\": " << guild_info.cape.cape_emblem_color << "," << std::endl
                                << "          \"shape\": " << guild_info.cape.cape_shape << "," << std::endl
                                << "          \"detail\": " << guild_info.cape.cape_detail << "," << std::endl
                                << "          \"emblem\": " << guild_info.cape.cape_emblem << "," << std::endl
                                << "          \"trim\": " << guild_info.cape.cape_trim << std::endl
                                << "        }" << std::endl
                                << "      }";
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

        if (!owner_plugin) {
            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Error: ObserverPlugin instance not available for string conversion in export.");
            return false; 
        }

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

    this->UpdateAgentSkillTemplates();

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
        std::string name_s(owner_plugin->export_folder_name);
        std::wstring msg_w = L"Auto-resetting Match Name to '" + owner_plugin->StringToWString(name_s) + L"'.";
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, msg_w.c_str());
    } else {
         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Auto-reset name disabled.");
    }
}

void MatchInfo::Reset() {
    map_id = 0;
    end_time_ms = 0;
    end_time_formatted = L"";
    winner_party_id = 0;
    ClearAgentInfoMap(); 
    ClearGuildInfoMap();
    {
        std::lock_guard<std::mutex> lock(team_damage_mutex);
        team_damage.clear();
    }
}

void MatchInfo::ClearAgentInfoMap() {
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    agents_info.clear();
}

void MatchInfo::ClearGuildInfoMap() {
    std::lock_guard<std::mutex> lock(guilds_info_mutex);
    guilds_info.clear();
}

void MatchInfo::AddPlayerDamage(uint32_t agent_id, long damage) {
    if (agent_id == 0) return;
    
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.total_damage += damage;
        if (it->second.total_damage < 0) {
            it->second.total_damage = 0;
        }
    }
}

void MatchInfo::AddTeamDamage(uint32_t team_id, long damage) {
    if (team_id == 0) return;
    
    std::lock_guard<std::mutex> lock(team_damage_mutex);
    team_damage[team_id] += damage;
    if (team_damage[team_id] < 0) {
        team_damage[team_id] = 0;
    }
}

long MatchInfo::GetTeamDamage(uint32_t team_id) const {
    std::lock_guard<std::mutex> lock(team_damage_mutex);
    auto it = team_damage.find(team_id);
    return (it != team_damage.end()) ? it->second : 0L;
}

void MatchInfo::IncrementAttacksStarted(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.attacks_started++;
    }
}

void MatchInfo::IncrementAttacksFinished(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.attacks_finished++;
    }
}

void MatchInfo::IncrementAttacksStopped(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.attacks_stopped++;
    }
}

void MatchInfo::IncrementSkillsActivated(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.skills_activated++;
    }
}

void MatchInfo::IncrementSkillsFinished(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.skills_finished++;
    }
}

void MatchInfo::IncrementSkillsStopped(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.skills_stopped++;
    }
}

void MatchInfo::IncrementAttackSkillsActivated(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.attack_skills_activated++;
    }
}

void MatchInfo::IncrementAttackSkillsFinished(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.attack_skills_finished++;
    }
}

void MatchInfo::IncrementAttackSkillsStopped(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.attack_skills_stopped++;
    }
}

void MatchInfo::IncrementInterrupted(uint32_t agent_id, bool is_skill) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.interrupted_count++;
        if (is_skill) {
            it->second.interrupted_skills_count++;
        }
    }
}

void MatchInfo::IncrementCancelledAttack(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.cancelled_attacks_count++;
    }
}

void MatchInfo::IncrementCancelledSkill(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.cancelled_skills_count++;
    }
}

void MatchInfo::IncrementCritsDealt(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.crits_dealt++;
    }
}

void MatchInfo::IncrementCritsReceived(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.crits_received++;
    }
}

void MatchInfo::IncrementDeaths(uint32_t agent_id) {
    if (agent_id == 0) return;
    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it != agents_info.end()) {
        it->second.deaths++;
    }
}

void ObserverMatch::SetOwnerPlugin(ObserverPlugin* plugin) {
    owner_plugin = plugin;
}

bool ObserverMatch::IsObserving() const {
    return is_observing;
}

MatchInfo& ObserverMatch::GetMatchInfo() {
    return current_match_info_;
}

void MatchInfo::UpdateAgentSkillTemplate(uint32_t agent_id) {
    if (agent_id == 0) return;

    std::lock_guard<std::mutex> lock(agents_info_mutex);
    auto it = agents_info.find(agent_id);
    if (it == agents_info.end()) return;

    AgentInfo& agent = it->second;
    
    GW::SkillbarMgr::SkillTemplate skill_template;
    
    skill_template.primary = static_cast<GW::Constants::Profession>(agent.primary_profession);
    skill_template.secondary = static_cast<GW::Constants::Profession>(agent.secondary_profession);
      skill_template.attributes_count = 0;
    memset(skill_template.attribute_ids, 0, sizeof(skill_template.attribute_ids));
    memset(skill_template.attribute_values, 0, sizeof(skill_template.attribute_values));
    
    memset(skill_template.skills, 0, sizeof(skill_template.skills));
    const size_t max_skills = std::min(agent.used_skill_ids.size(), static_cast<size_t>(8));
    for (size_t i = 0; i < max_skills; i++) {
        uint32_t skill_id = agent.used_skill_ids[i];
        
        // here we convert PvP skills to PvE skills if needed
        GW::Skill* skill = GW::SkillbarMgr::GetSkillConstantData(static_cast<GW::Constants::SkillID>(skill_id));
        if (skill && skill->IsPvP() && static_cast<uint32_t>(skill->skill_id_pvp) != 0) {
            // if it's a PvP skill, we use skill_id_pvp which is actually the PvE version
            skill_id = static_cast<uint32_t>(skill->skill_id_pvp);
        }
        
        skill_template.skills[i] = static_cast<GW::Constants::SkillID>(skill_id);
    }
    
    char template_code[128] = {0};
    if (GW::SkillbarMgr::EncodeSkillTemplate(skill_template, template_code, sizeof(template_code))) {
        agent.skill_template_code = template_code;
    } else {
        agent.skill_template_code = "";
    }
}

// implementation of the function to update the skill templates of all agents
void ObserverMatch::UpdateAgentSkillTemplates() {
    std::map<uint32_t, AgentInfo> agents_copy = current_match_info_.GetAgentsInfoCopy();
    for (const auto& pair : agents_copy) {
        current_match_info_.UpdateAgentSkillTemplate(pair.first);
    }

}
