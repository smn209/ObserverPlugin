#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>

#include <GWCA/Managers/UIMgr.h>

namespace ObserverUtils {
    
    /**
     * @brief decode an encoded agent name and format it for JSON display
     * 
     * @param encoded_name the encoded name to decode
     * @return std::string the decoded name, escaped and enclosed in quotes for JSON
     */
    std::string DecodeAgentNameForJSON(const std::wstring& encoded_name);
    
    /**
     * @brief decode an encoded agent name for display in the interface
     * 
     * @param encoded_name the encoded name to decode
     * @return const wchar_t* pointer to the decoded name (static buffer)
     */
    const wchar_t* DecodeAgentName(const std::wstring& encoded_name);
    
    /**
     * @brief escape a wide string for JSON
     * 
     * @param wstr the wide string to escape
     * @return std::string the escaped string formatted for JSON
     */
    std::string EscapeWideStringForJSON(const std::wstring& wstr);
} 