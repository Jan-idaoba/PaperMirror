#pragma once

#include <string>
#include <map>
#include <mutex>

#include "PrintAssitWndApi.h"

class PALanguage
{
public:
    static PALanguage& GetInstance();

    void SetLanguage(PA_Language lang);
    PA_Language GetCurrentLanguage() const;

    const wchar_t* Translate(const char* key) const;

private:
    PALanguage();

    std::map<PA_Language, std::map<std::string, std::wstring>> m_allStrings;
    PA_Language m_currentLang;
    mutable std::mutex m_mutex;
};
