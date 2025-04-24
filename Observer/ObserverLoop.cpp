#include "ObserverLoop.h"
#include "ObserverPlugin.h"

#include <GWCA/GWCA.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/EffectMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Skill.h>
#include <GWCA/GameEntities/Item.h>
#include <GWCA/Constants/Skills.h>
#include <GWCA/Constants/Constants.h>

#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath> 
#include <limits> 

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

ObserverLoop::ObserverLoop(ObserverPlugin* owner_plugin) 
    : owner(owner_plugin), run_loop_(false) {
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
    // Clear last entries map when stopping to ensure fresh data on next start
    std::lock_guard<std::mutex> lock(log_mutex_);
    last_log_entry_.clear();
}

void ObserverLoop::ClearAgentLogs() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    agent_logs_.clear();
    last_log_entry_.clear(); // also clear the last entry map
}

bool ObserverLoop::ExportAgentLogs(const wchar_t* folder_name) {
    if (!owner) return false;
    
    // create a local copy of the logs to avoid locking during entire export
    std::map<uint32_t, std::vector<std::wstring>> logs_copy;
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        // only copy if there are logs to prevent unnecessary work
        if (!agent_logs_.empty()){
            logs_copy = agent_logs_;
        } else {
             // if no logs, report and exit early
             if (owner) {
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
            // join log entries with newlines
            std::wstringstream buffer;
            for (const auto& entry : log_entries) {
                buffer << entry << L"\n";
            }
            
            // create file name using agent ID
            std::wstring filename = std::to_wstring(agent_id) + L".txt.gz";
            std::filesystem::path agent_file = agents_dir / filename;
            
            // compress and write data
            WriteCompressedFile(agent_file, compress_gzip(WStringToUTF8(buffer.str())));
        }
        
        return true;
    } catch (const std::exception& e) {
        if (owner) {
            wchar_t werror_msg[512] = {0}; // initialize buffer
            std::string error_msg = "Error exporting agent logs: "; // error message
            error_msg += e.what(); // add the error message
            MultiByteToWideChar(CP_UTF8, 0, error_msg.c_str(), -1, werror_msg, 512); // convert the error message to a wide string
            GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, werror_msg); // write the error message to the chat
        }
        return false;
    }
}

// Note: AddAgentLogEntry is now internal helper, not meant to be called directly
// void ObserverLoop::AddAgentLogEntry(uint32_t agent_id, const std::wstring& entry) {
//     std::lock_guard<std::mutex> lock(log_mutex_);
//     agent_logs_[agent_id].push_back(entry);
// }

void ObserverLoop::RunLoop() {
    const uint32_t kLoopInterval = 200; // milliseconds between updates
    const float kPositionThreshold = 30.0f;
    
    while (run_loop_.load()) {
        // skip if GWCA is not available or not initialized
        
        // get current instance time for timestamp
        uint32_t instance_time_ms = GW::Map::GetInstanceTime();

        if (instance_time_ms == 0 && GW::Map::GetInstanceType() == GW::Constants::InstanceType::Loading) { // check if still loading
            std::this_thread::sleep_for(std::chrono::milliseconds(kLoopInterval)); // sleep for the loop interval
            continue; // skip to the next iteration of the loop
        }
        
        // format timestamp
        wchar_t timestamp[32];
        swprintf(timestamp, 32, L"[%02u:%02u.%03u] ", 
                 (instance_time_ms / 1000) / 60, 
                 (instance_time_ms / 1000) % 60, 
                 instance_time_ms % 1000);
        std::wstring timestamp_str = timestamp; // convert the timestamp to a wide string

        // get all agents
        GW::AgentArray* agents = GW::Agents::GetAgentArray();
        if (!agents || !agents->valid()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kLoopInterval));
            continue; // skip to the next iteration of the loop
        } 
        
        std::lock_guard<std::mutex> lock(log_mutex_); // lock for accessing agent_logs_ and last_log_entry_
        
        // log data for each agent
        for (size_t i = 0; i < agents->size(); i++) {
            GW::Agent* agent = (*agents)[i];
            if (!agent) continue; // skip if the agent is invalid
            
            uint32_t current_agent_id = agent->agent_id;
            std::wstring agent_data_str = GetAgentLogLine(agent); // get the data part without timestamp
            std::wstring current_log_line = timestamp_str + agent_data_str; // combine the timestamp and data

            // optimization check
            auto it = last_log_entry_.find(current_agent_id);
            if (it != last_log_entry_.end()) {
                // previous entry exists
                const std::wstring& last_line = it->second; // get the last line
                float last_x, last_y, last_z; // last position
                std::wstring last_rest_data; // last rest of the data
                float current_x, current_y, current_z; // current position
                std::wstring current_rest_data; // current rest of the data

                // check if parsing succeeds for both lines
                if (ParseLogLine(last_line, last_x, last_y, last_z, last_rest_data) &&
                    ParseLogLine(current_log_line, current_x, current_y, current_z, current_rest_data)) 
                {
                    float distance = ManhattanDistance(current_x, current_y, current_z, last_x, last_y, last_z);
                    
                        // if position difference is small AND the rest of the data is the same, skip logging
                    if (distance < kPositionThreshold && current_rest_data == last_rest_data) {
                        continue; // skip adding this log entry
                    }
                }
                // if parsing failed or data is different enough, proceed to log and update
            }
            
            // add to agent's log and update the last entry map
            agent_logs_[current_agent_id].push_back(current_log_line);
            last_log_entry_[current_agent_id] = current_log_line;
        }
        
        // mutex is released when lock_guard goes out of scope
        
        // sleep before next update
        std::this_thread::sleep_for(std::chrono::milliseconds(kLoopInterval));
    }
}

std::wstring ObserverLoop::GetAgentLogLine(GW::Agent* agent) {
    if (!agent) return L"INVALID_AGENT";
    
    std::wstringstream ss;
    
    // format: x;y;z;angle;weapon_id;is_alive;is_dead;health_pct;is_knocked;life_hp;
    // has_condition;has_deep_wound;has_bleeding;has_crippled;has_blind;has_poison;has_hex;has_degen_hex;
    // has_enchantment;has_weapon_spell;is_holding;is_casting;skill_id
    // Note: Timestamp is prepended later in RunLoop
    
    // position and basic info
    ss << agent->pos.x << L";" 
       << agent->pos.y << L";" 
       << agent->z << L";" // Use agent->z for Z coord
       << agent->rotation_angle << L";";
    
    // equipment and stats - simplified as this isn't readily available in standard GWCA
    uint32_t weapon_id = 0; // default weapon id : to implement later
    ss << weapon_id << L";"; // add the weapon id
    
    // living agent specific data
    if (agent->GetIsLivingType()) {
        GW::AgentLiving* living = static_cast<GW::AgentLiving*>(agent); // cast to living agent
        
        // health and status
        bool is_alive = !living->GetIsDead(); // check if the agent is alive
        bool is_dead = living->GetIsDead(); // check if the agent is dead
        float health_pct = living->hp; // use living->hp directly
        bool is_knocked = living->GetIsKnockedDown(); // use model state check
        uint32_t life_hp = living->max_hp; // use living->max_hp
        
        ss << (is_alive ? L"1" : L"0") << L";" 
           << (is_dead ? L"1" : L"0") << L";" 
           << std::fixed << std::setprecision(3) << health_pct << L";" // format health % 
           << (is_knocked ? L"1" : L"0") << L";"
           << life_hp << L";";
        
        // condition flags - using methods based on 'effects' bitmap
        bool has_condition = living->GetIsConditioned();
        bool has_deep_wound = living->GetIsDeepWounded(); 
        bool has_bleeding = living->GetIsBleeding(); 
        bool has_crippled = living->GetIsCrippled(); 
        bool has_blind = false; // no simple GetIsBlinded() in AgentLiving effects bitmap // to implement later
        bool has_poison = living->GetIsPoisoned();
        bool has_hex = living->GetIsHexed(); // 
        bool has_degen_hex = living->GetIsDegenHexed(); 
        
        // add the condition flags to the string stream
        ss << (has_condition ? L"1" : L"0") << L";" 
           << (has_deep_wound ? L"1" : L"0") << L";" 
           << (has_bleeding ? L"1" : L"0") << L";" 
           << (has_crippled ? L"1" : L"0") << L";" 
           << (has_blind ? L"1" : L"0") << L";" 
           << (has_poison ? L"1" : L"0") << L";" 
           << (has_hex ? L"1" : L"0") << L";" 
           << (has_degen_hex ? L"1" : L"0") << L";";
        
        // buff status - using methods based on 'effects' bitmap
        bool has_enchantment = living->GetIsEnchanted(); // use GetIsEnchanted
        bool has_weapon_spell = living->GetIsWeaponSpelled(); // use GetIsWeaponSpelled
        
        ss << (has_enchantment ? L"1" : L"0") << L";" 
           << (has_weapon_spell ? L"1" : L"0") << L";";
        
        // action status
        // check if agent is holding something (flag, item) based on model state
        bool is_holding = (living->model_state & 0x400) != 0; // use living->model_state
        
        bool is_casting = living->GetIsCasting(); 
        uint32_t skill_id = living->skill; 
        
        // add the action status to the string stream
        ss << (is_holding ? L"1" : L"0") << L";" 
           << (is_casting ? L"1" : L"0") << L";" 
           << skill_id;
    } else {
        // non-living agent - fill with default values for consistent CSV format
        ss << L"0;0;0;0;0;0;0.000;0;0;0;0;0;0;0;0;0;0;0;0;0;0;0"; // adjusted defaults for consistency
    }
    
    return ss.str();
} 

bool ObserverLoop::IsRunning() const {
    return run_loop_.load();
} 