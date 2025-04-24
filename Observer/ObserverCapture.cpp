#include <windows.h>
#include "ObserverCapture.h"
#include "ObserverStoC.h" // include for marker definitions

#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/ChatMgr.h>

#include <filesystem>
#include <fstream>
#include <ctime>
#include <cstdio>
#include <stringapiset.h>
#include <vector>
#include <string>
#include <sstream> // for wstringstream
#include <zlib.h>  // for gzip compression (ensure zlib is linked)
#include <stdexcept> // for runtime_error
#include <map> // for storing category buffers

ObserverCapture::ObserverCapture() {
}

void ObserverCapture::AddLogEntry(const wchar_t* entry) {
    uint32_t instance_time_ms = GW::Map::GetInstanceTime();  // get instance time in milliseconds
    uint32_t total_seconds = instance_time_ms / 1000;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    uint32_t milliseconds = instance_time_ms % 1000;

    // format the timestamp as [mm:ss.ms]
    wchar_t timestamp[32];
    swprintf(timestamp, 32, L"[%02u:%02u.%03u] ", minutes, seconds, milliseconds);

    // add timestamped entry (with its prepended marker) to the log
    std::wstring log_entry = timestamp;
    log_entry += entry; // entry already contains the marker, e.g. "[skl] skill activated..."
    match_log_entries.push_back(log_entry);
}

void ObserverCapture::ClearLogs() {
    // clear the log entries
    match_log_entries.clear();
    GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Observer logs cleared.");
}

// helper function to compress data using zlib (gzip format)
// returns a vector of bytes containing the compressed data
std::vector<unsigned char> compress_gzip(const std::string& data) {
    if (data.empty()) {
        return {}; // return empty vector if input is empty
    }

    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    // initialize for gzip compression (windowbits = 15 + 16 for gzip header)
    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw(std::runtime_error("deflateInit2 failed while compressing."));
    }

    zs.next_in = (Bytef*)data.data();
    zs.avail_in = data.size();

    int ret;
    std::vector<unsigned char> outbuffer(32768); // start with a reasonable buffer size
    std::vector<unsigned char> compressed_data;

    do {
        zs.next_out = outbuffer.data();
        zs.avail_out = outbuffer.size();

        // perform the compression step, z_finish indicates the last block of input.
        ret = deflate(&zs, Z_FINISH);

        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR) {
             deflateEnd(&zs);
             throw(std::runtime_error("exception during gzip compression: deflate failed (" + std::to_string(ret) + ")"));
        }

        size_t have = outbuffer.size() - zs.avail_out;
        if (have > 0) {
            compressed_data.insert(compressed_data.end(), outbuffer.data(), outbuffer.data() + have);
        }

    } while (zs.avail_out == 0); // continue if the buffer was filled

    // check if deflation stream finished properly.
     if (ret != Z_STREAM_END) {
         // z_finish should lead to z_stream_end. if not, it might indicate an issue,
         // but cleanup with deflateend is generally sufficient.
     }

    deflateEnd(&zs);

    return compressed_data;
}

// helper to convert wstring to utf-8 string
std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) {
        throw std::runtime_error("WideCharToMultiByte failed to get size for UTF-8 conversion.");
    }
    std::string strTo(size_needed, 0);
    int bytes_converted = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    if (bytes_converted <= 0) {
         throw std::runtime_error("WideCharToMultiByte failed during UTF-8 conversion.");
    }
    return strTo;
}

// helper to write compressed data to a file
void WriteCompressedFile(const std::filesystem::path& path, const std::vector<unsigned char>& data) {
    if (data.empty()) return; // don't write empty files

    std::ofstream outfile(path, std::ios::binary | std::ios::out);
    if (!outfile.is_open()) {
         std::wstring error_msg = L"Failed to open output file for writing: ";
         error_msg += path.wstring();
         // throw to be caught by the main export function's try-catch block.
         throw std::runtime_error("File opening error.");
    }
    outfile.write(reinterpret_cast<const char*>(data.data()), data.size());
    outfile.close();
    // check for stream errors after closing.
    if (!outfile) {
        std::wstring error_msg = L"Error occurred while writing to file: ";
        error_msg += path.wstring();
        // throw to be caught by the main export function's try-catch block.
        throw std::runtime_error("File writing error.");
    }
}

// exports accumulated logs into separate gzip-compressed files based on category markers.
bool ObserverCapture::ExportLogsToFolder(const wchar_t* folder_name) {
    /* structure:
     captures/
      - folder_name/
          - stoc/
              - skill_events.txt.gz
              - attack_skill_events.txt.gz
              - basic_attack_events.txt.gz
              - combat_events.txt.gz
              - agent_events.txt.gz
              - jumbo_messages.txt.gz
              - unknown_events.txt.gz
    */
    if (match_log_entries.empty()) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"No logs to export.");
        return false;
    }

    try {
        std::filesystem::path base_dir = "captures"; // base directory for all captures
        std::filesystem::path match_dir = base_dir / folder_name; // specific dir for this match/export
        std::filesystem::path stoc_dir = match_dir / "StoC"; // subdir for stoc event categories

        // create directories if they don't exist
        std::filesystem::create_directories(stoc_dir);
        std::filesystem::path abs_match_path = std::filesystem::absolute(match_dir); // for success message

        // use buffers to collect logs for each category before compression.
        std::map<std::wstring, std::wstringstream> category_buffers;
        category_buffers[L"skill"] = std::wstringstream();
        category_buffers[L"attack_skill"] = std::wstringstream();
        category_buffers[L"basic_attack"] = std::wstringstream();
        category_buffers[L"combat"] = std::wstringstream();
        category_buffers[L"agent"] = std::wstringstream();
        category_buffers[L"jumbo"] = std::wstringstream();
        category_buffers[L"unknown"] = std::wstringstream();

        for (const auto& entry : match_log_entries) {
            size_t first_space = entry.find(L' ');
            if (first_space == std::wstring::npos) continue; // skip malformed entries

            std::wstring timestamp = entry.substr(0, first_space + 1); // include the space
            std::wstring data = entry.substr(first_space + 1); // marker + message
            std::wstring message_content; // content without marker
            std::wstring category_key = L"unknown"; // default category

            // identify marker and assign log entry to the correct category buffer.
            if (data.rfind(MARKER_SKILL_EVENT, 0) == 0) {
                 category_key = L"skill";
                 message_content = data.substr(MARKER_SKILL_EVENT_LEN);
            } else if (data.rfind(MARKER_ATTACK_SKILL_EVENT, 0) == 0) {
                category_key = L"attack_skill";
                message_content = data.substr(MARKER_ATTACK_SKILL_EVENT_LEN);
            } else if (data.rfind(MARKER_BASIC_ATTACK_EVENT, 0) == 0) {
                category_key = L"basic_attack";
                 message_content = data.substr(MARKER_BASIC_ATTACK_EVENT_LEN);
            } else if (data.rfind(MARKER_COMBAT_EVENT, 0) == 0) {
                 category_key = L"combat";
                 message_content = data.substr(MARKER_COMBAT_EVENT_LEN);
            } else if (data.rfind(MARKER_AGENT_EVENT, 0) == 0) {
                 category_key = L"agent";
                 message_content = data.substr(MARKER_AGENT_EVENT_LEN);
            } else if (data.rfind(MARKER_JUMBO_EVENT, 0) == 0) {
                 category_key = L"jumbo";
                 message_content = data.substr(MARKER_JUMBO_EVENT_LEN);
            } else {
                 // for unknown category, keep the full data part including potential unknown marker.
                 message_content = data; // use full data part
                 category_key = L"unknown";
            }

            // append timestamp, message content, and newline to the correct category buffer.
            category_buffers[category_key] << timestamp << message_content << L"\n";
        }

        // compress and write each category buffer to its respective file.
        WriteCompressedFile(stoc_dir / "skill_events.txt.gz", compress_gzip(WStringToUTF8(category_buffers[L"skill"].str())));
        WriteCompressedFile(stoc_dir / "attack_skill_events.txt.gz", compress_gzip(WStringToUTF8(category_buffers[L"attack_skill"].str())));
        WriteCompressedFile(stoc_dir / "basic_attack_events.txt.gz", compress_gzip(WStringToUTF8(category_buffers[L"basic_attack"].str())));
        WriteCompressedFile(stoc_dir / "combat_events.txt.gz", compress_gzip(WStringToUTF8(category_buffers[L"combat"].str())));
        WriteCompressedFile(stoc_dir / "agent_events.txt.gz", compress_gzip(WStringToUTF8(category_buffers[L"agent"].str())));
        WriteCompressedFile(stoc_dir / "jumbo_messages.txt.gz", compress_gzip(WStringToUTF8(category_buffers[L"jumbo"].str())));
        WriteCompressedFile(stoc_dir / "unknown_events.txt.gz", compress_gzip(WStringToUTF8(category_buffers[L"unknown"].str())));

        std::wstring success_msg = L"StoC logs exported and compressed to folder: ";
        success_msg += abs_match_path.wstring();
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, success_msg.c_str());

        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::string error_msg_str = "Filesystem error during export: ";
        error_msg_str += e.what();
        // convert narrow error string to wide string for writechat.
         wchar_t werror_msg[512] = {0}; // initialize buffer
         MultiByteToWideChar(CP_UTF8, 0, error_msg_str.c_str(), -1, werror_msg, 512);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, werror_msg);
        return false;
    } catch (const std::exception& e) {
        std::string error_msg_str = "Generic error during export: ";
        error_msg_str += e.what();
        // convert narrow error string to wide string for writechat.
        wchar_t werror_msg[512] = {0}; // initialize buffer
        MultiByteToWideChar(CP_UTF8, 0, error_msg_str.c_str(), -1, werror_msg, 512);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, werror_msg);
        return false;
    } catch (...) {
        // catch-all for unexpected errors (e.g., from helper functions).
         GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"An unexpected error occurred during export.");
         return false;
    }
} 