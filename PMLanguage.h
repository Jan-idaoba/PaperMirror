#pragma once

#include <string>
#include <map>
#include <mutex>

#include "PaperMirrorApi.h"

class PMLanguage
{
public:
    static PMLanguage& GetInstance();

    void SetLanguage(PM_Language lang);
    PM_Language GetCurrentLanguage() const;

    const wchar_t* Translate(const char* key) const;

private:
    PMLanguage();

    std::map<PM_Language, std::map<std::string, std::wstring>> m_allStrings;
    PM_Language m_currentLang;
    mutable std::mutex m_mutex;
};
