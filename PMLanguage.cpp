#include "pch.h"
#include "PMLanguage.h"

PMLanguage& PMLanguage::GetInstance()
{
    static PMLanguage instance;
    return instance;
}

PMLanguage::PMLanguage()
    : m_currentLang(PM_LANG_EN)
{
    // English
    m_allStrings[PM_LANG_EN]["FullScreen"] = L"Full Screen";
    m_allStrings[PM_LANG_EN]["Close"]      = L"Close";

    // Simplified Chinese
    m_allStrings[PM_LANG_SC]["FullScreen"] = L"\x5168\x5C4F";
    m_allStrings[PM_LANG_SC]["Close"]      = L"\x5173\x95ED";

    // Traditional Chinese
    m_allStrings[PM_LANG_TC]["FullScreen"] = L"\x5168\x87A2\x5E55";
    m_allStrings[PM_LANG_TC]["Close"]      = L"\x95DC\x9589";

    // Japanese
    m_allStrings[PM_LANG_JA]["FullScreen"] = L"\x30D5\x30EB\x30B9\x30AF\x30EA\x30FC\x30F3";
    m_allStrings[PM_LANG_JA]["Close"]      = L"\x9589\x3058\x308B";

    // Korean
    m_allStrings[PM_LANG_KO]["FullScreen"] = L"\xC804\xCCB4 \xD654\xBA74";
    m_allStrings[PM_LANG_KO]["Close"]      = L"\xB2EB\xAE30";

    // German
    m_allStrings[PM_LANG_GE]["FullScreen"] = L"Vollbild";
    m_allStrings[PM_LANG_GE]["Close"]      = L"Schlie\x00DF" L"en";

    // French
    m_allStrings[PM_LANG_FR]["FullScreen"] = L"Plein \x00E9" L"cran";
    m_allStrings[PM_LANG_FR]["Close"]      = L"Fermer";

    // Spanish
    m_allStrings[PM_LANG_SP]["FullScreen"] = L"Pantalla completa";
    m_allStrings[PM_LANG_SP]["Close"]      = L"Cerrar";

    // Portuguese (Brazil)
    m_allStrings[PM_LANG_PB]["FullScreen"] = L"Tela cheia";
    m_allStrings[PM_LANG_PB]["Close"]      = L"Fechar";

    // Italian
    m_allStrings[PM_LANG_IT]["FullScreen"] = L"Schermo intero";
    m_allStrings[PM_LANG_IT]["Close"]      = L"Chiudi";

    // Dutch
    m_allStrings[PM_LANG_DU]["FullScreen"] = L"Volledig scherm";
    m_allStrings[PM_LANG_DU]["Close"]      = L"Sluiten";

    // Danish
    m_allStrings[PM_LANG_DA]["FullScreen"] = L"Fuld sk\x00E6" L"rm";
    m_allStrings[PM_LANG_DA]["Close"]      = L"Luk";

    // Swedish
    m_allStrings[PM_LANG_SW]["FullScreen"] = L"Helsk\x00E4rm";
    m_allStrings[PM_LANG_SW]["Close"]      = L"St\x00E4ng";

    // Norwegian
    m_allStrings[PM_LANG_NO]["FullScreen"] = L"Fullskjerm";
    m_allStrings[PM_LANG_NO]["Close"]      = L"Lukk";

    // Finnish
    m_allStrings[PM_LANG_FI]["FullScreen"] = L"Koko n\x00E4ytt\x00F6";
    m_allStrings[PM_LANG_FI]["Close"]      = L"Sulje";

    // Arabic
    m_allStrings[PM_LANG_AR]["FullScreen"] = L"\x0634\x0627\x0634\x0629 \x0643\x0627\x0645\x0644\x0629";
    m_allStrings[PM_LANG_AR]["Close"]      = L"\x0625\x063A\x0644\x0627\x0642";

    // Hebrew
    m_allStrings[PM_LANG_HE]["FullScreen"] = L"\x05DE\x05E1\x05DA \x05DE\x05DC\x05D0";
    m_allStrings[PM_LANG_HE]["Close"]      = L"\x05E1\x05D2\x05D5\x05E8";

    // Russian
    m_allStrings[PM_LANG_RU]["FullScreen"] = L"\x041F\x043E\x043B\x043D\x044B\x0439 \x044D\x043A\x0440\x0430\x043D";
    m_allStrings[PM_LANG_RU]["Close"]      = L"\x0417\x0430\x043A\x0440\x044B\x0442\x044C";
}

void PMLanguage::SetLanguage(PM_Language lang)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // Validate against the populated table; unknown values fall back to EN.
    if (m_allStrings.find(lang) == m_allStrings.end())
        lang = PM_LANG_EN;
    m_currentLang = lang;
}

PM_Language PMLanguage::GetCurrentLanguage() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentLang;
}

const wchar_t* PMLanguage::Translate(const char* key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto langIt = m_allStrings.find(m_currentLang);
    if (langIt == m_allStrings.end())
        langIt = m_allStrings.find(PM_LANG_EN);
    if (langIt == m_allStrings.end())
        return L"";

    auto strIt = langIt->second.find(key);
    if (strIt == langIt->second.end())
        return L"";

    return strIt->second.c_str();
}
