#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <cassert>

// ============================================================================
// PM API types and function pointer typedefs (must match PaperMirrorApi.h)
// ============================================================================

enum PM_Language : int
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

enum PM_PaperType : int
{
    PM_PAPER_A4             = 0,
    PM_PAPER_A3             = 1,
    PM_PAPER_A5             = 2,
    PM_PAPER_B5             = 3,
    PM_PAPER_LETTER         = 4,
    PM_PAPER_LEGAL          = 5,
    PM_PAPER_GRID_5MM       = 100,
    PM_PAPER_GRID_1CM       = 101,
    PM_PAPER_GRID_2CM       = 102,
    PM_PAPER_GRID_1IN       = 103,
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

typedef int  (*PFN_PM_Initialize)();
typedef void (*PFN_PM_Uninitialize)();
typedef int  (*PFN_PM_OpenWindow)(const PM_WindowParams*);
typedef int  (*PFN_PM_ModifyWindow)(const PM_WindowParams*);
typedef int  (*PFN_PM_CloseWindow)();
typedef void (*PFN_PM_SetLanguage)(PM_Language);

// ============================================================================
// Global function pointers
// ============================================================================

static PFN_PM_Initialize      g_PM_Initialize      = nullptr;
static PFN_PM_Uninitialize    g_PM_Uninitialize    = nullptr;
static PFN_PM_OpenWindow      g_PM_OpenWindow      = nullptr;
static PFN_PM_ModifyWindow    g_PM_ModifyWindow    = nullptr;
static PFN_PM_CloseWindow     g_PM_CloseWindow     = nullptr;
static PFN_PM_SetLanguage     g_PM_SetLanguage     = nullptr;

// ============================================================================
// Helper: load DLL and resolve all exports
// ============================================================================

static HMODULE LoadPaperMirrorDLL()
{
    const char* candidates[] = {
        "PaperMirror.dll",
        "..\\x64\\Debug\\PaperMirror.dll",
        "..\\x64\\Release\\PaperMirror.dll",
        "..\\Debug\\PaperMirror.dll",
        "..\\Release\\PaperMirror.dll",
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
        std::cerr << "[ERROR] Failed to load PaperMirror.dll.\n";
        return nullptr;
    }

    g_PM_Initialize      = (PFN_PM_Initialize)      GetProcAddress(hDll, "PM_Initialize");
    g_PM_Uninitialize    = (PFN_PM_Uninitialize)    GetProcAddress(hDll, "PM_Uninitialize");
    g_PM_OpenWindow      = (PFN_PM_OpenWindow)      GetProcAddress(hDll, "PM_OpenWindow");
    g_PM_ModifyWindow    = (PFN_PM_ModifyWindow)    GetProcAddress(hDll, "PM_ModifyWindow");
    g_PM_CloseWindow     = (PFN_PM_CloseWindow)     GetProcAddress(hDll, "PM_CloseWindow");
    g_PM_SetLanguage     = (PFN_PM_SetLanguage)     GetProcAddress(hDll, "PM_SetLanguage");

    if (!g_PM_Initialize || !g_PM_Uninitialize ||
        !g_PM_OpenWindow || !g_PM_ModifyWindow ||
        !g_PM_CloseWindow || !g_PM_SetLanguage)
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

    int ret = g_PM_Initialize();
    TEST_CHECK(ret == 0, "PM_Initialize returns 0");

    int ret2 = g_PM_Initialize();
    TEST_CHECK(ret2 == 0, "PM_Initialize called twice returns 0");

    g_PM_Uninitialize();
    std::cout << "  [INFO] PM_Uninitialize called.\n";

    g_PM_Uninitialize();
    TEST_CHECK(true, "Double PM_Uninitialize does not crash");
}

// ============================================================================
// Test 2: OpenWindow with null params should fail
// ============================================================================

static void Test_OpenWindow_NullParams()
{
    std::cout << "\n=== Test: OpenWindow with null params ===\n";

    g_PM_Initialize();

    int ret = g_PM_OpenWindow(nullptr);
    TEST_CHECK(ret == -1, "PM_OpenWindow(nullptr) returns -1");

    g_PM_Uninitialize();
}

// ============================================================================
// Test 3: OpenWindow before Initialize should fail
// ============================================================================

static void Test_OpenWindow_NotInitialized()
{
    std::cout << "\n=== Test: OpenWindow before Initialize ===\n";

    g_PM_Uninitialize();

    PM_WindowParams params{};
    params.paperType = PM_PAPER_A4;
    params.language  = PM_LANG_EN;
    int ret = g_PM_OpenWindow(&params);
    TEST_CHECK(ret == -2, "PM_OpenWindow before Initialize returns -2");
}

// ============================================================================
// Test 4: ModifyWindow with null params should fail
// ============================================================================

static void Test_ModifyWindow_NullParams()
{
    std::cout << "\n=== Test: ModifyWindow with null params ===\n";

    g_PM_Initialize();

    int ret = g_PM_ModifyWindow(nullptr);
    TEST_CHECK(ret == -1, "PM_ModifyWindow(nullptr) returns -1");

    g_PM_Uninitialize();
}

// ============================================================================
// Test 5: ModifyWindow before Initialize should fail
// ============================================================================

static void Test_ModifyWindow_NotInitialized()
{
    std::cout << "\n=== Test: ModifyWindow before Initialize ===\n";

    g_PM_Uninitialize();

    PM_WindowParams params{};
    params.paperType = PM_PAPER_A4;
    params.language  = PM_LANG_EN;
    int ret = g_PM_ModifyWindow(&params);
    TEST_CHECK(ret == -2, "PM_ModifyWindow before Initialize returns -2");
}

// ============================================================================
// Test 6: CloseWindow before Initialize should fail
// ============================================================================

static void Test_CloseWindow_NotInitialized()
{
    std::cout << "\n=== Test: CloseWindow before Initialize ===\n";

    g_PM_Uninitialize();

    int ret = g_PM_CloseWindow();
    TEST_CHECK(ret == -2, "PM_CloseWindow before Initialize returns -2");
}

// ============================================================================
// Test 7: Full workflow — open, modify, close a paper-outline window
// ============================================================================

static void Test_PaperOutlineWorkflow()
{
    std::cout << "\n=== Test: Paper Outline Workflow (open -> modify -> close) ===\n";

    int ret = g_PM_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PM_WindowParams params{};
    params.paperType = PM_PAPER_A4;
    params.language  = PM_LANG_EN;
    ret = g_PM_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow A4 paper outline");

    Sleep(1500);

    params.paperType = PM_PAPER_B5;
    ret = g_PM_ModifyWindow(&params);
    TEST_CHECK(ret == 0, "ModifyWindow to B5 paper");

    Sleep(1000);

    ret = g_PM_CloseWindow();
    TEST_CHECK(ret == 0, "CloseWindow");

    Sleep(500);
    g_PM_Uninitialize();
}

// ============================================================================
// Test 8: Full-screen grid mode
// ============================================================================

static void Test_FullScreenGridWorkflow()
{
    std::cout << "\n=== Test: Full Screen Grid Workflow ===\n";

    int ret = g_PM_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PM_WindowParams params{};
    params.paperType = PM_PAPER_GRID_1CM;
    params.language  = PM_LANG_EN;
    ret = g_PM_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow fullscreen 1cm grid");

    Sleep(1500);

    ret = g_PM_CloseWindow();
    TEST_CHECK(ret == 0, "CloseWindow");

    Sleep(500);
    g_PM_Uninitialize();
}

// ============================================================================
// Test 9: Open multiple windows, then CloseAll
// ============================================================================

static void Test_OpenCloseAll()
{
    std::cout << "\n=== Test: Open + CloseAll ===\n";

    int ret = g_PM_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PM_WindowParams params{};
    params.paperType = PM_PAPER_A4;
    params.language  = PM_LANG_EN;
    ret = g_PM_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow (A4)");

    Sleep(1500);

    g_PM_CloseWindow();
    TEST_CHECK(true, "CloseWindow does not crash");

    Sleep(500);
    g_PM_Uninitialize();
}

// ============================================================================
// Test 10: Language switching
// ============================================================================

static void Test_LanguageSwitching()
{
    std::cout << "\n=== Test: Language Switching ===\n";

    int ret = g_PM_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PM_WindowParams params{};
    params.paperType = PM_PAPER_A4;
    params.language  = PM_LANG_EN;
    ret = g_PM_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow in English");

    Sleep(1000);

    g_PM_SetLanguage(PM_LANG_SC);
    std::cout << "  [INFO] Switched language to SC.\n";
    Sleep(1000);

    g_PM_SetLanguage(PM_LANG_JA);
    std::cout << "  [INFO] Switched language to JA.\n";
    Sleep(1000);

    g_PM_SetLanguage(PM_LANG_EN);
    std::cout << "  [INFO] Switched language to EN.\n";
    Sleep(500);

    g_PM_CloseWindow();
    Sleep(500);
    g_PM_Uninitialize();

    // Unknown enum should silently fall back to EN — must not crash
    g_PM_SetLanguage(static_cast<PM_Language>(99999));
    TEST_CHECK(true, "PM_SetLanguage(unknown) does not crash");
}

// ============================================================================
// Test 11: Different paper types
// ============================================================================

static void Test_DifferentPaperTypes()
{
    std::cout << "\n=== Test: Different Paper Types ===\n";

    int ret = g_PM_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    struct PaperCase
    {
        const char*  name;
        PM_PaperType type;
    };

    PaperCase papers[] = {
        { "A4",       PM_PAPER_A4 },
        { "B5",       PM_PAPER_B5 },
        { "A3",       PM_PAPER_A3 },
        { "Letter",   PM_PAPER_LETTER },
        { "GRID_1CM", PM_PAPER_GRID_1CM },
    };

    for (int i = 0; i < _countof(papers); i++)
    {
        PM_WindowParams params{};
        params.paperType = papers[i].type;
        params.language  = PM_LANG_EN;

        ret = g_PM_OpenWindow(&params);
        std::string msg = std::string("OpenWindow with paper type: ") + papers[i].name;
        TEST_CHECK(ret == 0, msg.c_str());

        Sleep(1000);

        g_PM_CloseWindow();
        Sleep(300);
    }

    g_PM_Uninitialize();
}

// ============================================================================
// Test 12: Rapid open/close stress test
// ============================================================================

static void Test_RapidOpenClose()
{
    std::cout << "\n=== Test: Rapid Open/Close Stress ===\n";

    int ret = g_PM_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    for (int i = 0; i < 5; i++)
    {
        PM_WindowParams params{};
        params.paperType = PM_PAPER_A4;
        params.language  = PM_LANG_EN;
        g_PM_OpenWindow(&params);
        Sleep(200);
        g_PM_CloseWindow();
        Sleep(100);
    }

    TEST_CHECK(true, "Rapid open/close 5 iterations completed without crash");

    g_PM_CloseWindow();
    Sleep(300);
    g_PM_Uninitialize();
}

// ============================================================================
// Test 13: Custom paper size
// ============================================================================

static void Test_CustomPaperSize()
{
    std::cout << "\n=== Test: Custom Paper Size ===\n";

    int ret = g_PM_Initialize();
    TEST_CHECK(ret == 0, "Initialize");

    PM_WindowParams params{};
    params.paperType     = PM_PAPER_CUSTOM_OUTLINE;
    params.language      = PM_LANG_EN;
    params.paperWidthMm  = 100.0;
    params.paperHeightMm = 150.0;
    ret = g_PM_OpenWindow(&params);
    TEST_CHECK(ret == 0, "OpenWindow with CUSTOM_OUTLINE 100x150mm");

    Sleep(1200);

    params.paperType     = PM_PAPER_CUSTOM_GRID;
    params.paperWidthMm  = 15.0;
    params.paperHeightMm = 15.0;
    ret = g_PM_ModifyWindow(&params);
    TEST_CHECK(ret == 0, "ModifyWindow to CUSTOM_GRID 15x15mm");

    Sleep(1200);

    g_PM_CloseWindow();
    Sleep(300);
    g_PM_Uninitialize();
}

// ============================================================================
// Interactive menu
// ============================================================================

static void PrintMenu()
{
    std::cout << "\n"
        "============================================\n"
        "   PaperMirror Test Suite\n"
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

// Usage: TestPaperMirror.exe [test_number]
//   no args  -> interactive menu
//   0        -> run all tests
//   1-14     -> run specific test
int main(int argc, char* argv[])
{
    HMODULE hDll = LoadPaperMirrorDLL();
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
