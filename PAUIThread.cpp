#include "pch.h"
#include "PAUIThread.h"
#include "PAOverlayWindow.h"
#include "PALanguage.h"
#include "PAMonitorRegistry.h"

PAUIThread& PAUIThread::GetInstance()
{
    static PAUIThread instance;
    return instance;
}

PAUIThread::PAUIThread()
    : m_hThread(NULL)
    , m_dwThreadId(0)
    , m_hMsgWnd(NULL)
    , m_hMonitorWatchWnd(NULL)
    , m_bInitialized(false)
    , m_hReadyEvent(NULL)
{
}

PAUIThread::~PAUIThread()
{
    Uninitialize();
}

PA_WindowParams* PAUIThread::CopyParams(const PA_WindowParams* params)
{
    if (!params) return nullptr;
    return new PA_WindowParams(*params);
}

static void FreeParams(PA_WindowParams* params)
{
    delete params;
}

int PAUIThread::Initialize()
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

void PAUIThread::Uninitialize()
{
    if (!m_bInitialized.load())
        return;

    if (m_hMsgWnd && IsWindow(m_hMsgWnd))
        PostMessageW(m_hMsgWnd, WM_PA_QUIT, 0, 0);

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

void PAUIThread::PostOpen(const PA_WindowParams* params)
{
    if (!m_hMsgWnd) return;
    PA_WindowParams* copy = CopyParams(params);
    PostMessageW(m_hMsgWnd, WM_PA_OPEN, 0, reinterpret_cast<LPARAM>(copy));
}

void PAUIThread::PostModify(const PA_WindowParams* params)
{
    if (!m_hMsgWnd) return;
    PA_WindowParams* copy = CopyParams(params);
    PostMessageW(m_hMsgWnd, WM_PA_MODIFY, 0, reinterpret_cast<LPARAM>(copy));
}

void PAUIThread::PostClose()
{
    if (!m_hMsgWnd) return;
    PostMessageW(m_hMsgWnd, WM_PA_CLOSE, 0, 0);
}

void PAUIThread::PostSetLanguage(PA_Language language)
{
    if (!m_hMsgWnd) return;
    PostMessageW(m_hMsgWnd, WM_PA_SETLANG, static_cast<WPARAM>(language), 0);
}

DWORD WINAPI PAUIThread::UIThreadProc(LPVOID lpParam)
{
    PAUIThread* pThis = reinterpret_cast<PAUIThread*>(lpParam);
    pThis->RunMessageLoop();
    return 0;
}

void PAUIThread::RunMessageLoop()
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

    PAOverlayWindow::RegisterWindowClass(hInst);

    m_hMsgWnd = CreateWindowExW(0, MSG_WND_CLASS, L"PAMsgWindow", 0,
                                 0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, this);

    m_hMonitorWatchWnd = CreateWindowExW(WS_EX_TOOLWINDOW,
                                          MONITOR_WATCH_CLASS, L"PAMonitorWatch", WS_POPUP,
                                          0, 0, 0, 0, NULL, NULL, hInst, this);

    PAMonitorRegistry::GetInstance().Refresh();

    if (m_hReadyEvent)
        SetEvent(m_hReadyEvent);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

LRESULT CALLBACK PAUIThread::MsgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAUIThread* pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<PAUIThread*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<PAUIThread*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        switch (msg)
        {
        case WM_PA_OPEN:
        {
            PA_WindowParams* params = reinterpret_cast<PA_WindowParams*>(lParam);
            pThis->HandleOpen(params);
            FreeParams(params);
            return 0;
        }
        case WM_PA_MODIFY:
        {
            PA_WindowParams* params = reinterpret_cast<PA_WindowParams*>(lParam);
            pThis->HandleModify(params);
            FreeParams(params);
            return 0;
        }
        case WM_PA_CLOSE:
            pThis->HandleClose();
            return 0;
        case WM_PA_SETLANG:
            pThis->HandleSetLanguage(static_cast<PA_Language>(wParam));
            return 0;
        case WM_PA_QUIT:
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

LRESULT CALLBACK PAUIThread::MonitorWatchProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAUIThread* pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<PAUIThread*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<PAUIThread*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis && msg == WM_DISPLAYCHANGE)
    {
        pThis->HandleDisplayChanged();
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void PAUIThread::HandleOpen(PA_WindowParams* params)
{
    if (!params) return;

    PAWindowRegistry::GetInstance().CloseAndClear();

    PAMonitorEntry entry;
    if (!PAMonitorRegistry::GetInstance().GetPrimaryEntry(&entry))
        return;

    PAOverlayWindow* pWnd = new PAOverlayWindow();
    if (pWnd->Create(*params, entry))
        PAWindowRegistry::GetInstance().Set(pWnd);
    else
        delete pWnd;
}

void PAUIThread::HandleModify(PA_WindowParams* params)
{
    if (!params) return;

    PAOverlayWindow* pWnd = PAWindowRegistry::GetInstance().Get();
    if (pWnd)
        pWnd->Modify(*params);
}

void PAUIThread::HandleClose()
{
    PAWindowRegistry::GetInstance().CloseAndClear();
}

void PAUIThread::HandleSetLanguage(PA_Language language)
{
    PALanguage::GetInstance().SetLanguage(language);

    PAOverlayWindow* pWnd = PAWindowRegistry::GetInstance().Get();
    if (pWnd && pWnd->GetHWND())
        pWnd->Render();
}

void PAUIThread::HandleDisplayChanged()
{
    PAMonitorRegistry::GetInstance().Refresh();

    PAOverlayWindow* pWnd = PAWindowRegistry::GetInstance().Get();
    if (!pWnd || !pWnd->GetHWND())
        return;

    HMONITOR hMon = MonitorFromWindow(pWnd->GetHWND(), MONITOR_DEFAULTTONEAREST);
    if (hMon)
        pWnd->AdaptToMonitor(hMon);
}
