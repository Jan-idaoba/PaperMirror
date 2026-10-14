#include "pch.h"
#include "PMUIThread.h"
#include "PMOverlayWindow.h"
#include "PMLanguage.h"
#include "PMMonitorRegistry.h"

PMUIThread& PMUIThread::GetInstance()
{
    static PMUIThread instance;
    return instance;
}

PMUIThread::PMUIThread()
    : m_hThread(NULL)
    , m_dwThreadId(0)
    , m_hMsgWnd(NULL)
    , m_hMonitorWatchWnd(NULL)
    , m_bInitialized(false)
    , m_hReadyEvent(NULL)
{
}

PMUIThread::~PMUIThread()
{
    Uninitialize();
}

PM_WindowParams* PMUIThread::CopyParams(const PM_WindowParams* params)
{
    if (!params) return nullptr;
    return new PM_WindowParams(*params);
}

static void FreeParams(PM_WindowParams* params)
{
    delete params;
}

int PMUIThread::Initialize()
{
    if (m_bInitialized.load())
        return 0;

    m_hReadyEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!m_hReadyEvent)
        return -1;

    m_hThread = CreateThread(NULL, 0, UIThreadProc, this, 0, &m_dwThreadId);
    if (!m_hThread)
    {
        CloseHandle(m_hReadyEvent);
        m_hReadyEvent = NULL;
        return -1;
    }

    WaitForSingleObject(m_hReadyEvent, 5000);
    CloseHandle(m_hReadyEvent);
    m_hReadyEvent = NULL;

    m_bInitialized.store(true);
    return 0;
}

void PMUIThread::Uninitialize()
{
    if (!m_bInitialized.load())
        return;

    if (m_hMsgWnd && IsWindow(m_hMsgWnd))
        PostMessageW(m_hMsgWnd, WM_PM_QUIT, 0, 0);

    if (m_hThread)
    {
        WaitForSingleObject(m_hThread, 10000);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    m_dwThreadId = 0;
    m_hMsgWnd = NULL;
    m_hMonitorWatchWnd = NULL;
    m_bInitialized.store(false);
}

void PMUIThread::PostOpen(const PM_WindowParams* params)
{
    if (!m_hMsgWnd) return;
    PM_WindowParams* copy = CopyParams(params);
    PostMessageW(m_hMsgWnd, WM_PM_OPEN, 0, reinterpret_cast<LPARAM>(copy));
}

void PMUIThread::PostModify(const PM_WindowParams* params)
{
    if (!m_hMsgWnd) return;
    PM_WindowParams* copy = CopyParams(params);
    PostMessageW(m_hMsgWnd, WM_PM_MODIFY, 0, reinterpret_cast<LPARAM>(copy));
}

void PMUIThread::PostClose()
{
    if (!m_hMsgWnd) return;
    PostMessageW(m_hMsgWnd, WM_PM_CLOSE, 0, 0);
}

void PMUIThread::PostSetLanguage(PM_Language language)
{
    if (!m_hMsgWnd) return;
    PostMessageW(m_hMsgWnd, WM_PM_SETLANG, static_cast<WPARAM>(language), 0);
}

DWORD WINAPI PMUIThread::UIThreadProc(LPVOID lpParam)
{
    PMUIThread* pThis = reinterpret_cast<PMUIThread*>(lpParam);
    pThis->RunMessageLoop();
    return 0;
}

void PMUIThread::RunMessageLoop()
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HINSTANCE hInst = GetModuleHandle(NULL);

    WNDCLASSEXW wcMsg = {};
    wcMsg.cbSize = sizeof(WNDCLASSEXW);
    wcMsg.lpfnWndProc = MsgWndProc;
    wcMsg.hInstance = hInst;
    wcMsg.lpszClassName = MSG_WND_CLASS;
    RegisterClassExW(&wcMsg);

    WNDCLASSEXW wcWatch = {};
    wcWatch.cbSize = sizeof(WNDCLASSEXW);
    wcWatch.lpfnWndProc = MonitorWatchProc;
    wcWatch.hInstance = hInst;
    wcWatch.lpszClassName = MONITOR_WATCH_CLASS;
    RegisterClassExW(&wcWatch);

    PMOverlayWindow::RegisterWindowClass(hInst);

    m_hMsgWnd = CreateWindowExW(0, MSG_WND_CLASS, L"PMMsgWindow", 0,
                                 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, this);

    m_hMonitorWatchWnd = CreateWindowExW(WS_EX_TOOLWINDOW,
                                          MONITOR_WATCH_CLASS, L"PMMonitorWatch", WS_POPUP,
                                          0, 0, 0, 0, NULL, NULL, hInst, this);

    PMMonitorRegistry::GetInstance().Refresh();

    if (m_hReadyEvent)
        SetEvent(m_hReadyEvent);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

LRESULT CALLBACK PMUIThread::MsgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PMUIThread* pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<PMUIThread*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<PMUIThread*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        switch (msg)
        {
        case WM_PM_OPEN:
        {
            PM_WindowParams* params = reinterpret_cast<PM_WindowParams*>(lParam);
            pThis->HandleOpen(params);
            FreeParams(params);
            return 0;
        }
        case WM_PM_MODIFY:
        {
            PM_WindowParams* params = reinterpret_cast<PM_WindowParams*>(lParam);
            pThis->HandleModify(params);
            FreeParams(params);
            return 0;
        }
        case WM_PM_CLOSE:
            pThis->HandleClose();
            return 0;
        case WM_PM_SETLANG:
            pThis->HandleSetLanguage(static_cast<PM_Language>(wParam));
            return 0;
        case WM_PM_QUIT:
        {
            pThis->HandleClose();
            if (pThis->m_hMonitorWatchWnd)
            {
                DestroyWindow(pThis->m_hMonitorWatchWnd);
                pThis->m_hMonitorWatchWnd = NULL;
            }
            DestroyWindow(hWnd);
            PostQuitMessage(0);
            return 0;
        }
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK PMUIThread::MonitorWatchProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PMUIThread* pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<PMUIThread*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<PMUIThread*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis && msg == WM_DISPLAYCHANGE)
    {
        pThis->HandleDisplayChanged();
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void PMUIThread::HandleOpen(PM_WindowParams* params)
{
    if (!params) return;

    PMWindowRegistry::GetInstance().CloseAndClear();

    PMMonitorEntry entry;
    if (!PMMonitorRegistry::GetInstance().GetPrimaryEntry(&entry))
        return;

    PMOverlayWindow* pWnd = new PMOverlayWindow();
    if (pWnd->Create(*params, entry))
        PMWindowRegistry::GetInstance().Set(pWnd);
    else
        delete pWnd;
}

void PMUIThread::HandleModify(PM_WindowParams* params)
{
    if (!params) return;

    PMOverlayWindow* pWnd = PMWindowRegistry::GetInstance().Get();
    if (pWnd)
        pWnd->Modify(*params);
}

void PMUIThread::HandleClose()
{
    PMWindowRegistry::GetInstance().CloseAndClear();
}

void PMUIThread::HandleSetLanguage(PM_Language language)
{
    PMLanguage::GetInstance().SetLanguage(language);

    PMOverlayWindow* pWnd = PMWindowRegistry::GetInstance().Get();
    if (pWnd && pWnd->GetHWND())
        pWnd->Render();
}

void PMUIThread::HandleDisplayChanged()
{
    PMMonitorRegistry::GetInstance().Refresh();

    PMOverlayWindow* pWnd = PMWindowRegistry::GetInstance().Get();
    if (!pWnd || !pWnd->GetHWND())
        return;

    HMONITOR hMon = MonitorFromWindow(pWnd->GetHWND(), MONITOR_DEFAULTTONEAREST);
    if (hMon)
        pWnd->AdaptToMonitor(hMon);
}
