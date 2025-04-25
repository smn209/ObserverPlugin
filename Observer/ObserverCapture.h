#pragma once

#include <vector>
#include <string>
#include <cstdint>

class ObserverCapture {
public:
    ObserverCapture();
    ~ObserverCapture() = default;

    void AddLogEntry(const wchar_t* entry); 
    void ClearLogs();
    bool ExportLogsToFolder(const wchar_t* folder_name);

    size_t GetLogCount() const;
    const std::vector<std::wstring>& GetLogEntries() const;
    const std::vector<std::wstring>& GetLogs() const;

private:
    std::vector<std::wstring> match_log_entries;
}; 