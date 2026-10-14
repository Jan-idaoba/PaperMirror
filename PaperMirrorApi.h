#pragma once

#ifdef PAPERMIRROR_EXPORTS
#define PM_API extern "C" __declspec(dllexport)
#else
#define PM_API extern "C" __declspec(dllimport)
#endif

#include <Windows.h>

enum PM_Language
{
    PM_LANG_EN  = 0,   // English
    PM_LANG_SC  = 1,   // Simplified Chinese
    PM_LANG_TC  = 2,   // Traditional Chinese
    PM_LANG_JA  = 3,   // Japanese
    PM_LANG_KO  = 4,   // Korean
    PM_LANG_GE  = 5,   // German
    PM_LANG_FR  = 6,   // French
    PM_LANG_SP  = 7,   // Spanish
    PM_LANG_PB  = 8,   // Portuguese (Brazil)
    PM_LANG_IT  = 9,   // Italian
    PM_LANG_DU  = 10,  // Dutch
    PM_LANG_DA  = 11,  // Danish
    PM_LANG_SW  = 12,  // Swedish
    PM_LANG_NO  = 13,  // Norwegian
    PM_LANG_FI  = 14,  // Finnish
    PM_LANG_AR  = 15,  // Arabic
    PM_LANG_HE  = 16,  // Hebrew
    PM_LANG_RU  = 17,  // Russian
};

enum PM_PaperType
{
    // Paper outline (built-in standard sizes)
    PM_PAPER_A4             = 0,    // 210 x 297 mm
    PM_PAPER_A3             = 1,    // 297 x 420 mm
    PM_PAPER_A5             = 2,    // 148 x 210 mm
    PM_PAPER_B5             = 3,    // 176 x 250 mm
    PM_PAPER_LETTER         = 4,    // 215.9 x 279.4 mm
    PM_PAPER_LEGAL          = 5,    // 215.9 x 355.6 mm

    // Full-screen grid (built-in spacings)
    PM_PAPER_GRID_5MM       = 100,
    PM_PAPER_GRID_1CM       = 101,
    PM_PAPER_GRID_2CM       = 102,
    PM_PAPER_GRID_1IN       = 103,  // 25.4 mm

    // Custom (caller supplies size in PM_WindowParams.paperWidthMm/HeightMm)
    PM_PAPER_CUSTOM_OUTLINE = 200,
    PM_PAPER_CUSTOM_GRID    = 201,
};

struct PM_WindowParams
{
    PM_PaperType paperType;        
    PM_Language  language;         
    double       paperWidthMm;     
    double       paperHeightMm;
};

PM_API int  PM_Initialize();
PM_API void PM_Uninitialize();
PM_API int  PM_OpenWindow(const PM_WindowParams* params);
PM_API int  PM_ModifyWindow(const PM_WindowParams* params);
PM_API int  PM_CloseWindow();
PM_API void PM_SetLanguage(PM_Language language);
