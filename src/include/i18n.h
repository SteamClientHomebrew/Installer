#pragma once
#include <string>
#include <vector>

class Locale {
public:
    struct Language {
        std::string id;
        std::string displayName;
    };

    /** Detect system language, load translations, set current language. */
    static void Initialize();

    /** Switch to a different language at runtime. */
    static void SetLanguage(const std::string& langId);

    /** Returns e.g. "russian", "english". */
    static const std::string& GetCurrentLanguageId();

    /** Look up a translated string. Falls back to English if key/lang not found. */
    static const char* Get(const std::string& key);

    /** All available languages for the selector dropdown. */
    static const std::vector<Language>& GetAvailableLanguages();

    /** True if current language uses Cyrillic (load extra font ranges). */
    static bool UsesCyrillic();

    /** True if current language uses CJK (Chinese/Japanese/Korean). */
    static bool UsesCJK();

private:
    static std::string DetectSystemLanguage();
};
