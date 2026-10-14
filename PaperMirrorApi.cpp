#include "pch.h"
#include "PaperMirrorApi.h"
#include "PMUIThread.h"
#include "PMLanguage.h"
#include "PMMonitorRegistry.h"

PM_API int PM_Initialize()
{
    return PMUIThread::GetInstance().Initialize();
}

PM_API void PM_Uninitialize()
{
    PMUIThread::GetInstance().Uninitialize();
}

PM_API int PM_OpenWindow(const PM_WindowParams* params)
{
    if (!params) return -1;
    if (!PMUIThread::GetInstance().IsInitialized()) return -2;

    PMLanguage::GetInstance().SetLanguage(params->language);

    PMUIThread::GetInstance().PostOpen(params);
    return 0;
}

PM_API int PM_ModifyWindow(const PM_WindowParams* params)
{
    if (!params) return -1;
    if (!PMUIThread::GetInstance().IsInitialized()) return -2;

    PMUIThread::GetInstance().PostModify(params);
    return 0;
}

PM_API int PM_CloseWindow()
{
    if (!PMUIThread::GetInstance().IsInitialized()) return -2;

    PMUIThread::GetInstance().PostClose();
    return 0;
}

PM_API void PM_SetLanguage(PM_Language language)
{
    PMLanguage::GetInstance().SetLanguage(language);

    if (PMUIThread::GetInstance().IsInitialized())
        PMUIThread::GetInstance().PostSetLanguage(language);
}
