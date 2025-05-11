#include "TextUtils.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <mutex>

namespace ObserverUtils {

    // buffer for decoding names - protected by mutex
    static std::mutex decoder_mutex;
    static std::vector<wchar_t> decoder_buffer(256, 0);

    std::string EscapeWideStringForJSON(const std::wstring& wstr) {
        // clean the string by removing control characters
        std::wstring cleaned;
        cleaned.reserve(wstr.length());
        
        // keep only printable ASCII and common Unicode characters
        for (wchar_t wc : wstr) {
            // ignore control characters and specific observed markers
            if (wc == 0x0ba9 || wc == 0x0107 || wc == 0x0001) {
                continue;
            }
            
            // keep printable characters
            if (wc >= 32) {
                cleaned += wc;
            }
        }
        
        // escape for JSON
        std::wstring result;
        result.reserve(cleaned.length() * 2);  // reserve more space for escapes
        
        for (wchar_t wc : cleaned) {
            switch (wc) {
                case L'\\': result += L"\\\\"; break;
                case L'"': result += L"\\\""; break;
                case L'/': result += L"\\/"; break;
                case L'\b': result += L"\\b"; break;
                case L'\f': result += L"\\f"; break;
                case L'\n': result += L"\\n"; break;
                case L'\r': result += L"\\r"; break;
                case L'\t': result += L"\\t"; break;
                default:
                    if (wc <= 126) {
                        // printable ASCII
                        result += wc;
                    } else {
                        // Unicode - use the format \uXXXX
                        std::wostringstream woss;
                        woss << L"\\u" << std::hex << std::setw(4) << std::setfill(L'0') << static_cast<int>(wc);
                        result += woss.str();
                    }
                    break;
            }
        }
        
        // convert to UTF-8
        if (result.empty()) {
            return "\"\"";
        }
        
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &result[0], (int)result.size(), NULL, 0, NULL, NULL);
        std::string utf8_str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &result[0], (int)result.size(), &utf8_str[0], size_needed, NULL, NULL);
        
        // return the JSON string properly enclosed in quotes
        return "\"" + utf8_str + "\"";
    }

    const wchar_t* DecodeAgentName(const std::wstring& encoded_name) {
        if (encoded_name.empty()) return L"";

        std::lock_guard<std::mutex> lock(decoder_mutex);

        // decode the name using GWCA
        GW::UI::AsyncDecodeStr(encoded_name.c_str(), decoder_buffer.data(), decoder_buffer.size());
        
        return decoder_buffer.data();
    }

    std::string DecodeAgentNameForJSON(const std::wstring& encoded_name) {
        if (encoded_name.empty()) return "\"\"";

        std::lock_guard<std::mutex> lock(decoder_mutex);

        // decode the name using GWCA
        GW::UI::AsyncDecodeStr(encoded_name.c_str(), decoder_buffer.data(), decoder_buffer.size());
        
        // check if decoding is finished
        if (wcscmp(decoder_buffer.data(), L"<Decoding...>") == 0) {
            // use the raw encoded name if decoding isn't finished
            return EscapeWideStringForJSON(encoded_name);
        }
        
        // return the decoded name properly escaped for JSON
        std::wstring decoded_name(decoder_buffer.data());
        return EscapeWideStringForJSON(decoded_name);
    }
} 