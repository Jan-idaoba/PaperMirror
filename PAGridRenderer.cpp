#include "pch.h"
#include "PAGridRenderer.h"
#include "PALanguage.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "gdiplus.lib")

// Button dimensions (logical pixels, scaled by DPI at draw time)
static const int BTN_WIDTH      = 100;
static const int BTN_HEIGHT     = 30;
static const int BTN_MARGIN     = 10;
static const int BTN_FONT_SIZE  = 14;
static const int BTN_TOP_MARGIN = 10;

// ============================================================================
// Built-in paper / grid size table (mm)
// ============================================================================

namespace
{
    struct PaperSpec
    {
        PA_PaperType type;
        double       widthMm;
        double       heightMm;
    };

    static const PaperSpec kPaperSpecs[] = {
        // Outline mode
        { PA_PAPER_A4,        210.0, 297.0 },
        { PA_PAPER_A3,        297.0, 420.0 },
        { PA_PAPER_A5,        148.0, 210.0 },
        { PA_PAPER_B5,        176.0, 250.0 },
        { PA_PAPER_LETTER,    215.9, 279.4 },
        { PA_PAPER_LEGAL,     215.9, 355.6 },
        // Grid mode (cell size)
        { PA_PAPER_GRID_5MM,  5.0,   5.0   },
        { PA_PAPER_GRID_1CM,  10.0,  10.0  },
        { PA_PAPER_GRID_2CM,  20.0,  20.0  },
        { PA_PAPER_GRID_1IN,  25.4,  25.4  },
    };

    static bool LookupPaperSpec(PA_PaperType type, double* w, double* h)
    {
        for (const auto& s : kPaperSpecs)
        {
            if (s.type == type)
            {
                *w = s.widthMm;
                *h = s.heightMm;
                return true;
            }
        }
        return false;
    }
}

bool PAGridRenderer::IsFullScreenMode(PA_PaperType type)
{
    return (type >= 100 && type < 200) || type == PA_PAPER_CUSTOM_GRID;
}

void PAGridRenderer::GetPaperSizeMm(const PA_WindowParams& params,
                                     double* outWidthMm,
                                     double* outHeightMm)
{
    if (params.paperType == PA_PAPER_CUSTOM_OUTLINE ||
        params.paperType == PA_PAPER_CUSTOM_GRID)
    {
        *outWidthMm  = params.paperWidthMm  > 0 ? params.paperWidthMm  : 210.0;
        *outHeightMm = params.paperHeightMm > 0 ? params.paperHeightMm : 297.0;
        return;
    }

    if (LookupPaperSpec(params.paperType, outWidthMm, outHeightMm))
        return;

    // Unknown type → fall back to A4
    *outWidthMm  = 210.0;
    *outHeightMm = 297.0;
}

// ============================================================================
// Pixel helpers
// ============================================================================

void PAGridRenderer::PA_SetPixel(BYTE* pBits, int stride, int imgHeight,
                                  int x, int y,
                                  BYTE r, BYTE g, BYTE b, BYTE a)
{
    if (x < 0 || y < 0 || x >= stride / 4 || y >= imgHeight)
        return;
    int row = imgHeight - 1 - y;
    BYTE* px = pBits + row * stride + x * 4;
    px[0] = (BYTE)(b * a / 255);
    px[1] = (BYTE)(g * a / 255);
    px[2] = (BYTE)(r * a / 255);
    px[3] = a;
}

void PAGridRenderer::PA_FillRect(BYTE* pBits, int stride, int imgHeight,
                                  int x, int y, int w, int h,
                                  BYTE r, BYTE g, BYTE b, BYTE a)
{
    int imgWidth = stride / 4;
    int x0 = (0 > x) ? 0 : x;
    int y0 = (0 > y) ? 0 : y;
    int x1 = (imgWidth < x + w) ? imgWidth : (x + w);
    int y1 = (imgHeight < y + h) ? imgHeight : (y + h);

    BYTE pb = (BYTE)(b * a / 255);
    BYTE pg = (BYTE)(g * a / 255);
    BYTE pr = (BYTE)(r * a / 255);

    for (int iy = y0; iy < y1; iy++)
    {
        int row = imgHeight - 1 - iy;
        BYTE* scanline = pBits + row * stride;
        for (int ix = x0; ix < x1; ix++)
        {
            BYTE* px = scanline + ix * 4;
            int sa = a;
            int da = px[3];
            int outA = sa + da * (255 - sa) / 255;
            if (outA > 0)
            {
                px[0] = (BYTE)(pb + px[0] * (255 - sa) / 255);
                px[1] = (BYTE)(pg + px[1] * (255 - sa) / 255);
                px[2] = (BYTE)(pr + px[2] * (255 - sa) / 255);
                px[3] = (BYTE)outA;
            }
        }
    }
}

void PAGridRenderer::PA_DrawHLine(BYTE* pBits, int stride, int imgHeight,
                                   int x, int y, int length,
                                   BYTE r, BYTE g, BYTE b, BYTE a)
{
    int imgWidth = stride / 4;
    if (y < 0 || y >= imgHeight) return;
    int x0 = (0 > x) ? 0 : x;
    int x1 = (imgWidth < x + length) ? imgWidth : (x + length);

    BYTE pb = (BYTE)(b * a / 255);
    BYTE pg = (BYTE)(g * a / 255);
    BYTE pr = (BYTE)(r * a / 255);

    int row = imgHeight - 1 - y;
    BYTE* scanline = pBits + row * stride;
    for (int ix = x0; ix < x1; ix++)
    {
        BYTE* px = scanline + ix * 4;
        int sa = a;
        int da = px[3];
        int outA = sa + da * (255 - sa) / 255;
        if (outA > 0)
        {
            px[0] = (BYTE)(pb + px[0] * (255 - sa) / 255);
            px[1] = (BYTE)(pg + px[1] * (255 - sa) / 255);
            px[2] = (BYTE)(pr + px[2] * (255 - sa) / 255);
            px[3] = (BYTE)outA;
        }
    }
}

void PAGridRenderer::PA_DrawVLine(BYTE* pBits, int stride, int imgHeight,
                                   int x, int y, int length,
                                   BYTE r, BYTE g, BYTE b, BYTE a)
{
    int imgWidth = stride / 4;
    if (x < 0 || x >= imgWidth) return;
    int y0 = (0 > y) ? 0 : y;
    int y1 = (imgHeight < y + length) ? imgHeight : (y + length);

    BYTE pb = (BYTE)(b * a / 255);
    BYTE pg = (BYTE)(g * a / 255);
    BYTE pr = (BYTE)(r * a / 255);

    for (int iy = y0; iy < y1; iy++)
    {
        int row = imgHeight - 1 - iy;
        BYTE* px = pBits + row * stride + x * 4;
        int sa = a;
        int da = px[3];
        int outA = sa + da * (255 - sa) / 255;
        if (outA > 0)
        {
            px[0] = (BYTE)(pb + px[0] * (255 - sa) / 255);
            px[1] = (BYTE)(pg + px[1] * (255 - sa) / 255);
            px[2] = (BYTE)(pr + px[2] * (255 - sa) / 255);
            px[3] = (BYTE)outA;
        }
    }
}

void PAGridRenderer::PA_DrawRect(BYTE* pBits, int stride, int imgHeight,
                                  int x, int y, int w, int h,
                                  BYTE r, BYTE g, BYTE b, BYTE a, int thickness)
{
    for (int t = 0; t < thickness; t++)
    {
        PA_DrawHLine(pBits, stride, imgHeight, x, y + t, w, r, g, b, a);
        PA_DrawHLine(pBits, stride, imgHeight, x, y + h - 1 - t, w, r, g, b, a);
        PA_DrawVLine(pBits, stride, imgHeight, x + t, y, h, r, g, b, a);
        PA_DrawVLine(pBits, stride, imgHeight, x + w - 1 - t, y, h, r, g, b, a);
    }
}

void PAGridRenderer::PA_DrawTextOnBitmap(BYTE* pBits, int bmpWidth, int bmpHeight,
                                          const RECT& rc, const wchar_t* text,
                                          COLORREF color, int fontSize, float dpiScale)
{
    int tw = rc.right - rc.left;
    int th = rc.bottom - rc.top;
    if (tw <= 0 || th <= 0) return;

    HDC hScreenDC = GetDC(NULL);
    HDC hTempDC = CreateCompatibleDC(hScreenDC);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = tw;
    bmi.bmiHeader.biHeight = th;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE* pTempBits = nullptr;
    HBITMAP hTempBmp = CreateDIBSection(hTempDC, &bmi, DIB_RGB_COLORS, (void**)&pTempBits, NULL, 0);
    if (!hTempBmp)
    {
        DeleteDC(hTempDC);
        ReleaseDC(NULL, hScreenDC);
        return;
    }

    HBITMAP hOldBmp = (HBITMAP)SelectObject(hTempDC, hTempBmp);

    memset(pTempBits, 0, tw * th * 4);

    // After adding per-monitor DPI awareness manifest, GetDC(NULL) returns a
    // DC whose LOGPIXELSX matches the target monitor's real DPI.
    // fontSize * dpiScale already gives the correct physical pixel height.
    int scaledFontSize = (int)(fontSize * dpiScale);
    if (scaledFontSize <= 0)
        scaledFontSize = 1;

    HFONT hFont = CreateFontW(-scaledFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hTempDC, hFont);

    SetBkMode(hTempDC, TRANSPARENT);
    SetTextColor(hTempDC, RGB(255, 255, 255));

    RECT textRect = { 0, 0, tw, th };
    DrawTextW(hTempDC, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hTempDC, hOldFont);
    DeleteObject(hFont);

    GdiFlush();

    BYTE cr = GetRValue(color);
    BYTE cg = GetGValue(color);
    BYTE cb = GetBValue(color);

    int tempStride = tw * 4;
    int mainStride = bmpWidth * 4;

    for (int ty = 0; ty < th; ty++)
    {
        int srcRow = ty;
        int dstY = rc.top + (th - 1 - ty);
        if (dstY < 0 || dstY >= bmpHeight) continue;

        int dstRow = bmpHeight - 1 - dstY;
        BYTE* srcLine = pTempBits + srcRow * tempStride;
        BYTE* dstLine = pBits + dstRow * mainStride;

        for (int tx = 0; tx < tw; tx++)
        {
            int dstX = rc.left + tx;
            if (dstX < 0 || dstX >= bmpWidth) continue;

            BYTE* sp = srcLine + tx * 4;
            BYTE textAlpha = sp[2];
            if (textAlpha == 0) continue;

            BYTE* dp = dstLine + dstX * 4;
            BYTE pa = textAlpha;
            dp[0] = (BYTE)(cb * pa / 255 + dp[0] * (255 - pa) / 255);
            dp[1] = (BYTE)(cg * pa / 255 + dp[1] * (255 - pa) / 255);
            dp[2] = (BYTE)(cr * pa / 255 + dp[2] * (255 - pa) / 255);
            dp[3] = (BYTE)(pa + dp[3] * (255 - pa) / 255);
        }
    }

    SelectObject(hTempDC, hOldBmp);
    DeleteObject(hTempBmp);
    DeleteDC(hTempDC);
    ReleaseDC(NULL, hScreenDC);
}

void PAGridRenderer::DrawButtons(BYTE* pBits, int width, int height,
                                  int btnX, int btnY,
                                  bool showFullScreenBtn,
                                  int tabIndex,
                                  bool highContrast,
                                  float dpiScale,
                                  PAButtonRects& outBtnRects)
{
    int scaledBtnW = (int)(BTN_WIDTH * dpiScale);
    int scaledBtnH = (int)(BTN_HEIGHT * dpiScale);
    int scaledMargin = (int)(BTN_MARGIN * dpiScale);
    int stride = width * 4;

    const wchar_t* closeText = PALanguage::GetInstance().Translate("Close");
    const wchar_t* fullScreenText = PALanguage::GetInstance().Translate("FullScreen");

    BYTE btnBgR = 0x33, btnBgG = 0x33, btnBgB = 0x33, btnBgA = 200;
    BYTE btnBorderR = 0xFF, btnBorderG = 0xFF, btnBorderB = 0xFF, btnBorderA = 200;
    BYTE focusBorderR = 0x00, focusBorderG = 0x78, focusBorderB = 0xD4, focusBorderA = 255;

    if (highContrast)
    {
        btnBgR = 0; btnBgG = 0; btnBgB = 0; btnBgA = 255;
        btnBorderR = 0xFF; btnBorderG = 0xFF; btnBorderB = 0xFF; btnBorderA = 255;
    }

    int currentX = btnX;

    if (showFullScreenBtn)
    {
        int fsW = (int)(scaledBtnW * 1.2);
        RECT rcFS = { currentX, btnY, currentX + fsW, btnY + scaledBtnH };
        outBtnRects.rcBtnFullScreen = rcFS;

        PA_FillRect(pBits, stride, height, rcFS.left, rcFS.top,
                    fsW, scaledBtnH, btnBgR, btnBgG, btnBgB, btnBgA);

        if (tabIndex == 0)
            PA_DrawRect(pBits, stride, height, rcFS.left, rcFS.top, fsW, scaledBtnH,
                        focusBorderR, focusBorderG, focusBorderB, focusBorderA, 2);
        else
            PA_DrawRect(pBits, stride, height, rcFS.left, rcFS.top, fsW, scaledBtnH,
                        btnBorderR, btnBorderG, btnBorderB, btnBorderA, 1);

        PA_DrawTextOnBitmap(pBits, width, height, rcFS, fullScreenText,
                            RGB(255, 255, 255), BTN_FONT_SIZE, dpiScale);

        currentX += fsW + scaledMargin;
    }
    else
    {
        SetRectEmpty(&outBtnRects.rcBtnFullScreen);
    }

    RECT rcClose = { currentX, btnY, currentX + scaledBtnW, btnY + scaledBtnH };
    outBtnRects.rcBtnClose = rcClose;

    PA_FillRect(pBits, stride, height, rcClose.left, rcClose.top,
                scaledBtnW, scaledBtnH, btnBgR, btnBgG, btnBgB, btnBgA);

    int closeFocusIdx = showFullScreenBtn ? 1 : 0;
    if (tabIndex == closeFocusIdx)
        PA_DrawRect(pBits, stride, height, rcClose.left, rcClose.top, scaledBtnW, scaledBtnH,
                    focusBorderR, focusBorderG, focusBorderB, focusBorderA, 2);
    else
        PA_DrawRect(pBits, stride, height, rcClose.left, rcClose.top, scaledBtnW, scaledBtnH,
                    btnBorderR, btnBorderG, btnBorderB, btnBorderA, 1);

    PA_DrawTextOnBitmap(pBits, width, height, rcClose, closeText,
                        RGB(255, 255, 255), BTN_FONT_SIZE, dpiScale);
}

void PAGridRenderer::DrawPaperOutline(BYTE* pBits, int width, int height,
                                       const PA_WindowParams& params,
                                       float dpiScale,
                                       const POINT& paperOffset,
                                       int tabIndex,
                                       bool highContrast,
                                       PAButtonRects& outBtnRects)
{
    int stride = width * 4;

    double widthMm = 0, heightMm = 0;
    GetPaperSizeMm(params, &widthMm, &heightMm);

    double realDpi = 96.0 * dpiScale;
    int paperW = (int)(widthMm  * realDpi / 25.4);
    int paperH = (int)(heightMm * realDpi / 25.4);

#ifndef NDEBUG
    {
        wchar_t dbg[160];
        swprintf_s(dbg, L"[PA] Paper: %.0fx%.0fmm => %dx%dpx, effectiveDPI=%.0f, scale=%.2f\n",
                   widthMm, heightMm, paperW, paperH, realDpi, dpiScale);
        OutputDebugStringW(dbg);
    }
#endif

    int paperX = (width  - paperW) / 2 + paperOffset.x;
    int paperY = (height - paperH) / 2 + paperOffset.y;

    PA_FillRect(pBits, stride, height, paperX, paperY, paperW, paperH,
                0, 0, 0, 0x4C);

    BYTE borderR = 0xFF, borderG = 0xFF, borderB = 0xFF, borderA = 200;
    if (highContrast)
    {
        borderR = 0xFF; borderG = 0x00; borderB = 0x00; borderA = 255;
    }
    PA_DrawRect(pBits, stride, height, paperX, paperY, paperW, paperH,
                borderR, borderG, borderB, borderA, 2);

    int scaledBtnW = (int)(BTN_WIDTH * dpiScale);
    int scaledMargin = (int)(BTN_MARGIN * dpiScale);
    int btnX = paperX + paperW - scaledBtnW - scaledMargin;
    int btnY = paperY + (int)(BTN_TOP_MARGIN * dpiScale);

    DrawButtons(pBits, width, height, btnX, btnY,
                false, tabIndex, highContrast, dpiScale, outBtnRects);
}

void PAGridRenderer::DrawFullScreenGrid(BYTE* pBits, int width, int height,
                                         const PA_WindowParams& params,
                                         float dpiScale,
                                         int tabIndex,
                                         bool highContrast,
                                         PAButtonRects& outBtnRects)
{
    int stride = width * 4;

    PA_FillRect(pBits, stride, height, 0, 0, width, height, 0, 0, 0, 0x4C);

    BYTE lineR, lineG, lineB, lineA;
    if (highContrast)
    {
        lineR = 0xFF; lineG = 0x00; lineB = 0x00; lineA = 255;
    }
    else
    {
        lineR = 0x59; lineG = 0x59; lineB = 0x59; lineA = 180;
    }

    double widthMm = 0, heightMm = 0;
    GetPaperSizeMm(params, &widthMm, &heightMm);

    double realDpi = 96.0 * dpiScale;
    double gridSpacingX = widthMm  * realDpi / 25.4;
    double gridSpacingY = heightMm * realDpi / 25.4;

    if (gridSpacingX < 10) gridSpacingX = 10;
    if (gridSpacingY < 10) gridSpacingY = 10;

    for (double x = 0; x < width; x += gridSpacingX)
    {
        int ix = (int)(x + 0.5);
        if (ix >= 0 && ix < width)
            PA_DrawVLine(pBits, stride, height, ix, 0, height, lineR, lineG, lineB, lineA);
    }

    for (double y = 0; y < height; y += gridSpacingY)
    {
        int iy = (int)(y + 0.5);
        if (iy >= 0 && iy < height)
            PA_DrawHLine(pBits, stride, height, 0, iy, width, lineR, lineG, lineB, lineA);
    }

    int scaledBtnW = (int)(BTN_WIDTH * dpiScale);
    int scaledMargin = (int)(BTN_MARGIN * dpiScale);
    int fsBtnW = (int)(scaledBtnW * 1.2);
    int totalBtnW = fsBtnW + scaledMargin + scaledBtnW;
    int btnX = width - totalBtnW - scaledMargin;
    int btnY = (int)(BTN_TOP_MARGIN * dpiScale);

    DrawButtons(pBits, width, height, btnX, btnY,
                true, tabIndex, highContrast, dpiScale, outBtnRects);
}

HBITMAP PAGridRenderer::Render(HDC hdc,
                                const PA_WindowParams& params,
                                int windowWidth,
                                int windowHeight,
                                float dpiScale,
                                const POINT& paperOffset,
                                int tabIndex,
                                bool highContrast,
                                PAButtonRects& outBtnRects)
{
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = windowWidth;
    bmi.bmiHeader.biHeight = windowHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
    if (!hBitmap || !pBits)
        return NULL;

    memset(pBits, 0, windowWidth * windowHeight * 4);

    if (IsFullScreenMode(params.paperType))
    {
        DrawFullScreenGrid(pBits, windowWidth, windowHeight, params,
                           dpiScale, tabIndex, highContrast, outBtnRects);
    }
    else
    {
        DrawPaperOutline(pBits, windowWidth, windowHeight, params,
                         dpiScale, paperOffset, tabIndex, highContrast, outBtnRects);
    }

    return hBitmap;
}
