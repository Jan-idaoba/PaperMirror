#pragma once

#include <Windows.h>
#include <mutex>

#include "PrintAssitWndApi.h"
#include "PAGridRenderer.h"
#include "PAMonitorRegistry.h"

class PAOverlayWindow
{
public:
    PAOverlayWindow();
    ~PAOverlayWindow();

    bool Create(const PA_WindowParams& params, const PAMonitorEntry& entry);
    void Modify(const PA_WindowParams& params);
    void AdaptToMonitor(HMONITOR hMon);
    void Close();

    HWND GetHWND() const { return m_hWnd; }

    static bool RegisterWindowClass(HINSTANCE hInstance);

    void Render();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnMouseDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnMouseUp();
    void OnKeyDown(WPARAM vk);
    void OnTimer(UINT_PTR nIDEvent);

    bool IsPointInPaper(int x, int y) const;
    bool IsPointInButton(int x, int y, const RECT& btnRect) const;

private:
    HWND               m_hWnd;
    PA_WindowParams    m_params;
    PAMonitorEntry     m_entry;
    HMONITOR           m_hMonLastAdapted;

    bool  m_bDragging;
    POINT m_ptDragStart;
    POINT m_ptPaperOffset;

    PAButtonRects m_btnRects;
    int   m_iTabIndex;

    bool  m_bHighContrast;
    float m_dpiScale;

    static const UINT_PTR TIMER_HIGH_CONTRAST = 1;
    static const UINT HIGH_CONTRAST_INTERVAL = 2000;
    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"PAOverlayWindowClass";
};

class PAWindowRegistry
{
public:
    static PAWindowRegistry& GetInstance();

    PAOverlayWindow* Get();
    void Set(PAOverlayWindow* pWnd);
    void CloseAndClear();
    void Clear();

private:
    PAOverlayWindow* m_window = nullptr;
    std::mutex m_mutex;
};
