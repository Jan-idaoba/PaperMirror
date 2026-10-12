#pragma once

#include <Windows.h>
#include "PrintAssitWndApi.h"

struct PAButtonRects
{
    RECT rcBtnFullScreen;
    RECT rcBtnClose;
};

class PAGridRenderer
{
public:
    static HBITMAP Render(HDC hdc,
                          const PA_WindowParams& params,
                          int windowWidth,
                          int windowHeight,
                          float dpiScale,
                          const POINT& paperOffset,
                          int tabIndex,
                          bool highContrast,
                          PAButtonRects& outBtnRects);

    // True if paperType selects the full-screen grid renderer.
    static bool IsFullScreenMode(PA_PaperType type);

    // Resolves paper size in millimeters. For built-in types this comes from
    // an internal table; for PA_PAPER_CUSTOM_* it reads params.paperWidthMm /
    // paperHeightMm. Unknown types fall back to A4 (210x297).
    static void GetPaperSizeMm(const PA_WindowParams& params,
                               double* outWidthMm,
                               double* outHeightMm);

private:
    static void DrawPaperOutline(BYTE* pBits, int width, int height,
                                  const PA_WindowParams& params,
                                  float dpiScale,
                                  const POINT& paperOffset,
                                  int tabIndex,
                                  bool highContrast,
                                  PAButtonRects& outBtnRects);

    static void DrawFullScreenGrid(BYTE* pBits, int width, int height,
                                    const PA_WindowParams& params,
                                    float dpiScale,
                                    int tabIndex,
                                    bool highContrast,
                                    PAButtonRects& outBtnRects);

    static void DrawButtons(BYTE* pBits, int width, int height,
                             int btnX, int btnY,
                             bool showFullScreenBtn,
                             int tabIndex,
                             bool highContrast,
                             float dpiScale,
                             PAButtonRects& outBtnRects);

    static void PA_FillRect(BYTE* pBits, int stride, int imgHeight,
                             int x, int y, int w, int h,
                             BYTE r, BYTE g, BYTE b, BYTE a);

    static void PA_DrawRect(BYTE* pBits, int stride, int imgHeight,
                             int x, int y, int w, int h,
                             BYTE r, BYTE g, BYTE b, BYTE a, int thickness);

    static void PA_DrawHLine(BYTE* pBits, int stride, int imgHeight,
                              int x, int y, int length,
                              BYTE r, BYTE g, BYTE b, BYTE a);

    static void PA_DrawVLine(BYTE* pBits, int stride, int imgHeight,
                              int x, int y, int length,
                              BYTE r, BYTE g, BYTE b, BYTE a);

    static void PA_SetPixel(BYTE* pBits, int stride, int imgHeight,
                             int x, int y,
                             BYTE r, BYTE g, BYTE b, BYTE a);

    static void PA_DrawTextOnBitmap(BYTE* pBits, int bmpWidth, int bmpHeight,
                                     const RECT& rc, const wchar_t* text,
                                     COLORREF color, int fontSize, float dpiScale);
};
