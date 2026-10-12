// TestPriAssistantUI.cpp : PrintAssistant overlay window UI test harness.
// Dynamically loads PrintAssistant.dll and exercises PA_OpenWindow
// for every built-in paper / grid type plus a custom-size path.

#include "framework.h"
#include "TestPriAssistantUI.h"

#define MAX_LOADSTRING 100

// ── Control IDs ──────────────────────────────────────────────────────────────
#define IDC_BTN_A4          1001
#define IDC_BTN_A3          1002
#define IDC_BTN_A5          1003
#define IDC_BTN_B5          1004
#define IDC_BTN_LETTER      1005
#define IDC_BTN_LEGAL       1006
#define IDC_BTN_GRID_5MM    1007
#define IDC_BTN_GRID_1CM    1008
#define IDC_BTN_GRID_2CM    1009
#define IDC_BTN_GRID_1IN    1010
#define IDC_BTN_CLOSE       1011
#define IDC_LANG_COMBO      1012
#define IDC_BTN_CUSTOM      1013
#define IDC_EDIT_WIDTH      1014
#define IDC_EDIT_HEIGHT     1015
#define IDC_STATUS_LABEL    1016

// ── PA_PaperType constants (mirrors PrintAssitWndApi.h) ──────────────────────
#define PA_PAPER_A4             0
#define PA_PAPER_A3             1
#define PA_PAPER_A5             2
#define PA_PAPER_B5             3
#define PA_PAPER_LETTER         4
#define PA_PAPER_LEGAL          5
#define PA_PAPER_GRID_5MM       100
#define PA_PAPER_GRID_1CM       101
#define PA_PAPER_GRID_2CM       102
#define PA_PAPER_GRID_1IN       103
#define PA_PAPER_CUSTOM_OUTLINE 200

struct PA_WindowParams {
    int    paperType;
    int    language;
    double paperWidthMm;
    double paperHeightMm;
};

// ── Function-pointer typedefs ────────────────────────────────────────────────
typedef int  (*PFN_PA_Initialize)();
typedef void (*PFN_PA_Uninitialize)();
typedef int  (*PFN_PA_OpenWindow)(const PA_WindowParams*);
typedef int  (*PFN_PA_CloseWindow)();
typedef void (*PFN_PA_SetLanguage)(int);

// ── Globals ──────────────────────────────────────────────────────────────────
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

HMODULE             g_hDll            = nullptr;
PFN_PA_Initialize   g_PA_Initialize   = nullptr;
PFN_PA_Uninitialize g_PA_Uninitialize = nullptr;
PFN_PA_OpenWindow   g_PA_OpenWindow   = nullptr;
PFN_PA_CloseWindow  g_PA_CloseWindow  = nullptr;
PFN_PA_SetLanguage  g_PA_SetLanguage  = nullptr;

HWND g_hStatus   = nullptr;
int  g_currentLang = 0; // PA_LANG_EN

struct LangEntry { const wchar_t* name; int value; };
static const LangEntry g_langs[] = {
    { L"English (EN)",      0  }, { L"简体中文 (SC)",   1  },
    { L"繁體中文 (TC)",     2  }, { L"日本語 (JA)",     3  },
    { L"한국어 (KO)",       4  }, { L"Deutsch (GE)",    5  },
    { L"Francais (FR)",     6  }, { L"Espanol (SP)",    7  },
    { L"Portugues BR (PB)", 8  }, { L"Italiano (IT)",   9  },
    { L"Nederlands (DU)",   10 }, { L"Dansk (DA)",      11 },
    { L"Svenska (SW)",      12 }, { L"Norsk (NO)",      13 },
    { L"Suomi (FI)",        14 }, { L"Arabic (AR)",     15 },
    { L"Hebrew (HE)",       16 }, { L"Russian (RU)",    17 },
};

// Forward declarations
ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

// ── Helpers ──────────────────────────────────────────────────────────────────

static void SetStatus(const wchar_t* msg)
{
    if (g_hStatus) SetWindowTextW(g_hStatus, msg);
}

static bool LoadDll()
{
    static const char* paths[] = {
        "PrintAssistant.dll",
        "..\\x64\\Debug\\PrintAssistant.dll",
        "..\\x64\\Release\\PrintAssistant.dll",
        "..\\Debug\\PrintAssistant.dll",
        "..\\Release\\PrintAssistant.dll",
    };
    for (auto p : paths) {
        g_hDll = LoadLibraryA(p);
        if (g_hDll) break;
    }
    if (!g_hDll) return false;

    g_PA_Initialize   = (PFN_PA_Initialize)  GetProcAddress(g_hDll, "PA_Initialize");
    g_PA_Uninitialize = (PFN_PA_Uninitialize) GetProcAddress(g_hDll, "PA_Uninitialize");
    g_PA_OpenWindow   = (PFN_PA_OpenWindow)   GetProcAddress(g_hDll, "PA_OpenWindow");
    g_PA_CloseWindow  = (PFN_PA_CloseWindow)  GetProcAddress(g_hDll, "PA_CloseWindow");
    g_PA_SetLanguage  = (PFN_PA_SetLanguage)  GetProcAddress(g_hDll, "PA_SetLanguage");

    return g_PA_Initialize && g_PA_OpenWindow && g_PA_CloseWindow;
}

static void OpenPaper(int paperType, double widthMm = 0.0, double heightMm = 0.0)
{
    if (!g_PA_OpenWindow) { SetStatus(L"DLL 未加载"); return; }
    PA_WindowParams p = { paperType, g_currentLang, widthMm, heightMm };
    int ret = g_PA_OpenWindow(&p);
    if (ret == 0) {
        SetStatus(L"叠加窗口已打开");
    } else {
        wchar_t buf[64];
        swprintf_s(buf, L"打开失败 (错误码 %d)", ret);
        SetStatus(buf);
    }
}

static HWND MakeLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h)
{
    return CreateWindowExW(0, L"STATIC", text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, h, parent, nullptr, hInst, nullptr);
}

static HWND MakeBtn(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h)
{
    return CreateWindowExW(0, L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, hInst, nullptr);
}

static void CreateControls(HWND hWnd)
{
    const int MARGIN = 14;
    const int BW = 118, BH = 28, GAP = 6;
    int x = MARGIN, y = MARGIN;

    // ── 纸张轮廓 ──────────────────────────────────────────────────
    MakeLabel(hWnd, L"[纸张轮廓 Paper Outline]", x, y, 340, 18);
    y += 22;

    MakeBtn(hWnd, L"A4  210×297 mm",   IDC_BTN_A4,    x,              y, BW, BH);
    MakeBtn(hWnd, L"A3  297×420 mm",   IDC_BTN_A3,    x + BW+GAP,     y, BW, BH);
    MakeBtn(hWnd, L"A5  148×210 mm",   IDC_BTN_A5,    x+(BW+GAP)*2,   y, BW, BH);
    y += BH + GAP;

    MakeBtn(hWnd, L"B5  176×250 mm",   IDC_BTN_B5,    x,              y, BW, BH);
    MakeBtn(hWnd, L"Letter 215.9×279", IDC_BTN_LETTER, x + BW+GAP,    y, BW, BH);
    MakeBtn(hWnd, L"Legal  215.9×356", IDC_BTN_LEGAL,  x+(BW+GAP)*2,  y, BW, BH);
    y += BH + GAP * 3;

    // ── 全屏网格 ──────────────────────────────────────────────────
    MakeLabel(hWnd, L"[全屏网格 Full-screen Grid]", x, y, 340, 18);
    y += 22;

    MakeBtn(hWnd, L"网格  5 mm",  IDC_BTN_GRID_5MM, x,            y, BW, BH);
    MakeBtn(hWnd, L"网格  1 cm",  IDC_BTN_GRID_1CM, x + BW+GAP,   y, BW, BH);
    MakeBtn(hWnd, L"网格  2 cm",  IDC_BTN_GRID_2CM, x+(BW+GAP)*2, y, BW, BH);
    MakeBtn(hWnd, L"网格  1 in",  IDC_BTN_GRID_1IN, x+(BW+GAP)*3, y, BW, BH);
    y += BH + GAP * 3;

    // ── 自定义轮廓 ────────────────────────────────────────────────
    MakeLabel(hWnd, L"[自定义轮廓 Custom Outline]", x, y, 340, 18);
    y += 22;

    MakeLabel(hWnd, L"宽 (mm):", x,       y + 3, 55, 18);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"200",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x + 58, y, 52, 22, hWnd, (HMENU)IDC_EDIT_WIDTH, hInst, nullptr);

    MakeLabel(hWnd, L"高 (mm):", x + 116, y + 3, 55, 18);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"280",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x + 174, y, 52, 22, hWnd, (HMENU)IDC_EDIT_HEIGHT, hInst, nullptr);

    MakeBtn(hWnd, L"打开自定义", IDC_BTN_CUSTOM, x + 234, y - 3, 100, BH);
    y += BH + GAP * 3;

    // ── 语言 ──────────────────────────────────────────────────────
    MakeLabel(hWnd, L"语言:", x, y + 4, 38, 18);
    HWND hCombo = CreateWindowExW(0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        x + 42, y, 200, 300, hWnd, (HMENU)IDC_LANG_COMBO, hInst, nullptr);
    for (auto& e : g_langs)
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)e.name);
    SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
    y += 32;

    // ── 关闭 ──────────────────────────────────────────────────────
    MakeBtn(hWnd, L"关闭叠加窗口 (Close)", IDC_BTN_CLOSE, x, y, 160, BH);
    y += BH + GAP * 2;

    // ── 状态栏 ────────────────────────────────────────────────────
    MakeLabel(hWnd, L"状态:", x, y + 3, 36, 18);
    g_hStatus = CreateWindowExW(0, L"STATIC", L"初始化中…",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x + 40, y + 3, 430, 18, hWnd, (HMENU)IDC_STATUS_LABEL, hInst, nullptr);
}

// ── Win32 boilerplate ────────────────────────────────────────────────────────

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex    = {};
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTPRIASSISTANTUI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TESTPRIASSISTANTUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, L"PrintAssistant UI 测试",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 560, 450, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateControls(hWnd);
        if (!LoadDll()) {
            SetStatus(L"PrintAssistant.dll 加载失败！请确认 DLL 路径。");
        } else {
            int r = g_PA_Initialize();
            SetStatus(r == 0 ? L"就绪 — DLL 已初始化" : L"PA_Initialize 返回错误");
        }
        return 0;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        if (HIWORD(wParam) == CBN_SELCHANGE && id == IDC_LANG_COMBO) {
            int sel = (int)SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < ARRAYSIZE(g_langs)) {
                g_currentLang = g_langs[sel].value;
                if (g_PA_SetLanguage) g_PA_SetLanguage(g_currentLang);
            }
            return 0;
        }

        switch (id)
        {
        case IDC_BTN_A4:       OpenPaper(PA_PAPER_A4);       break;
        case IDC_BTN_A3:       OpenPaper(PA_PAPER_A3);       break;
        case IDC_BTN_A5:       OpenPaper(PA_PAPER_A5);       break;
        case IDC_BTN_B5:       OpenPaper(PA_PAPER_B5);       break;
        case IDC_BTN_LETTER:   OpenPaper(PA_PAPER_LETTER);   break;
        case IDC_BTN_LEGAL:    OpenPaper(PA_PAPER_LEGAL);    break;
        case IDC_BTN_GRID_5MM: OpenPaper(PA_PAPER_GRID_5MM); break;
        case IDC_BTN_GRID_1CM: OpenPaper(PA_PAPER_GRID_1CM); break;
        case IDC_BTN_GRID_2CM: OpenPaper(PA_PAPER_GRID_2CM); break;
        case IDC_BTN_GRID_1IN: OpenPaper(PA_PAPER_GRID_1IN); break;
        case IDC_BTN_CUSTOM:
        {
            wchar_t wb[32] = {}, hb[32] = {};
            GetDlgItemTextW(hWnd, IDC_EDIT_WIDTH,  wb, 32);
            GetDlgItemTextW(hWnd, IDC_EDIT_HEIGHT, hb, 32);
            double w = _wtof(wb), h = _wtof(hb);
            if (w <= 0 || h <= 0) { SetStatus(L"请输入有效的宽度和高度 (mm)"); break; }
            OpenPaper(PA_PAPER_CUSTOM_OUTLINE, w, h);
            break;
        }
        case IDC_BTN_CLOSE:
            if (g_PA_CloseWindow) { g_PA_CloseWindow(); SetStatus(L"叠加窗口已关闭"); }
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    case WM_DESTROY:
        if (g_PA_Uninitialize) g_PA_Uninitialize();
        if (g_hDll) { FreeLibrary(g_hDll); g_hDll = nullptr; }
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TESTPRIASSISTANTUI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTPRIASSISTANTUI));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
