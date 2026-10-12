#include <iostream>
#include "PrintAssitWndApi.h"

int main()
{
    if (PA_Initialize() != 0)
    {
        std::cerr << "PA_Initialize failed.\n";
        return 1;
    }

    PA_WindowParams params{};
    params.paperType = PA_PAPER_CUSTOM_GRID;
    params.language  = PA_LANG_EN;
	params.paperHeightMm = 85.0;
	params.paperWidthMm = 85.0;

    int rc = PA_OpenWindow(&params);
    if (rc != 0)
    {
        std::cerr << "PA_OpenWindow failed (rc=" << rc << ").\n";
        PA_Uninitialize();
        return 1;
    }

    std::cout << "Window opened. Press Enter to close...\n";
    std::cin.get();

    PA_CloseWindow();
    PA_Uninitialize();
    return 0;
}
