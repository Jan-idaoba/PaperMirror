#pragma once

#ifdef PRINTASSISTANT_EXPORTS
#define PA_API extern "C" __declspec(dllexport)
#else
#define PA_API extern "C" __declspec(dllimport)
#endif

#include <Windows.h>

enum PA_Language
{
    PA_LANG_EN  = 0,   // English
    PA_LANG_SC  = 1,   // Simplified Chinese
    PA_LANG_TC  = 2,   // Traditional Chinese
    PA_LANG_JA  = 3,   // Japanese
    PA_LANG_KO  = 4,   // Korean
    PA_LANG_GE  = 5,   // German
    PA_LANG_FR  = 6,   // French
    PA_LANG_SP  = 7,   // Spanish
    PA_LANG_PB  = 8,   // Portuguese (Brazil)
    PA_LANG_IT  = 9,   // Italian
    PA_LANG_DU  = 10,  // Dutch
    PA_LANG_DA  = 11,  // Danish
    PA_LANG_SW  = 12,  // Swedish
    PA_LANG_NO  = 13,  // Norwegian
    PA_LANG_FI  = 14,  // Finnish
    PA_LANG_AR  = 15,  // Arabic
    PA_LANG_HE  = 16,  // Hebrew
    PA_LANG_RU  = 17,  // Russian
};

enum PA_PaperType
{
    // Paper outline (built-in standard sizes)
    PA_PAPER_A4             = 0,    // 210 x 297 mm
    PA_PAPER_A3             = 1,    // 297 x 420 mm
    PA_PAPER_A5             = 2,    // 148 x 210 mm
    PA_PAPER_B5             = 3,    // 176 x 250 mm
    PA_PAPER_LETTER         = 4,    // 215.9 x 279.4 mm
    PA_PAPER_LEGAL          = 5,    // 215.9 x 355.6 mm

    // Full-screen grid (built-in spacings)
    PA_PAPER_GRID_5MM       = 100,
    PA_PAPER_GRID_1CM       = 101,
    PA_PAPER_GRID_2CM       = 102,
    PA_PAPER_GRID_1IN       = 103,  // 25.4 mm

    // Custom (caller supplies size in PA_WindowParams.paperWidthMm/HeightMm)
    PA_PAPER_CUSTOM_OUTLINE = 200,
    PA_PAPER_CUSTOM_GRID    = 201,
};

struct PA_WindowParams
{
    PA_PaperType paperType;        
    PA_Language  language;         
    double       paperWidthMm;     
    double       paperHeightMm;
};

PA_API int  PA_Initialize();
PA_API void PA_Uninitialize();
PA_API int  PA_OpenWindow(const PA_WindowParams* params);
PA_API int  PA_ModifyWindow(const PA_WindowParams* params);
PA_API int  PA_CloseWindow();
PA_API void PA_SetLanguage(PA_Language language);
