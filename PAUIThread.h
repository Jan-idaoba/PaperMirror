#pragma once

#include <Windows.h>
#include <atomic>

#include "PrintAssitWndApi.h"

enum PAMessage
{
    WM_PA_OPEN     = WM_USER + 100,
    WM_PA_MODIFY   = WM_USER + 101,
    WM_PA_CLOSE    = WM_USER + 102,
    WM_PA_SETLANG  = WM_USER + 104,
    WM_PA_QUIT     = WM_USER + 105,
};

class PAUIThread
{
public:
    static PAUIThread& GetInstance();

    int  Initialize();
    void Uninitialize();
    bool IsInitialized() const { return m_bInitialized.load(); }

    void PostOpen(const PA_WindowParams* params);
    void PostModify(const PA_WindowParams* params);
    void PostClose();
    void PostSetLanguage(PA_Language language);

private:
    PAUIThread();
    ~PAUIThread();

    static DWORD WINAPI UIThreadProc(LPVOID lpParam);
    static LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MonitorWatchProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void RunMessageLoop();
    void HandleOpen(PA_WindowParams* params);
    void HandleModify(PA_WindowParams* params);
    void HandleClose();
    void HandleSetLanguage(PA_Language language);
    void HandleDisplayChanged();

    static PA_WindowParams* CopyParams(const PA_WindowParams* params);

    HANDLE  m_hThread;
    DWORD   m_dwThreadId;
    HWND    m_hMsgWnd;
    HWND    m_hMonitorWatchWnd;
    std::atomic<bool> m_bInitialized;
    HANDLE  m_hReadyEvent;

    static constexpr const wchar_t* MSG_WND_CLASS       = L"PAMsgWindowClass";
    static constexpr const wchar_t* MONITOR_WATCH_CLASS = L"PAMonitorWatchClass";
};
