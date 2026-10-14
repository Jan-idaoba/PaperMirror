#pragma once

#include <Windows.h>
#include <atomic>

#include "PaperMirrorApi.h"

enum PMMessage
{
    WM_PM_OPEN     = WM_USER + 100,
    WM_PM_MODIFY   = WM_USER + 101,
    WM_PM_CLOSE    = WM_USER + 102,
    WM_PM_SETLANG  = WM_USER + 104,
    WM_PM_QUIT     = WM_USER + 105,
};

class PMUIThread
{
public:
    static PMUIThread& GetInstance();

    int  Initialize();
    void Uninitialize();
    bool IsInitialized() const { return m_bInitialized.load(); }

    void PostOpen(const PM_WindowParams* params);
    void PostModify(const PM_WindowParams* params);
    void PostClose();
    void PostSetLanguage(PM_Language language);

private:
    PMUIThread();
    ~PMUIThread();

    static DWORD WINAPI UIThreadProc(LPVOID lpParam);
    static LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MonitorWatchProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void RunMessageLoop();
    void HandleOpen(PM_WindowParams* params);
    void HandleModify(PM_WindowParams* params);
    void HandleClose();
    void HandleSetLanguage(PM_Language language);
    void HandleDisplayChanged();

    static PM_WindowParams* CopyParams(const PM_WindowParams* params);

    HANDLE  m_hThread;
    DWORD   m_dwThreadId;
    HWND    m_hMsgWnd;
    HWND    m_hMonitorWatchWnd;
    std::atomic<bool> m_bInitialized;
    HANDLE  m_hReadyEvent;

    static constexpr const wchar_t* MSG_WND_CLASS       = L"PMMsgWindowClass";
    static constexpr const wchar_t* MONITOR_WATCH_CLASS = L"PMMonitorWatchClass";
};
