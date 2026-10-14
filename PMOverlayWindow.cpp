#include "pch.h"
#include "PMOverlayWindow.h"
#include "PMLanguage.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shcore.lib")

static bool IsHighContrastEnabled()
{
    HIGHCONTRAST hc = { sizeof(HIGHCONTRAST) };
    SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0);
    return (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
}

static PM_PaperType ToggleToOutline(PM_PaperType pt)
{
    if (pt == PM_PAPER_CUSTOM_GRID)
        return PM_PAPER_CUSTOM_OUTLINE;
    return PM_PAPER_A4;
}

// ============================================================================
// PMOverlayWindow
// ============================================================================

PMOverlayWindow::PMOverlayWindow()
    : m_hWnd(NULL)
    , m_params{}
    , m_entry{}
    , m_bDragging(false)
    , m_ptDragStart{}
    , m_ptPaperOffset{}
    , m_btnRects{}
    , m_iTabIndex(-1)
    , m_bHighContrast(false)
    , m_dpiScale(1.0f)
{
}

PMOverlayWindow::~PMOverlayWindow()
{
}

bool PMOverlayWindow::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    return RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool PMOverlayWindow::Create(const PM_WindowParams& params, const PMMonitorEntry& entry)
{
    m_params = params;
    m_entry  = entry;

    m_dpiScale = entry.dpiX / 96.0f;
    m_bHighContrast = IsHighContrastEnabled();
    m_ptPaperOffset = { 0, 0 };
    m_iTabIndex = -1;
    m_bDragging = false;
    m_hMonLastAdapted = entry.hMonitor;

    int x = entry.rect.left;
    int y = entry.rect.top;
    int w = entry.rect.right - entry.rect.left;
    int h = entry.rect.bottom - entry.rect.top;

    HINSTANCE hInst = GetModuleHandle(NULL);

    m_hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        WINDOW_CLASS_NAME,
        L"PaperMirror",
        WS_POPUP,
        x, y, w, h,
        NULL, NULL, hInst, this);

    if (!m_hWnd)
        return false;

    LONG_PTR exStyle = GetWindowLongPtrW(m_hWnd, GWL_EXSTYLE);
    exStyle &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(m_hWnd, GWL_EXSTYLE, exStyle);

    Render();
    ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
    SetTimer(m_hWnd, TIMER_HIGH_CONTRAST, HIGH_CONTRAST_INTERVAL, NULL);

    return true;
}

void PMOverlayWindow::Modify(const PM_WindowParams& params)
{
    m_params = params;
    m_ptPaperOffset = { 0, 0 };
    Render();
}

void PMOverlayWindow::AdaptToMonitor(HMONITOR hMon)
{
    if (!hMon || !m_hWnd) return;
    if (hMon == m_entry.hMonitor) return;

    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hMon, &mi);

    UINT dpiX = 96, dpiY = 96;
    GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

    POINT savedOffset = m_ptPaperOffset;

    m_entry.hMonitor = hMon;
    m_entry.rect     = mi.rcMonitor;
    m_entry.dpiX     = dpiX;
    m_entry.dpiY     = dpiY;
    m_dpiScale       = dpiX / 96.0f;
    m_ptPaperOffset  = savedOffset;
    m_hMonLastAdapted = hMon;

    int x = mi.rcMonitor.left;
    int y = mi.rcMonitor.top;
    int w = mi.rcMonitor.right  - mi.rcMonitor.left;
    int h = mi.rcMonitor.bottom - mi.rcMonitor.top;
    SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
    Render();
}

void PMOverlayWindow::Close()
{
    if (m_hWnd && IsWindow(m_hWnd))
    {
        KillTimer(m_hWnd, TIMER_HIGH_CONTRAST);
        SetWindowLongPtrW(m_hWnd, GWLP_USERDATA, 0);
        HWND hWnd = m_hWnd;
        m_hWnd = NULL;
        DestroyWindow(hWnd);
    }
}

void PMOverlayWindow::Render()
{
    if (!m_hWnd) return;

    RECT rc;
    GetWindowRect(m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = PMGridRenderer::Render(
        hMemDC, m_params, width, height,
        m_dpiScale, m_ptPaperOffset, m_iTabIndex,
        m_bHighContrast, m_btnRects);

    if (hBitmap)
    {
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBitmap);

        POINT ptSrc = { 0, 0 };
        SIZE sz = { width, height };
        BLENDFUNCTION blend = {};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        POINT ptWnd = { rc.left, rc.top };
        UpdateLayeredWindow(m_hWnd, hScreenDC, &ptWnd, &sz, hMemDC, &ptSrc, 0, &blend, ULW_ALPHA);

        SelectObject(hMemDC, hOldBmp);
        DeleteObject(hBitmap);
    }

    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);
}

bool PMOverlayWindow::IsPointInPaper(int x, int y) const
{
    if (PMGridRenderer::IsFullScreenMode(m_params.paperType))
        return true;

    RECT rc;
    GetWindowRect(m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    double widthMm = 0, heightMm = 0;
    PMGridRenderer::GetPaperSizeMm(m_params, &widthMm, &heightMm);

    int paperW = (int)(widthMm * m_entry.dpiX / 25.4);
    int paperH = (int)(heightMm * m_entry.dpiY / 25.4);
    int paperX = (width - paperW) / 2 + m_ptPaperOffset.x;
    int paperY = (height - paperH) / 2 + m_ptPaperOffset.y;

    return x >= paperX && x < paperX + paperW &&
           y >= paperY && y < paperY + paperH;
}

bool PMOverlayWindow::IsPointInButton(int x, int y, const RECT& btnRect) const
{
    return x >= btnRect.left && x < btnRect.right &&
           y >= btnRect.top && y < btnRect.bottom;
}

void PMOverlayWindow::OnMouseDown(int x, int y)
{
    if (IsPointInButton(x, y, m_btnRects.rcBtnClose))
    {
        PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
        return;
    }

    bool fullScreen = PMGridRenderer::IsFullScreenMode(m_params.paperType);

    if (fullScreen && IsPointInButton(x, y, m_btnRects.rcBtnFullScreen))
    {
        m_params.paperType = ToggleToOutline(m_params.paperType);
        m_ptPaperOffset = { 0, 0 };
        Render();
        return;
    }

    if (!fullScreen && IsPointInPaper(x, y))
    {
        m_bDragging = true;
        m_ptDragStart = { x, y };
        m_hMonLastAdapted = m_entry.hMonitor;
        SetCapture(m_hWnd);
    }
}

void PMOverlayWindow::OnMouseMove(int x, int y)
{
    if (m_bDragging)
    {
        m_ptPaperOffset.x += x - m_ptDragStart.x;
        m_ptPaperOffset.y += y - m_ptDragStart.y;
        m_ptDragStart = { x, y };

        // Keep paper center within window so paper never leaves the visible area entirely.
        int halfW = (m_entry.rect.right  - m_entry.rect.left) / 2;
        int halfH = (m_entry.rect.bottom - m_entry.rect.top)  / 2;
        if (m_ptPaperOffset.x >  halfW) m_ptPaperOffset.x =  halfW;
        if (m_ptPaperOffset.x < -halfW) m_ptPaperOffset.x = -halfW;
        if (m_ptPaperOffset.y >  halfH) m_ptPaperOffset.y =  halfH;
        if (m_ptPaperOffset.y < -halfH) m_ptPaperOffset.y = -halfH;

        // Detect cursor crossing into another monitor and move the window there.
        POINT ptScreen;
        if (GetCursorPos(&ptScreen))
        {
            HMONITOR hMonCursor = MonitorFromPoint(ptScreen, MONITOR_DEFAULTTONULL);
            if (hMonCursor && hMonCursor != m_hMonLastAdapted)
            {
                AdaptToMonitor(hMonCursor);
                halfW = (m_entry.rect.right  - m_entry.rect.left) / 2;
                halfH = (m_entry.rect.bottom - m_entry.rect.top)  / 2;
                if (m_ptPaperOffset.x >  halfW) m_ptPaperOffset.x =  halfW;
                if (m_ptPaperOffset.x < -halfW) m_ptPaperOffset.x = -halfW;
                if (m_ptPaperOffset.y >  halfH) m_ptPaperOffset.y =  halfH;
                if (m_ptPaperOffset.y < -halfH) m_ptPaperOffset.y = -halfH;
            }
        }

        Render();
    }
}

void PMOverlayWindow::OnMouseUp()
{
    if (m_bDragging)
    {
        m_bDragging = false;
        ReleaseCapture();
    }
}

void PMOverlayWindow::OnKeyDown(WPARAM vk)
{
    bool fullScreen = PMGridRenderer::IsFullScreenMode(m_params.paperType);
    int btnCount = fullScreen ? 2 : 1;

    if (vk == VK_TAB)
    {
        m_iTabIndex = (m_iTabIndex + 1) % btnCount;
        Render();
    }
    else if (vk == VK_RETURN || vk == VK_SPACE)
    {
        if (fullScreen)
        {
            if (m_iTabIndex == 0)
            {
                m_params.paperType = ToggleToOutline(m_params.paperType);
                m_ptPaperOffset = { 0, 0 };
                m_iTabIndex = -1;
                Render();
            }
            else if (m_iTabIndex == 1)
            {
                PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
            }
        }
        else if (m_iTabIndex == 0)
        {
            PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
        }
    }
    else if (vk == VK_ESCAPE)
    {
        PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
    }
}

void PMOverlayWindow::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_HIGH_CONTRAST)
    {
        bool bHC = IsHighContrastEnabled();
        if (bHC != m_bHighContrast)
        {
            m_bHighContrast = bHC;
            Render();
        }
    }
}

LRESULT CALLBACK PMOverlayWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PMOverlayWindow* pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<PMOverlayWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<PMOverlayWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        switch (msg)
        {
        case WM_LBUTTONDOWN:
            pThis->OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_MOUSEMOVE:
            pThis->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONUP:
            pThis->OnMouseUp();
            return 0;

        case WM_KEYDOWN:
            pThis->OnKeyDown(wParam);
            return 0;

        case WM_TIMER:
            pThis->OnTimer(wParam);
            return 0;

        case WM_WINDOWPOSCHANGED:
        {
            // Detect when the window has been moved to a different monitor and
            // automatically resize + re-render with the new monitor's DPI.
            HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            if (hMon && hMon != pThis->m_entry.hMonitor)
                pThis->AdaptToMonitor(hMon);
            return 0;
        }

        case WM_CLOSE:
        {
            KillTimer(hWnd, TIMER_HIGH_CONTRAST);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
            pThis->m_hWnd = NULL;
            DestroyWindow(hWnd);
            PMWindowRegistry::GetInstance().Clear();
            return 0;
        }

        case WM_DESTROY:
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================================
// PMWindowRegistry
// ============================================================================

PMWindowRegistry& PMWindowRegistry::GetInstance()
{
    static PMWindowRegistry instance;
    return instance;
}

PMOverlayWindow* PMWindowRegistry::Get()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_window;
}

void PMWindowRegistry::Set(PMOverlayWindow* pWnd)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_window = pWnd;
}

void PMWindowRegistry::CloseAndClear()
{
    PMOverlayWindow* pWnd = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        pWnd = m_window;
        m_window = nullptr;
    }
    if (pWnd)
    {
        pWnd->Close();
        delete pWnd;
    }
}

void PMWindowRegistry::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    delete m_window;
    m_window = nullptr;
}
