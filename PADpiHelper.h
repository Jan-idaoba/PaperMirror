#pragma once

#include <Windows.h>
#include <shellscalingapi.h>

class PADpiHelper
{
public:
    static float GetScaleForMonitor(HMONITOR hMonitor);
    static float GetScaleForRect(const RECT& monitorRect);
};
