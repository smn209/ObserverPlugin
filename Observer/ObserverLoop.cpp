#include "ObserverLoop.h"
#include "ObserverPlugin.h"
#include "ObserverMatch.h"

#include <GWCA/GWCA.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/GuildMgr.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/GameEntities/Item.h>
#include <GWCA/GameEntities/Guild.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/Constants/Constants.h>

#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath> 
#include <limits> 

#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/PartyContext.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/GameEntities/Player.h>

// shared export helpers from ObserverCapture.cpp
extern std::vector<unsigned char> compress_gzip(const std::string& data); // compress_gzip
extern std::string WStringToUTF8(const std::wstring& wstr); // WStringToUTF8
extern void WriteCompressedFile(const std::filesystem::path& path, const std::vector<unsigned char>& data); // WriteCompressedFile



bool ParseLogLine(const std::wstring& line, float& x, float& y, float& z, std::wstring& rest_of_data) {
    // parse the line to extract position and the rest of the data
    size_t first_semi = line.find(L';'); // find the first semicolon
    if (first_semi == std::wstring::npos) return false; // if no semicolon found, return false
    size_t second_semi = line.find(L';', first_semi + 1); // find the second semicolon
    if (second_semi == std::wstring::npos) return false; // if no second semicolon found, return false
    size_t third_semi = line.find(L';', second_semi + 1); // find the third semicolon
    if (third_semi == std::wstring::npos) return false; // if no third semicolon found, return false

    try {
        // skip timestamp part
        size_t timestamp_end = line.find(L']'); // find the end of the timestamp
        if (timestamp_end == std::wstring::npos || timestamp_end >= first_semi) return false; // if no timestamp or timestamp is after first semicolon, return false
        size_t data_start = timestamp_end + 2; // skip '] ' 
        
        x = std::stof(line.substr(data_start, first_semi - data_start)); // convert the substring to a float
        y = std::stof(line.substr(first_semi + 1, second_semi - first_semi - 1)); // convert the substring to a float
        z = std::stof(line.substr(second_semi + 1, third_semi - second_semi - 1)); // convert the substring to a float
        rest_of_data = line.substr(third_semi + 1); // get the rest of the data
        return true; 
    } catch (const std::invalid_argument&) { // if the conversion fails, return false
        return false;
    } catch (const std::out_of_range&) { // if the value is out of range, return false
        return false;
    }
}

float ManhattanDistance(float x1, float y1, float z1, float x2, float y2, float z2) {
    return std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(z1 - z2); // calculate the Manhattan distance between two points
}

ObserverLoop::ObserverLoop(ObserverPlugin* owner_plugin, ObserverMatch* match_handler) 
    : owner_(owner_plugin), match_handler_(match_handler), run_loop_(false) {
}

ObserverLoop::~ObserverLoop() {
    Stop();
}

void ObserverLoop::Start() {
    // avoid starting if already running
    if (run_loop_.load()) return;

    // clear previous logs when starting new session
    ClearAgentLogs(); // this now also clears last_log_entry_
    
    // start the background thread
    run_loop_ = true;
    loop_thread_ = std::thread(&ObserverLoop::RunLoop, this);
}

void ObserverLoop::Stop() {
    // signal thread to stop
    run_loop_ = false;
    
    // wait for thread to terminate if it's running
    if (loop_thread_.joinable()) {
        loop_thread_.join();
    }
    // clear last entries map when stopping to ensure fresh data on next start
    std::lock_guard<std::mutex> lock(log_mutex_);
    last_agent_state_.clear();
}

void ObserverLoop::ClearAgentLogs() {
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        agent_logs_.clear();
        last_agent_state_.clear();
    }
    // clear party logs via match_handler_
    if (match_handler_) {
        match_handler_->GetMatchInfo().ClearAgentInfoMap();
    }
}

bool ObserverLoop::ExportAgentLogs(const wchar_t* folder_name) {
    if (!owner_) return false;
    
    // create a local copy of the logs to avoid locking during entire export
    std::map<uint32_t, std::vector<std::pair<uint32_t, AgentState>>> logs_copy;
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        // only copy if there are logs to prevent unnecessary work
        if (!agent_logs_.empty()){
            logs_copy = agent_logs_;
        } else {
             // if no logs, report and exit early
             if (owner_) {
                GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"No agent logs to export.");
             }
             return false; 
        }
    } // mutex released here
    
    // check again if the copy is empty (although the previous check should handle this)
    if (logs_copy.empty()) {
        return false;
    }
    
    try {
        std::filesystem::path base_dir = "captures";
        std::filesystem::path match_dir = base_dir / folder_name;
        std::filesystem::path agents_dir = match_dir / "Agents";
        
        // create directories if they don't exist
        std::filesystem::create_directories(agents_dir);
        
        // export each agent's logs to its own file
        for (const auto& [agent_id, log_entries] : logs_copy) {
            // format log entries from AgentState structs
            std::wstringstream buffer;
            buffer << std::fixed << std::setprecision(3); 
            for (const auto& entry_pair : log_entries) {
                const uint32_t timestamp_ms = entry_pair.first;
                const AgentState& state = entry_pair.second;

                // format timestamp
                wchar_t timestamp[32];
                swprintf(timestamp, 32, L"[%02u:%02u.%03u] ", 
                         (timestamp_ms / 1000) / 60, 
                         (timestamp_ms / 1000) % 60, 
                         timestamp_ms % 1000);
                buffer << timestamp;

                // format AgentState back into the semicolon-delimited string
                buffer << state.x << L";" << state.y << L";" << state.z << L";"
                       << state.rotation_angle << L";" << state.weapon_id << L";"
                       << (state.is_alive ? L"1" : L"0") << L";"
                       << (state.is_dead ? L"1" : L"0") << L";"
                       << state.health_pct << L";"
                       << (state.is_knocked ? L"1" : L"0") << L";"
                       << state.max_hp << L";"
                       << (state.has_condition ? L"1" : L"0") << L";"
                       << (state.has_deep_wound ? L"1" : L"0") << L";"
                       << (state.has_bleeding ? L"1" : L"0") << L";"
                       << (state.has_crippled ? L"1" : L"0") << L";"
                       << (state.has_blind ? L"1" : L"0") << L";"
                       << (state.has_poison ? L"1" : L"0") << L";"
                       << (state.has_hex ? L"1" : L"0") << L";"
                       << (state.has_degen_hex ? L"1" : L"0") << L";"
                       << (state.has_enchantment ? L"1" : L"0") << L";"
                       << (state.has_weapon_spell ? L"1" : L"0") << L";"
                       << (state.is_holding ? L"1" : L"0") << L";"
                       << (state.is_casting ? L"1" : L"0") << L";"
                       << state.skill_id;

                buffer << L"\n";
            }
            
            // create file name using agent ID
            std::wstring filename = std::to_wstring(agent_id) + L".txt.gz";
            std::filesystem::path agent_file = agents_dir / filename;
            
            // compress and write data
            WriteCompressedFile(agent_file, compress_gzip(WStringToUTF8(buffer.str())));
        }
        
        return true;
    } catch (const std::exception& e) {
        if (owner_) {
            wchar_t werror_msg[512] = {0}; // initialize buffer
            std::string error_msg = "Error exporting agent logs: "; // error message
            error_msg += e.what(); // add the error message
            MultiByteToWideChar(CP_UTF8, 0, error_msg.c_str(), -1, werror_msg, 512); // convert the error message to a wide string
            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, werror_msg); // write the error message to the chat
        }
        return false;
    }
}

void ObserverLoop::RunLoop() {
    const uint32_t kLoopInterval = 200; // milliseconds between updates
    const float kPositionThreshold = 30.0f;
    const float kDistanceThresholdSq = kPositionThreshold * kPositionThreshold; 
    
    while (run_loop_.load()) {
        uint32_t instance_time_ms = GW::Map::GetInstanceTime(); // get the instance time
        if (instance_time_ms == 0 && GW::Map::GetInstanceType() == GW::Constants::InstanceType::Loading) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kLoopInterval));
            continue;
        } // if the instance time is 0 and the instance type is loading, sleep for the loop interval
        
        GW::AgentArray* agents = GW::Agents::GetAgentArray(); // get the agent array
        if (agents && agents->valid()) { // check if the agent array is valid
            std::lock_guard<std::mutex> lock(log_mutex_); // lock the log mutex
            for (size_t i = 0; i < agents->size(); i++) { // iterate through the agents
                GW::Agent* agent = (*agents)[i]; // get the agent
                if (!agent) continue; // if the agent is invalid, continue
                
                uint32_t current_agent_id = agent->agent_id; // get the agent id
                AgentState current_state = GetAgentState(agent); // get the agent state

                auto it = last_agent_state_.find(current_agent_id); // find the agent in the last agent state
                bool should_log = true; // should log is true
                if (it != last_agent_state_.end()) { // if the agent is in the last agent state
                    const AgentState& last_state = it->second; // get the last state
                    
                    float dx = current_state.x - last_state.x; // calculate the distance between the current and last state
                    float dy = current_state.y - last_state.y;
                    float dz = current_state.z - last_state.z;
                    float distSq = dx*dx + dy*dy + dz*dz;

                    // create temporary copies excluding position for full state comparison
                    AgentState current_state_no_pos = current_state;
                    current_state_no_pos.x = current_state_no_pos.y = current_state_no_pos.z = 0.0f;
                    AgentState last_state_no_pos = last_state;
                    last_state_no_pos.x = last_state_no_pos.y = last_state_no_pos.z = 0.0f;

                    if (distSq < kDistanceThresholdSq && current_state_no_pos == last_state_no_pos) {
                        // only position changed slightly, and rest of state is identical
                        should_log = false; // don't log this minor movement
                    }
                }

                if (should_log) {
                    agent_logs_[current_agent_id].push_back({instance_time_ms, current_state}); // add the current state to the agent logs
                    last_agent_state_[current_agent_id] = current_state; // update the last agent state
                }
            }
        }

        UpdatePartiesInformations(); 

        std::this_thread::sleep_for(std::chrono::milliseconds(kLoopInterval));
    }
}

void ObserverLoop::UpdatePartiesInformations() {
    if (!match_handler_) return;

    GW::PartyContext* party_ctx = GW::GetGameContext() ? GW::GetGameContext()->party : nullptr;
    if (!party_ctx || !party_ctx->parties.valid()) {
        return; // cannot get party context
    }

    GW::PlayerArray* players = GW::Agents::GetPlayerArray(); 
    if (!players || !players->valid()) {
        return; // cannot get player array
    }

    MatchInfo& match_info = match_handler_->GetMatchInfo(); // get reference to match info

    for (const GW::PartyInfo* party_info : party_ctx->parties) {
        if (!party_info) continue;

        uint32_t current_party_id = party_info->party_id;

        // get current players agent ids and informations
        if (party_info->players.valid()) { // check if the players array is valid
            for (const GW::PlayerPartyMember& p : party_info->players) { // iterate through the players
                const GW::Player& player = players->at(p.login_number); // get the player from the players array
                if (player.agent_id != 0) { // check if the player has an agent id
                    AgentInfo info;
                    GW::Agent* agent = GW::Agents::GetAgentByID(player.agent_id);

                    info.agent_id = player.agent_id;
                    info.party_id = current_party_id;
                    info.type = AgentType::PLAYER;
                    info.primary_profession = player.primary;
                    info.secondary_profession = player.secondary;
                    info.player_number = p.login_number;

                    if (agent && agent->GetIsLivingType()) {
                        PopulateLivingAgentDetails(static_cast<GW::AgentLiving*>(agent), info);
                    } else {
                        info.level = 0; 
                        info.team_id = 0;
                        info.guild_id = 0; 
                        info.encoded_name = L""; 
                    }

                    match_info.UpdateAgentInfo(info);
                    MaybeUpdateGuildInfo(info.guild_id, match_info);
                }
            }
        }

        // get heroes
        if (party_info->heroes.valid()) { // check if the hero array is valid
            for (const GW::HeroPartyMember& h : party_info->heroes) { // iterate through the heroes
                if (h.agent_id == 0) continue;
                AgentInfo info;
                GW::Agent* agent = GW::Agents::GetAgentByID(h.agent_id);

                info.agent_id = h.agent_id;
                info.party_id = current_party_id;
                info.type = AgentType::HERO;
                info.player_number = 0;

                if (agent && agent->GetIsLivingType()) {
                    PopulateLivingAgentDetails(static_cast<GW::AgentLiving*>(agent), info);
                } else {
                    info.primary_profession = 0;
                    info.secondary_profession = 0;
                    info.level = 0;
                    info.team_id = 0;
                    info.guild_id = 0;
                    info.encoded_name = L""; 
                }
                
                match_info.UpdateAgentInfo(info);
                MaybeUpdateGuildInfo(info.guild_id, match_info);
            }
        }

        // get henchmens
        if (party_info->henchmen.valid()) { // check if the henchman array is valid
            for (const GW::HenchmanPartyMember& h : party_info->henchmen) { // iterate through the henchmens
                if (h.agent_id == 0) continue;
                AgentInfo info;
                GW::Agent* agent = GW::Agents::GetAgentByID(h.agent_id);

                info.agent_id = h.agent_id;
                info.party_id = current_party_id;
                info.type = AgentType::HENCHMAN;
                info.player_number = 0; 

                if (agent && agent->GetIsLivingType()) {
                    PopulateLivingAgentDetails(static_cast<GW::AgentLiving*>(agent), info);
                } else {
                    info.primary_profession = 0;
                    info.secondary_profession = 0;
                    info.level = 0;
                    info.team_id = 0;
                    info.guild_id = 0;
                    info.encoded_name = L""; 
                }

                match_info.UpdateAgentInfo(info);
                MaybeUpdateGuildInfo(info.guild_id, match_info);
            }
        }

        // get others
        if (party_info->others.valid()) {
            for (GW::AgentID agent_id : party_info->others) {
                if (agent_id != 0) {
                    AgentInfo info;
                    GW::Agent* agent = GW::Agents::GetAgentByID(agent_id);

                    info.agent_id = agent_id;
                    info.party_id = current_party_id;
                    info.type = AgentType::OTHER;
                    info.player_number = 0; 

                    if (agent && agent->GetIsLivingType()) {
                        PopulateLivingAgentDetails(static_cast<GW::AgentLiving*>(agent), info);
                    } else {
                        info.primary_profession = 0;
                        info.secondary_profession = 0;
                        info.level = 0;
                        info.team_id = 0;
                        info.guild_id = 0;
                        info.encoded_name = L""; 
                    }

                    match_info.UpdateAgentInfo(info);
                    MaybeUpdateGuildInfo(info.guild_id, match_info);
                }
            }
        }
    }
}

AgentState ObserverLoop::GetAgentState(GW::Agent* agent) {
    AgentState state; // create default state
    if (!agent) return state; // return default state if agent is invalid
    
    // position and basic info
    state.x = agent->pos.x;
    state.y = agent->pos.y;
    state.z = agent->z;
    state.rotation_angle = agent->rotation_angle;
    
    // equipment and stats - simplified
    state.weapon_id = 0; // default weapon id
    
    // living agent specific data
    if (agent->GetIsLivingType()) {
        GW::AgentLiving* living = static_cast<GW::AgentLiving*>(agent);
        
        // health and status
        state.is_alive = !living->GetIsDead();
        state.is_dead = living->GetIsDead();
        state.health_pct = living->hp;
        state.is_knocked = living->GetIsKnockedDown();
        state.max_hp = living->max_hp;
        
        // condition flags
        state.has_condition = living->GetIsConditioned();
        state.has_deep_wound = living->GetIsDeepWounded(); 
        state.has_bleeding = living->GetIsBleeding(); 
        state.has_crippled = living->GetIsCrippled(); 
        state.has_blind = false; // placeholder - to implement later
        state.has_poison = living->GetIsPoisoned();
        state.has_hex = living->GetIsHexed();
        state.has_degen_hex = living->GetIsDegenHexed(); 
        
        // buff status
        state.has_enchantment = living->GetIsEnchanted();
        state.has_weapon_spell = living->GetIsWeaponSpelled();
        
        // action status
        state.is_holding = (living->model_state & 0x400) != 0;
        state.is_casting = living->GetIsCasting(); 
        state.skill_id = living->skill; 

    } 
    
    return state; 
} 

void ObserverLoop::PopulateLivingAgentDetails(GW::AgentLiving* living, AgentInfo& info) {
    if (!living) return; 

    info.primary_profession = living->primary;
    info.secondary_profession = living->secondary;
    
    info.level = living->level;
    info.team_id = living->team_id;

    if (living->tags) { 
        info.guild_id = living->tags->guild_id;
    } else {
        info.guild_id = 0; 
    }

    info.encoded_name = GW::Agents::GetAgentEncName(living); 
}

void ObserverLoop::MaybeUpdateGuildInfo(uint16_t guild_id, MatchInfo& match_info) {
    if (guild_id == 0) return; // no guild to check

    bool guild_known = false;
    {
        std::lock_guard<std::mutex> lock(match_info.guilds_info_mutex);
        guild_known = match_info.guilds_info.count(guild_id);
    }

    if (!guild_known) {
        GW::Guild* gw_guild = GW::GuildMgr::GetGuildInfo(guild_id);
        if (gw_guild) {
            GuildInfo guild_info;
            guild_info.guild_id = guild_id;
            guild_info.name = gw_guild->name;   
            guild_info.tag = gw_guild->tag;    
            guild_info.rank = gw_guild->rank;
            guild_info.features = gw_guild->features;
            guild_info.rating = gw_guild->rating;
            guild_info.faction = gw_guild->faction;
            guild_info.faction_points = gw_guild->faction_point; 
            guild_info.qualifier_points = gw_guild->qualifier_point;
            guild_info.cape = gw_guild->cape;
            
            match_info.UpdateGuildInfo(guild_info); 
        }

    }
}

bool ObserverLoop::IsRunning() const {
    return run_loop_.load();
} 