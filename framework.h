#pragma once

#define WIN32_LEAN_AND_MEAN
// Windows Header Files
#include <windows.h>
#include <windowsx.h>
#include <shellscalingapi.h>

// GDI+
#include <objidl.h>
#include <gdiplus.h>

// DPI awareness (needed for SetThreadDpiAwarenessContext)
#include <winuser.h>

// C++ Standard Library
#include <cstring>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include <functional>
