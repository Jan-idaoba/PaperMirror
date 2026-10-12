#include "pch.h"
#include "PrintAssitWndApi.h"
#include "PAUIThread.h"
#include "PALanguage.h"
#include "PAMonitorRegistry.h"

PA_API int PA_Initialize()
{
    return PAUIThread::GetInstance().Initialize();
}

PA_API void PA_Uninitialize()
{
    PAUIThread::GetInstance().Uninitialize();
}

PA_API int PA_OpenWindow(const PA_WindowParams* params)
{
    if (!params) return -1;
    if (!PAUIThread::GetInstance().IsInitialized()) return -2;

    PALanguage::GetInstance().SetLanguage(params->language);

    PAUIThread::GetInstance().PostOpen(params);
    return 0;
}

PA_API int PA_ModifyWindow(const PA_WindowParams* params)
{
    if (!params) return -1;
    if (!PAUIThread::GetInstance().IsInitialized()) return -2;

    PAUIThread::GetInstance().PostModify(params);
    return 0;
}

PA_API int PA_CloseWindow()
{
    if (!PAUIThread::GetInstance().IsInitialized()) return -2;

    PAUIThread::GetInstance().PostClose();
    return 0;
}

PA_API void PA_SetLanguage(PA_Language language)
{
    PALanguage::GetInstance().SetLanguage(language);

    if (PAUIThread::GetInstance().IsInitialized())
        PAUIThread::GetInstance().PostSetLanguage(language);
}
