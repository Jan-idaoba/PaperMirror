#include <iostream>
#include "PaperMirrorApi.h"

int main()
{
    if (PM_Initialize() != 0)
    {
        std::cerr << "PM_Initialize failed.\n";
        return 1;
    }

    PM_WindowParams params{};
    params.paperType = PM_PAPER_CUSTOM_GRID;
    params.language  = PM_LANG_EN;
	params.paperHeightMm = 85.0;
	params.paperWidthMm = 85.0;

    int rc = PM_OpenWindow(&params);
    if (rc != 0)
    {
        std::cerr << "PM_OpenWindow failed (rc=" << rc << ").\n";
        PM_Uninitialize();
        return 1;
    }

    std::cout << "Window opened. Press Enter to close...\n";
    std::cin.get();

    PM_CloseWindow();
    PM_Uninitialize();
    return 0;
}
