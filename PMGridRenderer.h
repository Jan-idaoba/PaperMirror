#pragma once

#include <Windows.h>
#include "PaperMirrorApi.h"

struct PMButtonRects
{
    RECT rcBtnFullScreen;
    RECT rcBtnClose;
};

class PMGridRenderer
{
public:
    static HBITMAP Render(HDC hdc,
                          const PM_WindowParams& params,
                          int windowWidth,
                          int windowHeight,
                          float dpiScale,
                          const POINT& paperOffset,
                          int tabIndex,
                          bool highContrast,
                          PMButtonRects& outBtnRects);

    // True if paperType selects the full-screen grid renderer.
    static bool IsFullScreenMode(PM_PaperType type);

    // Resolves paper size in millimeters. For built-in types this comes from
    // an internal table; for PM_PAPER_CUSTOM_* it reads params.paperWidthMm /
    // paperHeightMm. Unknown types fall back to A4 (210x297).
    static void GetPaperSizeMm(const PM_WindowParams& params,
                               double* outWidthMm,
                               double* outHeightMm);

private:
    static void DrawPaperOutline(BYTE* pBits, int width, int height,
                                  const PM_WindowParams& params,
                                  float dpiScale,
                                  const POINT& paperOffset,
                                  int tabIndex,
                                  bool highContrast,
                                  PMButtonRects& outBtnRects);

    static void DrawFullScreenGrid(BYTE* pBits, int width, int height,
                                    const PM_WindowParams& params,
                                    float dpiScale,
                                    int tabIndex,
                                    bool highContrast,
                                    PMButtonRects& outBtnRects);

    static void DrawButtons(BYTE* pBits, int width, int height,
                             int btnX, int btnY,
                             bool showFullScreenBtn,
                             int tabIndex,
                             bool highContrast,
                             float dpiScale,
                             PMButtonRects& outBtnRects);

    static void PM_FillRect(BYTE* pBits, int stride, int imgHeight,
                             int x, int y, int w, int h,
                             BYTE r, BYTE g, BYTE b, BYTE a);

    static void PM_DrawRect(BYTE* pBits, int stride, int imgHeight,
                             int x, int y, int w, int h,
                             BYTE r, BYTE g, BYTE b, BYTE a, int thickness);

    static void PM_DrawHLine(BYTE* pBits, int stride, int imgHeight,
                              int x, int y, int length,
                              BYTE r, BYTE g, BYTE b, BYTE a);

    static void PM_DrawVLine(BYTE* pBits, int stride, int imgHeight,
                              int x, int y, int length,
                              BYTE r, BYTE g, BYTE b, BYTE a);

    static void PM_SetPixel(BYTE* pBits, int stride, int imgHeight,
                             int x, int y,
                             BYTE r, BYTE g, BYTE b, BYTE a);

    static void PM_DrawTextOnBitmap(BYTE* pBits, int bmpWidth, int bmpHeight,
                                     const RECT& rc, const wchar_t* text,
                                     COLORREF color, int fontSize, float dpiScale);
};
