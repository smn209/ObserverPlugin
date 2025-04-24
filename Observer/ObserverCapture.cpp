#include <windows.h>
#include "ObserverCapture.h"
#include "ObserverStoC.h" // Include for marker definitions

#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/ChatMgr.h>

#include <filesystem>
#include <fstream>
#include <ctime>
#include <cstdio> 
#include <stringapiset.h> 
#include <vector>
#include <string>

ObserverCapture::ObserverCapture() {
}

void ObserverCapture::AddLogEntry(const wchar_t* entry) {
    uint32_t instance_time_ms = GW::Map::GetInstanceTime();  // get instance time in milliseconds
    uint32_t total_seconds = instance_time_ms / 1000;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    uint32_t milliseconds = instance_time_ms % 1000;

    // format the timestamp as [MM:SS.ms]
    wchar_t timestamp[32];
    swprintf(timestamp, 32, L"[%02u:%02u.%03u] ", minutes, seconds, milliseconds);

    // add timestamped entry (with its prepended marker) to the log
    std::wstring log_entry = timestamp;
    log_entry += entry; // entry already contains the marker, e.g. "[SKL] Skill Activated..."
    match_log_entries.push_back(log_entry);
}

void ObserverCapture::ClearLogs() {
    // clear the log entries
    match_log_entries.clear();
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer logs cleared.");
}

std::wofstream open_log_file(const std::filesystem::path& path) {
    // open the log file (if it exists, it will be overwritten)
    std::wofstream file(path, std::ios::out);
    if (!file.is_open()) {
        std::wstring error_msg = L"Failed to open log file: ";
        error_msg += path.wstring();
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, error_msg.c_str());
    }
    return file; // RVO should handle this efficiently
}

bool ObserverCapture::ExportLogsToFolder(const wchar_t* folder_name) {
    // export the logs to a structured folder hierarchy based on event category markers
    // structure : 
    // captures/
    //  - folder_name/
    //      - StoC/
    //          - skill_events.txt
    //          - attack_skill_events.txt
    //          - basic_attack_events.txt
    //          - combat_events.txt
    //          - agent_events.txt
    //          - jumbo_messages.txt
    //          - unknown_events.txt

    if (match_log_entries.empty()) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"No logs to export.");
        return false;
    }

    try {
        std::filesystem::path base_dir = "captures"; // base directory for all captures
        std::filesystem::path match_dir = base_dir / folder_name; // specific dir for this match/export
        std::filesystem::path stoc_dir = match_dir / "StoC"; // subdir for StoC event categories

        // create directories if they don't exist
        std::filesystem::create_directories(stoc_dir);
        std::filesystem::path abs_match_path = std::filesystem::absolute(match_dir); // for success message

        // file stream setup
        std::wofstream skill_file = open_log_file(stoc_dir / "skill_events.txt");
        std::wofstream attack_skill_file = open_log_file(stoc_dir / "attack_skill_events.txt");
        std::wofstream basic_attack_file = open_log_file(stoc_dir / "basic_attack_events.txt");
        std::wofstream combat_file = open_log_file(stoc_dir / "combat_events.txt");
        std::wofstream agent_file = open_log_file(stoc_dir / "agent_events.txt");
        std::wofstream jumbo_file = open_log_file(stoc_dir / "jumbo_messages.txt");
        std::wofstream unknown_file = open_log_file(stoc_dir / "unknown_events.txt"); 

        for (const auto& entry : match_log_entries) {
            // find the first space to separate timestamp from marker+message
            size_t first_space = entry.find(L' ');
            if (first_space == std::wstring::npos) continue; // skip malformed entries

            std::wstring timestamp = entry.substr(0, first_space + 1); // include the space
            std::wstring data = entry.substr(first_space + 1); // marker + message

            // identify marker and write to appropriate file
            if (data.rfind(MARKER_SKILL_EVENT, 0) == 0 && skill_file.is_open()) {
                skill_file << timestamp << data.substr(MARKER_SKILL_EVENT_LEN) << std::endl;
            } else if (data.rfind(MARKER_ATTACK_SKILL_EVENT, 0) == 0 && attack_skill_file.is_open()) {
                attack_skill_file << timestamp << data.substr(MARKER_ATTACK_SKILL_EVENT_LEN) << std::endl;
            } else if (data.rfind(MARKER_BASIC_ATTACK_EVENT, 0) == 0 && basic_attack_file.is_open()) {
                basic_attack_file << timestamp << data.substr(MARKER_BASIC_ATTACK_EVENT_LEN) << std::endl;
            } else if (data.rfind(MARKER_COMBAT_EVENT, 0) == 0 && combat_file.is_open()) {
                combat_file << timestamp << data.substr(MARKER_COMBAT_EVENT_LEN) << std::endl;
            } else if (data.rfind(MARKER_AGENT_EVENT, 0) == 0 && agent_file.is_open()) {
                agent_file << timestamp << data.substr(MARKER_AGENT_EVENT_LEN) << std::endl;
            } else if (data.rfind(MARKER_JUMBO_EVENT, 0) == 0 && jumbo_file.is_open()) {
                jumbo_file << timestamp << data.substr(MARKER_JUMBO_EVENT_LEN) << std::endl;
            } else if (unknown_file.is_open()) {
                // write the full entry (including potential unknown marker) if it doesn't match known ones
                unknown_file << entry << std::endl;
            }
        }

        // cleanup and report
        // files are closed automatically when wofstream objects go out of scope

        std::wstring success_msg = L"Logs exported to folder: ";
        success_msg += abs_match_path.wstring();
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, success_msg.c_str());

        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::string error_msg_str = "Filesystem error during export: ";
        error_msg_str += e.what();
        wchar_t werror_msg[512];
        MultiByteToWideChar(CP_UTF8, 0, error_msg_str.c_str(), -1, werror_msg, 512);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, werror_msg);
        return false;
    } catch (const std::exception& e) {
        std::string error_msg_str = "Generic error during export: ";
        error_msg_str += e.what();
        wchar_t werror_msg[512];
        MultiByteToWideChar(CP_UTF8, 0, error_msg_str.c_str(), -1, werror_msg, 512);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, werror_msg);
        return false;
    }
} 