#pragma once

#include <Windows.h>
#include <mutex>

#include "PaperMirrorApi.h"
#include "PMGridRenderer.h"
#include "PMMonitorRegistry.h"

class PMOverlayWindow
{
public:
    PMOverlayWindow();
    ~PMOverlayWindow();

    bool Create(const PM_WindowParams& params, const PMMonitorEntry& entry);
    void Modify(const PM_WindowParams& params);
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
    PM_WindowParams    m_params;
    PMMonitorEntry     m_entry;
    HMONITOR           m_hMonLastAdapted;

    bool  m_bDragging;
    POINT m_ptDragStart;
    POINT m_ptPaperOffset;

    PMButtonRects m_btnRects;
    int   m_iTabIndex;

    bool  m_bHighContrast;
    float m_dpiScale;

    static const UINT_PTR TIMER_HIGH_CONTRAST = 1;
    static const UINT HIGH_CONTRAST_INTERVAL = 2000;
    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"PMOverlayWindowClass";
};

class PMWindowRegistry
{
public:
    static PMWindowRegistry& GetInstance();

    PMOverlayWindow* Get();
    void Set(PMOverlayWindow* pWnd);
    void CloseAndClear();
    void Clear();

private:
    PMOverlayWindow* m_window = nullptr;
    std::mutex m_mutex;
};
