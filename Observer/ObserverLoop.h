#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>

class ObserverPlugin;
namespace GW { struct Agent; }

// logs agent state periodically during observer mode
class ObserverLoop {
public:
    ObserverLoop(ObserverPlugin* owner_plugin);
    ~ObserverLoop();

    void Start(); // starts the background logging thread
    void Stop();  // signals the background thread to stop and joins it

    
    bool ExportAgentLogs(const wchar_t* folder_name); // exports accumulated agent logs into separate gzip files per agent
    void ClearAgentLogs(); // clears all accumulated agent logs

private:
    void RunLoop(); // the function executed by the background thread
    void AddAgentLogEntry(uint32_t agent_id, const std::wstring& entry); // thread-safe log addition
    std::wstring GetAgentLogLine(GW::Agent* agent); // formats a single log line for an agent

    ObserverPlugin* owner = nullptr; // pointer back to the main plugin instance
    std::thread loop_thread_;        // the background thread handle
    std::atomic<bool> run_loop_;     // flag to control the loop execution
    std::mutex log_mutex_;           // mutex to protect access to agent_logs_
    
    
    std::map<uint32_t, std::vector<std::wstring>> agent_logs_; // map storing log entries keyed by agent id
    std::map<uint32_t, std::wstring> last_log_entry_; // map storing the most recent log entry string for optimization
}; 