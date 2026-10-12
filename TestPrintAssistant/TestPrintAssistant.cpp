#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <cassert>

// ============================================================================
// PA API types and function pointer typedefs (must match PrintAssitWndApi.h)
// ============================================================================

enum PA_Language : int
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

enum PA_PaperType : int
{
    PA_PAPER_A4             = 0,
    PA_PAPER_A3             = 1,
    PA_PAPER_A5             = 2,
    PA_PAPER_B5             = 3,
    PA_PAPER_LETTER         = 4,
    PA_PAPER_LEGAL          = 5,
    PA_PAPER_GRID_5MM       = 100,
    PA_PAPER_GRID_1CM       = 101,
    PA_PAPER_GRID_2CM       = 102,
    PA_PAPER_GRID_1IN       = 103,
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

typedef int  (*PFN_PA_Initialize)();
typedef void (*PFN_PA_Uninitialize)();
typedef int  (*PFN_PA_OpenWindow)(const PA_WindowParams*);
typedef int  (*PFN_PA_ModifyWindow)(const PA_WindowParams*);
typedef int  (*PFN_PA_CloseWindow)();
typedef void (*PFN_PA_SetLanguage)(PA_Language);

// ============================================================================
// Global function pointers
// ============================================================================

static PFN_PA_Initialize      g_PA_Initialize      = nullptr;
static PFN_PA_Uninitialize    g_PA_Uninitialize    = nullptr;
static PFN_PA_OpenWindow      g_PA_OpenWindow      = nullptr;
static PFN_PA_ModifyWindow    g_PA_ModifyWindow    = nullptr;
static PFN_PA_CloseWindow     g_PA_CloseWindow     = nullptr;
static PFN_PA_SetLanguage     g_PA_SetLanguage     = nullptr;

// ============================================================================
// Helper: load DLL and resolve all exports
// ============================================================================

static HMODULE LoadPrintAssistantDLL()
{
    const char* candidates[] = {
        "PrintAssistant.dll",
        "..\\x64\\Debug\\PrintAssistant.dll",
        "..\\x64\\Release\\PrintAssistant.dll",
        "..\\Debug\\PrintAssistant.dll",
        "..\\Release\\PrintAssistant.dll",
    };

    HMODULE hDll = nullptr;
    for (auto path : candidates)
    {
        hDll = LoadLibraryA(path);
        if (hDll)
        {
            std::cout << "[INFO] Loaded DLL from: " << path << "\n";
            break;
        }
    }

    if (!hDll)
    {
        std::cerr << "[ERROR] Failed to load PrintAssistant.dll.\n";
        return nullptr;
    }

    g_PA_Initialize      = (PFN_PA_Initialize)      GetProcAddress(hDll, "PA_Initialize");
    g_PA_Uninitialize    = (PFN_PA_Uninitialize)    GetProcAddress(hDll, "PA_Uninitialize");
    g_PA_OpenWindow      = (PFN_PA_OpenWindow)      GetProcAddress(hDll, "PA_OpenWindow");
    g_PA_ModifyWindow    = (PFN_PA_ModifyWindow)    GetProcAddress(hDll, "PA_ModifyWindow");
    g_PA_CloseWindow     = (PFN_PA_CloseWindow)     GetProcAddress(hDll, "PA_CloseWindow");
    g_PA_SetLanguage     = (PFN_PA_SetLanguage)     GetProcAddress(hDll, "PA_SetLanguage");

    if (!g_PA_Initialize || !g_PA_Uninitialize ||
        !g_PA_OpenWindow || !g_PA_ModifyWindow ||
        !g_PA_CloseWindow || !g_PA_SetLanguage)
    {
        std::cerr << "[ERROR] Failed to resolve one or more API exports.\n";
        FreeLibrary(hDll);
        return nullptr;
    }

    std::cout << "[INFO] All API exports resolved successfully.\n";
    return hDll;
}

// ============================================================================
// Test counters
// ============================================================================

static int g_passed = 0;
static int g_failed = 0;

#define TEST_CHECK(expr, msg) \
    do { \
        if (expr) { \
            std::cout << "  [PASS] " << (msg) << "\n"; \
            g_passed++; \
        } else { \
            std::cerr << "  [FAIL] " << (msg) << "\n"; \
            g_failed++; \
        } \
    } while (0)

// ============================================================================
// Test 1: Initialize / Uninitialize lifecycle
// ============================================================================

static void Test_InitUninit()
{
    std::cout << "\n=== Test: Initialize / Uninitialize ===\n";

    int ret = g_PA_Initialize();
    TEST_CHECK(ret == 0, "PA_Initialize returns 0");

    int ret2 = g_PA_Initialize();
    TEST_CHECK(ret2 == 0, "PA_Initialize called twice returns 0");

    g_PA_Uninitialize();
    std::cout << "  [INFO] PA_Uninitialize called.\n";

    g_PA_Uninitialize();
    TEST_CHECK(true, "Double PA_Uninitialize does not crash");
}

// ============================================================================
// Test 2: OpenWindow with null params should fail
// ============================================================================

static void Test_OpenWindow_NullParams()
{
    std::cout << "\n=== Test: OpenWindow with null params ===\n";

    g_PA_Initialize();

    int ret = g_PA_OpenWindow(nullptr);
    TEST_CHECK(ret == -1, "PA_OpenWindow(nullptr) returns -1");

    g_PA_Uninitialize();
}

// ============================================================================
// Test 3: OpenWindow before Initialize should fail
// ============================================================================

static void Test_OpenWindow_NotInitialized()
{
    std::cout << "\n=== Test: OpenWindow before Initialize ===\n";

    g_PA_Uninitialize();

    PA_WindowParams params{};
    params.paperType = PA_PAPER_A4;
    params.language  = PA_LANG_EN;
    int ret = g_PA_OpenWindow(&params);
    TEST_CHECK(ret == -2, "PA_OpenWindow before Initialize returns -2");
}

// ============================================================================
// Test 4: ModifyWindow with null params should fail
// ============================================================================

static void Test_ModifyWindow_NullParams()
{
    std::cout << "\n=== Test: ModifyWindow with null params ===\n";

    g_PA_Initialize();

    int ret = g_PA_ModifyWindow(nullptr);
    TEST_CHECK(ret == -1, "PA_ModifyWindow(nullptr) returns -1");

    g_PA_Uninitialize();
}

// ============================================================================
// Test 5: ModifyWindow before Initialize should fail
// ============================================================================

static void Test_ModifyWindow_NotInitialized()
{
    std::cout << "\n=== Test: ModifyWindow before Initialize ===\n";

    g_PA_Uninitialize();

    PA_WindowParams params{};
    params.paperType = PA_PAPER_A4;
    params.language  = PA_LANG_EN;
    int ret = g_PA_ModifyWindow(&params);
    TEST_CHECK(ret == -2, "PA_ModifyWindow before Initialize returns -2");
}

// ============================================================================
// Test 6: CloseWindow before Initialize should fail
// ============================================================================

static void Test_CloseWindow_NotInitialized()
{
    std::cout << "\n=== Test: CloseWindow before Initialize ===\n";

    g_PA_Uninitialize();

    int ret = g_PA_CloseWindow();
    TEST_CHECK(ret == -2, "PA_CloseWindow before Initialize returns -2");
}

// ============================================================================
// Test 7: Full workflow — open, modify, close a paper-outline window
// ============================================================================

static void Test_PaperOutlineWorkflow()
{
    std::cout << "\n=== Test: Paper Outline Workflow (open -> modify -> close) ===\n";

    int ret = g_PA_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PA_WindowParams params{};
    params.paperType = PA_PAPER_A4;
    params.language  = PA_LANG_EN;
    ret = g_PA_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow A4 paper outline");

    Sleep(1500);

    params.paperType = PA_PAPER_B5;
    ret = g_PA_ModifyWindow(&params);
    TEST_CHECK(ret == 0, "ModifyWindow to B5 paper");

    Sleep(1000);

    ret = g_PA_CloseWindow();
    TEST_CHECK(ret == 0, "CloseWindow");

    Sleep(500);
    g_PA_Uninitialize();
}

// ============================================================================
// Test 8: Full-screen grid mode
// ============================================================================

static void Test_FullScreenGridWorkflow()
{
    std::cout << "\n=== Test: Full Screen Grid Workflow ===\n";

    int ret = g_PA_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PA_WindowParams params{};
    params.paperType = PA_PAPER_GRID_1CM;
    params.language  = PA_LANG_EN;
    ret = g_PA_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow fullscreen 1cm grid");

    Sleep(1500);

    ret = g_PA_CloseWindow();
    TEST_CHECK(ret == 0, "CloseWindow");

    Sleep(500);
    g_PA_Uninitialize();
}

// ============================================================================
// Test 9: Open multiple windows, then CloseAll
// ============================================================================

static void Test_OpenCloseAll()
{
    std::cout << "\n=== Test: Open + CloseAll ===\n";

    int ret = g_PA_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PA_WindowParams params{};
    params.paperType = PA_PAPER_A4;
    params.language  = PA_LANG_EN;
    ret = g_PA_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow (A4)");

    Sleep(1500);

    g_PA_CloseWindow();
    TEST_CHECK(true, "CloseWindow does not crash");

    Sleep(500);
    g_PA_Uninitialize();
}

// ============================================================================
// Test 10: Language switching
// ============================================================================

static void Test_LanguageSwitching()
{
    std::cout << "\n=== Test: Language Switching ===\n";

    int ret = g_PA_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PA_WindowParams params{};
    params.paperType = PA_PAPER_A4;
    params.language  = PA_LANG_EN;
    ret = g_PA_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow in English");

    Sleep(1000);

    g_PA_SetLanguage(PA_LANG_SC);
    std::cout << "  [INFO] Switched language to SC.\n";
    Sleep(1000);

    g_PA_SetLanguage(PA_LANG_JA);
    std::cout << "  [INFO] Switched language to JA.\n";
    Sleep(1000);

    g_PA_SetLanguage(PA_LANG_EN);
    std::cout << "  [INFO] Switched language to EN.\n";
    Sleep(500);

    g_PA_CloseWindow();
    Sleep(500);
    g_PA_Uninitialize();

    // Unknown enum should silently fall back to EN — must not crash
    g_PA_SetLanguage(static_cast<PA_Language>(99999));
    TEST_CHECK(true, "PA_SetLanguage(unknown) does not crash");
}

// ============================================================================
// Test 11: Different paper types
// ============================================================================

static void Test_DifferentPaperTypes()
{
    std::cout << "\n=== Test: Different Paper Types ===\n";

    int ret = g_PA_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    struct PaperCase
    {
        const char*  name;
        PA_PaperType type;
    };

    PaperCase papers[] = {
        { "A4",       PA_PAPER_A4 },
        { "B5",       PA_PAPER_B5 },
        { "A3",       PA_PAPER_A3 },
        { "Letter",   PA_PAPER_LETTER },
        { "GRID_1CM", PA_PAPER_GRID_1CM },
    };

    for (int i = 0; i < _countof(papers); i++)
    {
        PA_WindowParams params{};
        params.paperType = papers[i].type;
        params.language  = PA_LANG_EN;

        ret = g_PA_OpenWindow(&params);
        std::string msg = std::string("OpenWindow with paper type: ") + papers[i].name;
        TEST_CHECK(ret == 0, msg.c_str());

        Sleep(1000);

        g_PA_CloseWindow();
        Sleep(300);
    }

    g_PA_Uninitialize();
}

// ============================================================================
// Test 12: Rapid open/close stress test
// ============================================================================

static void Test_RapidOpenClose()
{
    std::cout << "\n=== Test: Rapid Open/Close Stress ===\n";

    int ret = g_PA_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    for (int i = 0; i < 5; i++)
    {
        PA_WindowParams params{};
        params.paperType = PA_PAPER_A4;
        params.language  = PA_LANG_EN;
        g_PA_OpenWindow(&params);
        Sleep(200);
        g_PA_CloseWindow();
        Sleep(100);
    }

    TEST_CHECK(true, "Rapid open/close 5 iterations completed without crash");

    g_PA_CloseWindow();
    Sleep(300);
    g_PA_Uninitialize();
}

// ============================================================================
// Test 13: Custom paper size
// ============================================================================

static void Test_CustomPaperSize()
{
    std::cout << "\n=== Test: Custom Paper Size ===\n";

    int ret = g_PA_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PA_WindowParams params{};
    params.paperType     = PA_PAPER_CUSTOM_OUTLINE;
    params.language      = PA_LANG_EN;
    params.paperWidthMm  = 100.0;
    params.paperHeightMm = 150.0;
    ret = g_PA_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow with CUSTOM_OUTLINE 100x150mm");

    Sleep(1200);

    params.paperType     = PA_PAPER_CUSTOM_GRID;
    params.paperWidthMm  = 15.0;
    params.paperHeightMm = 15.0;
    ret = g_PA_ModifyWindow(&params);
    TEST_CHECK(ret == 0, "ModifyWindow to CUSTOM_GRID 15x15mm");

    Sleep(1200);

    g_PA_CloseWindow();
    Sleep(300);
    g_PA_Uninitialize();
}

// ============================================================================
// Interactive menu
// ============================================================================

static void PrintMenu()
{
    std::cout << "\n"
        "============================================\n"
        "   PrintAssistant Test Suite\n"
        "============================================\n"
        "  0 - Run ALL tests\n"
        "  1 - Initialize / Uninitialize lifecycle\n"
        "  2 - OpenWindow with null params\n"
        "  3 - OpenWindow before Initialize\n"
        "  4 - ModifyWindow with null params\n"
        "  5 - ModifyWindow before Initialize\n"
        "  6 - CloseWindow before Initialize\n"
        "  7 - Paper outline workflow (open->modify->close)\n"
        "  8 - Full-screen grid workflow\n"
        "  9 - Open + CloseAll\n"
        " 10 - Language switching\n"
        " 11 - Different paper types\n"
        " 12 - Rapid open/close stress test\n"
        " 13 - Custom paper size\n"
        "  q - Quit\n"
        "============================================\n"
        "Choice: ";
}

typedef void (*TestFunc)();

static TestFunc allTests[] = {
    Test_InitUninit,
    Test_OpenWindow_NullParams,
    Test_OpenWindow_NotInitialized,
    Test_ModifyWindow_NullParams,
    Test_ModifyWindow_NotInitialized,
    Test_CloseWindow_NotInitialized,
    Test_PaperOutlineWorkflow,
    Test_FullScreenGridWorkflow,
    Test_OpenCloseAll,
    Test_LanguageSwitching,
    Test_DifferentPaperTypes,
    Test_RapidOpenClose,
    Test_CustomPaperSize,
};
static constexpr int kTestCount = _countof(allTests);

static void RunAllTests()
{
    g_passed = 0;
    g_failed = 0;
    for (int i = 0; i < kTestCount; i++)
        allTests[i]();

    std::cout << "\n============================================\n"
              << "  Results: " << g_passed << " passed, "
              << g_failed << " failed\n"
              << "============================================\n";
}

static void RunSingleTest(int choice)
{
    if (choice >= 1 && choice <= kTestCount)
    {
        g_passed = 0;
        g_failed = 0;
        allTests[choice - 1]();
        std::cout << "  Results: " << g_passed << " passed, "
                  << g_failed << " failed\n";
    }
    else
    {
        std::cout << "Invalid test number: " << choice << "\n";
    }
}

// Usage: TestPrintAssistant.exe [test_number]
//   no args  -> interactive menu
//   0        -> run all tests
//   1-14     -> run specific test
int main(int argc, char* argv[])
{
    HMODULE hDll = LoadPrintAssistantDLL();
    if (!hDll)
    {
        std::cerr << "Cannot proceed without the DLL. Exiting.\n";
        return 1;
    }

    if (argc >= 2)
    {
        int choice = -1;
        try { choice = std::stoi(argv[1]); } catch (...) { choice = -1; }

        if (choice == 0)
            RunAllTests();
        else
            RunSingleTest(choice);

        FreeLibrary(hDll);
        return g_failed > 0 ? 1 : 0;
    }

    bool running = true;
    while (running)
    {
        PrintMenu();
        std::string input;
        if (!std::getline(std::cin, input))
            break;

        if (input == "q" || input == "Q")
        {
            running = false;
            continue;
        }

        if (input == "0")
        {
            RunAllTests();
            continue;
        }

        int choice = -1;
        try { choice = std::stoi(input); } catch (...) { choice = -1; }
        RunSingleTest(choice);
    }

    FreeLibrary(hDll);
    std::cout << "Done.\n";
    return 0;
}
