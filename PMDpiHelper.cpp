#include "pch.h"
#include "PMDpiHelper.h"

#pragma comment(lib, "shcore.lib")

float PMDpiHelper::GetScaleForMonitor(HMONITOR hMonitor)
{
    UINT dpiX = 96, dpiY = 96;
    if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
        return dpiX / 96.0f;
    return 1.0f;
}

float PMDpiHelper::GetScaleForRect(const RECT& monitorRect)
{
    POINT pt = { monitorRect.left + 1, monitorRect.top + 1 };
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    return GetScaleForMonitor(hMon);
}
