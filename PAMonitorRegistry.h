#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <mutex>

#include "PrintAssitWndApi.h"

struct PAMonitorEntry
{
    int          index;
    HMONITOR     hMonitor;
    RECT         rect;
    UINT         dpiX;
    UINT         dpiY;
    bool         isPrimary;
    std::wstring deviceName;
};

class PAMonitorRegistry
{
public:
    static PAMonitorRegistry& GetInstance();

    // Refreshes the cached monitor list. Must be called on the UI thread,
    // either at init time or in response to WM_DISPLAYCHANGE.
    void Refresh();

    // Read accessors — safe from any thread.
    int  GetMonitorCount() const;
    bool GetEntry(int index, PAMonitorEntry* out) const;
    bool GetPrimaryEntry(PAMonitorEntry* out) const;

private:
    PAMonitorRegistry() = default;

    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC hdc, LPRECT rc, LPARAM lParam);

    mutable std::mutex          m_mutex;
    std::vector<PAMonitorEntry> m_entries;
};
