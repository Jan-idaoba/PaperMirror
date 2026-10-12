#include "pch.h"
#include "PAMonitorRegistry.h"

#include <shellscalingapi.h>

#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "user32.lib")

PAMonitorRegistry& PAMonitorRegistry::GetInstance()
{
    static PAMonitorRegistry instance;
    return instance;
}

BOOL CALLBACK PAMonitorRegistry::MonitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM lParam)
{
    auto* entries = reinterpret_cast<std::vector<PAMonitorEntry>*>(lParam);

    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hMon, &mi))
        return TRUE;

    UINT dpiX = 96, dpiY = 96;
    // Use EFFECTIVE_DPI so that paper size calculations stay in the same
    // logical-pixel space as rcMonitor / GetWindowRect / CreateWindowEx.
    // RAW_DPI is the monitor's physical PPI and only works correctly when the
    // host process is per-monitor-v2 DPI aware (all Win32 coordinates become
    // physical pixels). In system-DPI-aware or unaware hosts the window bitmap
    // lives in logical pixels, so using RAW_DPI makes the paper ~scale% too large.
    GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

    PAMonitorEntry e;
    e.index      = static_cast<int>(entries->size());
    e.hMonitor   = hMon;
    e.rect       = mi.rcMonitor;
    e.dpiX       = dpiX;
    e.dpiY       = dpiY;
    e.isPrimary  = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    e.deviceName = mi.szDevice;
    entries->push_back(std::move(e));
    return TRUE;
}

void PAMonitorRegistry::Refresh()
{
    std::vector<PAMonitorEntry> fresh;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&fresh));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries = std::move(fresh);
}

int PAMonitorRegistry::GetMonitorCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_entries.size());
}

bool PAMonitorRegistry::GetEntry(int index, PAMonitorEntry* out) const
{
    if (!out) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index < 0 || index >= static_cast<int>(m_entries.size()))
        return false;
    *out = m_entries[index];
    return true;
}

bool PAMonitorRegistry::GetPrimaryEntry(PAMonitorEntry* out) const
{
    if (!out) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : m_entries)
    {
        if (e.isPrimary)
        {
            *out = e;
            return true;
        }
    }
    if (!m_entries.empty())
    {
        *out = m_entries[0];
        return true;
    }
    return false;
}
