/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "InternationalizationSystem.h"
#include <algorithm>
#include <fstream>
#include <sstream>

#ifndef JUCE_DISABLE_ICU
    // ICU is available - include headers
    #include <unicode/ucnv.h>
    #include <unicode/ubidi.h>
    #include <unicode/ustring.h>
    #include <unicode/ushape.h>
#else
    // Fallback when ICU is not available
    #warning "ICU library not available - Internationalization features will be limited"
#endif

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

InternationalizationSystem::InternationalizationSystem()
    : debugMode_(false)
{
    initializeLanguages();
    initializeLanguageDirections();
    initializeFontMappings();

    // Start timer for periodic updates
    startTimerHz(30); // 30 Hz for UI updates
    lastTranslationLoadTime_ = 0;
}

InternationalizationSystem::~InternationalizationSystem() {
    stopTimer();
    translationCache_.clear();
    translations_.clear();
    translationMemory_.clear();
}

//==============================================================================
// Language Management
//==============================================================================

void InternationalizationSystem::setLanguage(Language language) {
    if (config_.currentLanguage == language) {
        return;
    }

    Language previousLanguage = config_.currentLanguage;
    config_.currentLanguage = language;
    config_.locale = getLanguageCode(language);

    // Update text direction for RTL languages
    config_.forceDirection = getLanguageDirection(language);

    // Clear translation cache when language changes
    translationCache_.clear();

    // Trigger callback
    if (onLanguageChanged) {
        onLanguageChanged(language);
    }
}


juce::String InternationalizationSystem::getLanguageDisplayName(Language language) const {
    auto it = languageNames_.find(language);
    if (it != languageNames_.end()) {
        return it->second;
    }
    return "English";
}

WritingDirection InternationalizationSystem::getLanguageDirection(Language language) const {
    auto it = languageDirections_.find(language);
    if (it != languageDirections_.end()) {
        return it->second;
    }
    return WritingDirection::LeftToRight;
}

juce::String InternationalizationSystem::getLanguageCode(Language language) const {
    auto it = languageCodes_.find(language);
    if (it != languageCodes_.end()) {
        return it->second;
    }
    return "en-US";
}

std::vector<Language> InternationalizationSystem::getSupportedLanguages() const {
    std::vector<Language> languages;
    for (const auto& pair : languageNames_) {
        languages.push_back(pair.first);
    }
    return languages;
}

//==============================================================================
// Translation Management
//==============================================================================

bool InternationalizationSystem::loadTranslations(Language language, const juce::String& filePath) {
    juce::File file(filePath);
    if (!file.existsAsFile()) {
        if (debugMode_) {
            DBG("InternationalizationSystem: Translation file not found: " + filePath);
        }
        return false;
    }

    int count = 0;
    bool success = false;

    // Try JSON first, then CSV
    if (filePath.endsWithIgnoreCase(".json")) {
        success = loadTranslationsFromJSON(filePath);
    } else if (filePath.endsWithIgnoreCase(".csv")) {
        success = loadTranslationsFromCSV(filePath);
    }

    if (success) {
        // Count loaded translations
        for (const auto& pair : translations_) {
            if (pair.second.translation.isNotEmpty()) {
                count++;
            }
        }

        lastTranslationLoadTime_ = juce::Time::getCurrentTime().toMilliseconds();

        // Trigger callback
        if (onTranslationsLoaded) {
            onTranslationsLoaded(language, count);
        }
    }

    return success;
}

void InternationalizationSystem::addTranslation(const TranslationData& translation) {
    juce::String key = createTranslationHash(translation.source,
                                             translation.context,
                                             translation.gender);
    translations_[key] = translation;
    translationCache_.erase(key); // Invalidate cache
}

juce::String InternationalizationSystem::translate(const juce::String& text,
                                                   const juce::String& context,
                                                   const juce::String& gender)
{
    // Check cache first
    juce::String key = createTranslationHash(text, context, gender);
    if (config_.cacheTranslations) {
        juce::String cached = getTranslationFromCache(key);
        if (cached.isNotEmpty()) {
            return cached;
        }
    }

    // Look up translation
    auto it = translations_.find(key);
    juce::String result;

    if (it != translations_.end() && it->second.translation.isNotEmpty()) {
        result = it->second.translation;
    } else {
        // Fallback to source text
        result = text;

        if (debugMode_ && config_.logTranslationIssues) {
            DBG("InternationalizationSystem: Missing translation for: " + text);
            unsupportedTextWarnings_.push_back("Missing: " + text);
        }

        if (onTranslationMissing) {
            onTranslationMissing(text, context);
        }
    }

    // Cache the result
    if (config_.cacheTranslations) {
        addTranslationToCache(key, result);
    }

    return result;
}

juce::String InternationalizationSystem::translatePlural(const juce::String& text,
                                                         int count,
                                                         const juce::String& context)
{
    // Get plural form for current language
    int pluralForm = getPluralForm(count, config_.currentLanguage);

    // Create key with plural form suffix
    juce::String pluralKey = text + "_plural_" + juce::String(pluralForm);
    juce::String key = createTranslationHash(pluralKey, context, "");

    // Check cache
    if (config_.cacheTranslations) {
        juce::String cached = getTranslationFromCache(key);
        if (cached.isNotEmpty()) {
            return cached;
        }
    }

    // Look up translation
    auto it = translations_.find(key);
    juce::String result;

    if (it != translations_.end() && it->second.translation.isNotEmpty()) {
        result = it->second.translation;
    } else {
        // Fallback: replace {n} with actual count
        result = text;
        result = result.replace("{n}", juce::String(count));
    }

    return result;
}

float InternationalizationSystem::getTranslationConfidence(const juce::String& sourceText,
                                                           const juce::String& translatedText) const
{
    // Check translation memory
    for (const auto& entry : translationMemory_) {
        if (entry.sourceText == sourceText && entry.translatedText == translatedText) {
            return entry.confidence;
        }
    }
    return 0.0f;
}

//==============================================================================
// Text Processing
//==============================================================================

TextShapingInfo InternationalizationSystem::shapeText(const juce::String& text,
                                                      Language language) const
{
    TextShapingInfo info;

    info.requiresShaping = needsTextShaping(text, language);
    info.isComplexScript = (language == Language::Arabic ||
                           language == Language::Hebrew ||
                           language == Language::Hindi ||
                           language == Language::Thai ||
                           language == Language::Burmese ||
                           language == Language::Khmer ||
                           language == Language::Lao);

    info.hasLigatures = (language == Language::Arabic ||
                        language == Language::Urdu);

    info.hasDiacritics = (language == Language::Arabic ||
                         language == Language::Vietnamese ||
                         language == Language::Turkish);

    info.baseDirection = UBIDI_LTR;

    if (config_.enableRTLSupport) {
        WritingDirection dir = getLanguageDirection(language);
        if (dir == WritingDirection::RightToLeft) {
            info.baseDirection = UBIDI_RTL;
        } else if (dir == WritingDirection::TopToBottom) {
            info.baseDirection = UBIDI_DEFAULT_LTR;
        }
    }

    if (info.requiresShaping && config_.enableComplexTextShaping) {
        shapeComplexText(text, language);
    }

    return info;
}

juce::String InternationalizationSystem::processBidirectionalText(const juce::String& text,
                                                                  Language language) const
{
    WritingDirection direction = getLanguageDirection(language);

    if (direction == WritingDirection::LeftToRight && !config_.enableRTLSupport) {
        return text;
    }

    return applyBidiAlgorithm(text, direction);
}

juce::Point<float> InternationalizationSystem::adjustTextPosition(float x, float y,
                                                                  float width,
                                                                  WritingDirection direction) const
{
    if (direction == WritingDirection::RightToLeft) {
        // Mirror x position
        return juce::Point<float>(x - width + config_.rtlMargin, y);
    } else if (direction == WritingDirection::TopToBottom) {
        // Swap x and y for vertical text
        return juce::Point<float>(y, x);
    }
    return juce::Point<float>(x, y);
}

juce::Justification InternationalizationSystem::getTextAlignment(Language language) const {
    WritingDirection direction = getLanguageDirection(language);

    if (direction == WritingDirection::RightToLeft) {
        return juce::Justification::right;
    } else if (direction == WritingDirection::TopToBottom) {
        return juce::Justification::centredTop;
    }
    return juce::Justification::left;
}

juce::Font InternationalizationSystem::getFontForLanguage(Language language) const {
    auto it = fontMappings_.find(language);
    if (it != fontMappings_.end() && it->second.isNotEmpty()) {
        return juce::Font(it->second, 14.0f, 0);
    }

    // Use system font fallback
    juce::String fontName;
    if (language == Language::Japanese || language == Language::ChineseSimplified ||
        language == Language::ChineseTraditional || language == Language::Korean) {
        fontName = "Noto Sans CJK";
    } else if (language == Language::Arabic) {
        fontName = "Noto Sans Arabic";
    } else if (language == Language::Hebrew) {
        fontName = "Noto Sans Hebrew";
    } else if (language == Language::Thai) {
        fontName = "Noto Sans Thai";
    } else if (language == Language::Hindi) {
        fontName = "Noto Sans Devanagari";
    } else {
        fontName = juce::Font::getDefaultSansSerifFontName();
    }

    float fontSize = 14.0f * config_.fontSizeMultiplier;
    return juce::Font(fontName, fontSize, 0);
}

//==============================================================================
// Configuration
//==============================================================================

void InternationalizationSystem::setConfig(const I18nConfig& config) {
    config_ = config;
    translationCache_.clear();

    // Re-initialize if needed
    if (config.cacheTranslations) {
        // translationCache_ is a map, does not support reserve
    }
}

bool InternationalizationSystem::saveConfig(const juce::String& filePath) {
    juce::File file(filePath);
    std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());

    if (stream == nullptr || stream->failedToOpen()) {
        return false;
    }

    // Write configuration as JSON
    stream->writeText("{\n", false, false, nullptr);
    stream->writeText("  \"language\": \"" + getLanguageCode(config_.currentLanguage) + "\",\n", false, false, nullptr);
    stream->writeText("  \"locale\": \"" + config_.locale + "\",\n", false, false, nullptr);
    stream->writeText("  \"rtlSupport\": " + juce::String(config_.enableRTLSupport ? "true" : "false") + ",\n", false, false, nullptr);
    stream->writeText("  \"fontSizeMultiplier\": " + juce::String(config_.fontSizeMultiplier) + "\n", false, false, nullptr);
    stream->writeText("}\n", false, false, nullptr);

    return true;
}

bool InternationalizationSystem::loadConfig(const juce::String& filePath) {
    juce::File file(filePath);

    if (!file.existsAsFile()) {
        return false;
    }

    // Simple JSON parsing (for production, use a proper JSON parser)
    juce::String content = file.loadFileAsString();

    // Parse language
    int langIndex = content.indexOf("\"language\":");
    if (langIndex >= 0) {
        int startQuote = content.indexOfChar(langIndex, '"') + 1;
        int endQuote = content.indexOfChar(startQuote, '"');
        juce::String langCode = content.substring(startQuote, endQuote);

        // Find matching language
        for (const auto& pair : languageCodes_) {
            if (pair.second == langCode) {
                config_.currentLanguage = pair.first;
                config_.locale = langCode;
                break;
            }
        }
    }

    return true;
}

//==============================================================================
// Translation Memory
//==============================================================================

void InternationalizationSystem::addToTranslationMemory(const juce::String& sourceText,
                                                       const juce::String& translatedText,
                                                       const juce::String& context)
{
    TranslationMemory entry;
    entry.sourceText = sourceText;
    entry.translatedText = translatedText;
    entry.language = getLanguageCode(config_.currentLanguage);
    entry.context = context;
    entry.timestamp = juce::Time::getCurrentTime().toMilliseconds();
    entry.confidence = 0.8f; // Default confidence for manual translations
    entry.isAutoTranslated = false;

    translationMemory_.push_back(entry);

    // Limit memory size
    if (translationMemory_.size() > 10000) {
        translationMemory_.erase(translationMemory_.begin());
    }
}

TranslationMemory InternationalizationSystem::searchTranslationMemory(const juce::String& sourceText,
                                                                     const juce::String& context) const
{
    // Search for exact match with context
    for (const auto& entry : translationMemory_) {
        if (entry.sourceText == sourceText && entry.context == context) {
            return entry;
        }
    }

    // Search for exact match without context
    for (const auto& entry : translationMemory_) {
        if (entry.sourceText == sourceText) {
            return entry;
        }
    }

    // Return empty entry if not found
    TranslationMemory empty;
    empty.confidence = 0.0f;
    return empty;
}

void InternationalizationSystem::clearTranslationMemory() {
    translationMemory_.clear();
}

//==============================================================================
// Debug Methods
//==============================================================================

void InternationalizationSystem::setDebugMode(bool enabled) {
    debugMode_ = enabled;
}

juce::String InternationalizationSystem::getTranslationStatistics() const {
    juce::String stats;
    stats << "Language: " << getLanguageDisplayName(config_.currentLanguage) << "\n";
    stats << "Total translations: " << translations_.size() << "\n";
    stats << "Cache size: " << translationCache_.size() << "\n";
    stats << "Translation memory entries: " << translationMemory_.size() << "\n";
    stats << "RTL support: " << (config_.enableRTLSupport ? "enabled" : "disabled") << "\n";
    return stats;
}

std::vector<juce::String> InternationalizationSystem::getUnsupportedTextWarnings() const {
    return unsupportedTextWarnings_;
}

//==============================================================================
// Timer Callback
//==============================================================================

void InternationalizationSystem::timerCallback() {
    // Periodic cleanup of old warnings
    if (unsupportedTextWarnings_.size() > 1000) {
        unsupportedTextWarnings_.erase(unsupportedTextWarnings_.begin(),
                                      unsupportedTextWarnings_.begin() + 500);
    }

    // Clean old translation memory entries
    juce::uint64 currentTime = juce::Time::getCurrentTime().toMilliseconds();
    const juce::uint64 maxAge = 30 * 24 * 60 * 60 * 1000; // 30 days

    translationMemory_.erase(
        std::remove_if(translationMemory_.begin(), translationMemory_.end(),
                      [currentTime, maxAge](const TranslationMemory& entry) {
                          return (currentTime - entry.timestamp) > maxAge;
                      }),
        translationMemory_.end());
}

//==============================================================================
// File Methods
//==============================================================================

bool InternationalizationSystem::exportTranslations(const juce::String& filePath) {
    juce::File file(filePath);
    std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());

    if (stream == nullptr || stream->failedToOpen()) {
        return false;
    }

    // Export as CSV
    stream->writeText("Source,Translation,Context,Gender\n", false, false, nullptr);

    for (const auto& pair : translations_) {
        const TranslationData& data = pair.second;
        juce::String line;

        // Escape quotes
        juce::String source = data.source.replace("\"", "\"\"");
        juce::String translation = data.translation.replace("\"", "\"\"");
        juce::String context = data.context.replace("\"", "\"\"");

        line << "\"" << source << "\",\""
             << translation << "\",\""
             << context << "\",\""
             << data.gender << "\"\n";

        stream->writeText(line, false, false, nullptr);
    }

    return true;
}

bool InternationalizationSystem::importTranslations(const juce::String& filePath) {
    return loadTranslations(config_.currentLanguage, filePath);
}

//==============================================================================
// Private: Language Management
//==============================================================================

void InternationalizationSystem::initializeLanguages() {
    // English
    languageNames_[Language::English] = "English (United States)";
    languageNames_[Language::EnglishUK] = "English (United Kingdom)";
    languageNames_[Language::Spanish] = "Español (España)";
    languageNames_[Language::French] = "Français (France)";
    languageNames_[Language::German] = "Deutsch (Deutschland)";
    languageNames_[Language::Italian] = "Italiano (Italia)";
    languageNames_[Language::Portuguese] = "Português (Brasil)";
    languageNames_[Language::Russian] = "Русский (Россия)";
    languageNames_[Language::ChineseSimplified] = "中文 (简体)";
    languageNames_[Language::ChineseTraditional] = "中文 (繁體)";
    languageNames_[Language::Japanese] = "日本語";
    languageNames_[Language::Korean] = "한국어 (한국)";
    languageNames_[Language::Arabic] = "العربية";
    languageNames_[Language::Hebrew] = "עברית";
    languageNames_[Language::Persian] = "فارسی";
    languageNames_[Language::Urdu] = "اردو";
    languageNames_[Language::Hindi] = "हिन्दी";
    languageNames_[Language::Thai] = "ไทย";
    languageNames_[Language::Vietnamese] = "Tiếng Việt";
    languageNames_[Language::Turkish] = "Türkçe";
    languageNames_[Language::Dutch] = "Nederlands";
    languageNames_[Language::Swedish] = "Svenska";
    languageNames_[Language::Norwegian] = "Norsk";
    languageNames_[Language::Finnish] = "Suomi";
    languageNames_[Language::Polish] = "Polski";
    languageNames_[Language::Czech] = "Čeština";
    languageNames_[Language::Hungarian] = "Magyar";
    languageNames_[Language::Romanian] = "Română";
    languageNames_[Language::Greek] = "Ελληνικά";
    languageNames_[Language::Bulgarian] = "Български";
    languageNames_[Language::Ukrainian] = "Українська";
    languageNames_[Language::Serbian] = "Српски";
    languageNames_[Language::Croatian] = "Hrvatski";
    languageNames_[Language::Slovak] = "Slovenčina";
    languageNames_[Language::Slovenian] = "Slovenščina";
    languageNames_[Language::Estonian] = "Eesti";
    languageNames_[Language::Latvian] = "Latviešu";
    languageNames_[Language::Lithuanian] = "Lietuvių";
    languageNames_[Language::Danish] = "Dansk";
    languageNames_[Language::Icelandic] = "Íslenska";
    languageNames_[Language::Albanian] = "Shqip";
    languageNames_[Language::Macedonian] = "Македонски";
    languageNames_[Language::Georgian] = "ქართული";
    languageNames_[Language::Armenian] = "Հայերեն";
    languageNames_[Language::Azerbaijani] = "Azərbaycan";
    languageNames_[Language::Kazakh] = "Қазақша";
    languageNames_[Language::Uzbek] = "Oʻzbekcha";
    languageNames_[Language::Mongolian] = "Монгол";
    languageNames_[Language::Nepali] = "नेपाली";
    languageNames_[Language::Bengali] = "বাংলা";
    languageNames_[Language::Punjabi] = "ਪੰਜਾਬੀ";
    languageNames_[Language::Tamil] = "தமிழ்";
    languageNames_[Language::Telugu] = "తెలుగు";
    languageNames_[Language::Kannada] = "ಕನ್ನಡ";
    languageNames_[Language::Malayalam] = "മലയാളം";
    languageNames_[Language::Sinhala] = "සිංහල";
    languageNames_[Language::Khmer] = "ខ្មែរ";
    languageNames_[Language::Lao] = "ລາວ";
    languageNames_[Language::Burmese] = "မြန်မာ";
    languageNames_[Language::Amharic] = "አማርኛ";
    languageNames_[Language::Swahili] = "Kiswahili";
    languageNames_[Language::Hausa] = "Hausa";
    languageNames_[Language::Yoruba] = "Yorùbá";
    languageNames_[Language::Zulu] = "isiZulu";
    languageNames_[Language::Xhosa] = "isiXhosa";
    languageNames_[Language::Afrikaans] = "Afrikaans";
    languageNames_[Language::Filipino] = "Filipino";
    languageNames_[Language::Malaysian] = "Bahasa Melayu";
    languageNames_[Language::Indonesian] = "Bahasa Indonesia";
    languageNames_[Language::Brunei] = "Bahasa Brunei";
    languageNames_[Language::Singapore] = "Singapore";
    languageNames_[Language::HongKong] = "香港";
    languageNames_[Language::Taiwan] = "台灣";
    languageNames_[Language::Macau] = "澳門";
}

void InternationalizationSystem::initializeLanguageDirections() {
    // Left-to-Right languages
    languageDirections_[Language::English] = WritingDirection::LeftToRight;
    languageDirections_[Language::EnglishUK] = WritingDirection::LeftToRight;
    languageDirections_[Language::Spanish] = WritingDirection::LeftToRight;
    languageDirections_[Language::French] = WritingDirection::LeftToRight;
    languageDirections_[Language::German] = WritingDirection::LeftToRight;
    languageDirections_[Language::Italian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Portuguese] = WritingDirection::LeftToRight;
    languageDirections_[Language::Russian] = WritingDirection::LeftToRight;
    languageDirections_[Language::ChineseSimplified] = WritingDirection::LeftToRight;
    languageDirections_[Language::ChineseTraditional] = WritingDirection::LeftToRight;
    languageDirections_[Language::Japanese] = WritingDirection::LeftToRight;
    languageDirections_[Language::Korean] = WritingDirection::LeftToRight;
    languageDirections_[Language::Hindi] = WritingDirection::LeftToRight;
    languageDirections_[Language::Thai] = WritingDirection::LeftToRight;
    languageDirections_[Language::Vietnamese] = WritingDirection::LeftToRight;
    languageDirections_[Language::Turkish] = WritingDirection::LeftToRight;
    languageDirections_[Language::Dutch] = WritingDirection::LeftToRight;
    languageDirections_[Language::Swedish] = WritingDirection::LeftToRight;
    languageDirections_[Language::Norwegian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Finnish] = WritingDirection::LeftToRight;
    languageDirections_[Language::Polish] = WritingDirection::LeftToRight;
    languageDirections_[Language::Czech] = WritingDirection::LeftToRight;
    languageDirections_[Language::Hungarian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Romanian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Greek] = WritingDirection::LeftToRight;
    languageDirections_[Language::Bulgarian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Ukrainian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Serbian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Croatian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Slovak] = WritingDirection::LeftToRight;
    languageDirections_[Language::Slovenian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Estonian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Latvian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Lithuanian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Danish] = WritingDirection::LeftToRight;
    languageDirections_[Language::Icelandic] = WritingDirection::LeftToRight;
    languageDirections_[Language::Albanian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Macedonian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Georgian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Armenian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Azerbaijani] = WritingDirection::LeftToRight;
    languageDirections_[Language::Kazakh] = WritingDirection::LeftToRight;
    languageDirections_[Language::Uzbek] = WritingDirection::LeftToRight;
    languageDirections_[Language::Mongolian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Nepali] = WritingDirection::LeftToRight;
    languageDirections_[Language::Bengali] = WritingDirection::LeftToRight;
    languageDirections_[Language::Punjabi] = WritingDirection::LeftToRight;
    languageDirections_[Language::Tamil] = WritingDirection::LeftToRight;
    languageDirections_[Language::Telugu] = WritingDirection::LeftToRight;
    languageDirections_[Language::Kannada] = WritingDirection::LeftToRight;
    languageDirections_[Language::Malayalam] = WritingDirection::LeftToRight;
    languageDirections_[Language::Sinhala] = WritingDirection::LeftToRight;
    languageDirections_[Language::Khmer] = WritingDirection::LeftToRight;
    languageDirections_[Language::Lao] = WritingDirection::LeftToRight;
    languageDirections_[Language::Burmese] = WritingDirection::LeftToRight;
    languageDirections_[Language::Amharic] = WritingDirection::LeftToRight;
    languageDirections_[Language::Swahili] = WritingDirection::LeftToRight;
    languageDirections_[Language::Hausa] = WritingDirection::LeftToRight;
    languageDirections_[Language::Yoruba] = WritingDirection::LeftToRight;
    languageDirections_[Language::Zulu] = WritingDirection::LeftToRight;
    languageDirections_[Language::Xhosa] = WritingDirection::LeftToRight;
    languageDirections_[Language::Afrikaans] = WritingDirection::LeftToRight;
    languageDirections_[Language::Filipino] = WritingDirection::LeftToRight;
    languageDirections_[Language::Malaysian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Indonesian] = WritingDirection::LeftToRight;
    languageDirections_[Language::Brunei] = WritingDirection::LeftToRight;
    languageDirections_[Language::Singapore] = WritingDirection::LeftToRight;
    languageDirections_[Language::HongKong] = WritingDirection::LeftToRight;
    languageDirections_[Language::Taiwan] = WritingDirection::LeftToRight;
    languageDirections_[Language::Macau] = WritingDirection::LeftToRight;

    // Right-to-Left languages
    languageDirections_[Language::Arabic] = WritingDirection::RightToLeft;
    languageDirections_[Language::Hebrew] = WritingDirection::RightToLeft;
    languageDirections_[Language::Persian] = WritingDirection::RightToLeft;
    languageDirections_[Language::Urdu] = WritingDirection::RightToLeft;
}

void InternationalizationSystem::initializeFontMappings() {
    fontMappings_[Language::Japanese] = "Noto Sans JP";
    fontMappings_[Language::ChineseSimplified] = "Noto Sans SC";
    fontMappings_[Language::ChineseTraditional] = "Noto Sans TC";
    fontMappings_[Language::Korean] = "Noto Sans KR";
    fontMappings_[Language::Arabic] = "Noto Sans Arabic";
    fontMappings_[Language::Hebrew] = "Noto Sans Hebrew";
    fontMappings_[Language::Thai] = "Noto Sans Thai";
    fontMappings_[Language::Hindi] = "Noto Sans Devanagari";
    fontMappings_[Language::Bengali] = "Noto Sans Bengali";
    fontMappings_[Language::Tamil] = "Noto Sans Tamil";
    fontMappings_[Language::Telugu] = "Noto Sans Telugu";
    fontMappings_[Language::Kannada] = "Noto Sans Kannada";
    fontMappings_[Language::Malayalam] = "Noto Sans Malayalam";
    fontMappings_[Language::Sinhala] = "Noto Sans Sinhala";
    fontMappings_[Language::Khmer] = "Noto Sans Khmer";
    fontMappings_[Language::Lao] = "Noto Sans Lao";
    fontMappings_[Language::Burmese] = "Noto Sans Myanmar";
    fontMappings_[Language::Armenian] = "Noto Sans Armenian";
    fontMappings_[Language::Georgian] = "Noto Sans Georgian";
    fontMappings_[Language::Amharic] = "Noto Sans Ethiopic";
}

//==============================================================================
// Private: Translation Management
//==============================================================================

bool InternationalizationSystem::loadTranslationsFromJSON(const juce::String& filePath) {
    juce::File file(filePath);

    if (!file.existsAsFile()) {
        return false;
    }

    juce::String content = file.loadFileAsString();

    // Simple JSON parsing (note: for production, use a proper JSON parser)
    // This is a simplified version that handles basic key-value pairs

    // Find translations array
    int translationsStart = content.indexOf("\"translations\"");
    if (translationsStart < 0) {
        return false;
    }

    // Extract array content (simplified)
    int arrayStart = content.indexOfChar(translationsStart, '[');
    if (arrayStart < 0) {
        return false;
    }

    // Parse entries
    int bracePos = content.indexOfChar(arrayStart, '{');
    while (bracePos > 0) {
        int endBracePos = content.indexOfChar(bracePos + 1, '}');
        if (endBracePos < 0) {
            break;
        }

        juce::String entry = content.substring(bracePos, endBracePos + 1);

        // Extract source
        int sourceIdx = entry.indexOf("\"source\":");
        if (sourceIdx >= 0) {
            int quoteStart = entry.indexOfChar(sourceIdx + 8, '"') + 1;
            int quoteEnd = entry.indexOfChar(quoteStart, '"');
            juce::String source = entry.substring(quoteStart, quoteEnd);

            // Extract translation
            int transIdx = entry.indexOf("\"translation\":");
            juce::String translation;
            if (transIdx >= 0) {
                quoteStart = entry.indexOfChar(transIdx + 13, '"') + 1;
                quoteEnd = entry.indexOfChar(quoteStart, '"');
                translation = entry.substring(quoteStart, quoteEnd);
            }

            // Extract context
            int ctxIdx = entry.indexOf("\"context\":");
            juce::String context;
            if (ctxIdx >= 0) {
                quoteStart = entry.indexOfChar(ctxIdx + 9, '"') + 1;
                quoteEnd = entry.indexOfChar(quoteStart, '"');
                context = entry.substring(quoteStart, quoteEnd);
            }

            // Extract gender
            int genderIdx = entry.indexOf("\"gender\":");
            juce::String gender;
            if (genderIdx >= 0) {
                quoteStart = entry.indexOfChar(genderIdx + 9, '"') + 1;
                quoteEnd = entry.indexOfChar(quoteStart, '"');
                gender = entry.substring(quoteStart, quoteEnd);
            }

            TranslationData data;
            data.source = source;
            data.translation = translation;
            data.context = context;
            data.gender = gender;

            addTranslation(data);
        }

        bracePos = content.indexOfChar(endBracePos + 1, '{');
    }

    return true;
}

bool InternationalizationSystem::loadTranslationsFromCSV(const juce::String& filePath) {
    juce::File file(filePath);

    if (!file.existsAsFile()) {
        return false;
    }

    juce::StringArray lines;
    file.readLines(lines);

    // Skip header
    for (int i = 1; i < lines.size(); ++i) {
        juce::String line = lines[i];

        if (line.isEmpty()) {
            continue;
        }

        // Parse CSV (handle quoted strings)
        juce::StringArray tokens;
        int tokenStart = 0;
        bool inQuotes = false;

        for (int j = 0; j < line.length(); ++j) {
            juce::juce_wchar c = line[j];

            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == ',' && !inQuotes) {
                juce::String token = line.substring(tokenStart, j);
                token = token.removeCharacters("\"");
                tokens.add(token);
                tokenStart = j + 1;
            }
        }

        // Add last token
        if (tokenStart < line.length()) {
            juce::String token = line.substring(tokenStart);
            token = token.removeCharacters("\"");
            tokens.add(token);
        }

        if (tokens.size() >= 2) {
            TranslationData data;
            data.source = tokens[0];
            data.translation = tokens[1];
            data.context = tokens.size() > 2 ? tokens[2] : "";
            data.gender = tokens.size() > 3 ? tokens[3] : "";

            addTranslation(data);
        }
    }

    return true;
}

juce::String InternationalizationSystem::createTranslationHash(const juce::String& text,
                                                               const juce::String& context,
                                                               const juce::String& gender) const
{
    // Create a unique hash key from the parameters
    juce::String key = text;
    key << "|" << context << "|" << gender;
    return key;
}

juce::String InternationalizationSystem::getTranslationFromCache(const juce::String& hash) const {
    auto it = translationCache_.find(hash);
    if (it != translationCache_.end()) {
        return it->second;
    }
    return juce::String();
}

void InternationalizationSystem::addTranslationToCache(const juce::String& hash,
                                                       const juce::String& translation)
{
    if (config_.maxCacheSize > 0 && translationCache_.size() >= static_cast<size_t>(config_.maxCacheSize)) {
        // Simple FIFO eviction
        auto it = translationCache_.begin();
        translationCache_.erase(it);
    }
    translationCache_[hash] = translation;
}

bool InternationalizationSystem::needsTextShaping(const juce::String& text, Language language) const {
    // Complex scripts that need shaping
    if (language == Language::Arabic || language == Language::Hebrew ||
        language == Language::Hindi || language == Language::Thai ||
        language == Language::Burmese || language == Language::Khmer ||
        language == Language::Lao) {
        return true;
    }

    // Check for characters that need shaping
    for (int i = 0; i < text.length(); ++i) {
        juce::juce_wchar c = text[i];

        // Arabic range
        if (c >= 0x0600 && c <= 0x06FF) {
            return true;
        }
        // Hebrew range
        if (c >= 0x0590 && c <= 0x05FF) {
            return true;
        }
        // Indic scripts
        if (c >= 0x0900 && c <= 0x0DFF) {
            return true;
        }
        // Thai
        if (c >= 0x0E00 && c <= 0x0E7F) {
            return true;
        }
        // Myanmar
        if (c >= 0x1000 && c <= 0x109F) {
            return true;
        }
        // Khmer
        if (c >= 0x1780 && c <= 0x17FF) {
            return true;
        }
        // Lao
        if (c >= 0x0E80 && c <= 0x0EFF) {
            return true;
        }
    }

    return false;
}

TextShapingInfo InternationalizationSystem::shapeComplexText(const juce::String& text,
                                                             Language language) const
{
    TextShapingInfo info;

    info.requiresShaping = true;
    info.isComplexScript = true;

#ifndef JUCE_DISABLE_ICU
    // Use ICU for text shaping if available
    UErrorCode status = U_ZERO_ERROR;

    // Convert JUCE string to Unicode
    int32_t srcLength = text.length();
    unicodeBuffer_ = std::make_unique<UChar[]>(srcLength + 1);

    // Note: This is a simplified version
    // In production, you'd use proper ICU conversion functions
    for (int i = 0; i < srcLength; ++i) {
        unicodeBuffer_[i] = static_cast<UChar>(text[i]);
    }
    unicodeBuffer_[srcLength] = 0;

    // Get bidirectional information
    UBiDi* bidi = ubidi_open();
    if (bidi) {
        ubidi_setPara(bidi, unicodeBuffer_.get(), srcLength,
                     getLanguageDirection(language) == WritingDirection::RightToLeft ?
                     UBIDI_DEFAULT_RTL : UBIDI_DEFAULT_LTR,
                     nullptr, &status);

        if (U_SUCCESS(status)) {
            info.baseDirection = ubidi_getDirection(bidi);
        }

        ubidi_close(bidi);
    }
#endif

    return info;
}

juce::String InternationalizationSystem::applyBidiAlgorithm(const juce::String& text,
                                                           WritingDirection direction) const
{
#ifndef JUCE_DISABLE_ICU
    UErrorCode status = U_ZERO_ERROR;

    // Convert to Unicode
    int32_t srcLength = text.length();
    if (!bidiBuffer_ || srcLength > 1024) {
        bidiBuffer_ = std::make_unique<UChar[]>(srcLength + 1);
    }

    for (int i = 0; i < srcLength; ++i) {
        bidiBuffer_[i] = static_cast<UChar>(text[i]);
    }
    bidiBuffer_[srcLength] = 0;

    // Apply bidirectional algorithm
    UBiDi* bidi = ubidi_open();
    if (bidi) {
        UBiDiLevel paraLevel = (direction == WritingDirection::RightToLeft) ?
                              UBIDI_DEFAULT_RTL : UBIDI_DEFAULT_LTR;

        ubidi_setPara(bidi, bidiBuffer_.get(), srcLength, paraLevel, nullptr, &status);

        if (U_SUCCESS(status)) {
            // Get reordered text
            int32_t destLength = srcLength;
            ubidi_writeReordered(bidi, bidiBuffer_.get(), destLength,
                                UBIDI_DO_MIRRORING, &status);

            if (U_SUCCESS(status)) {
                // Convert back to JUCE string
                juce::String result;
                for (int32_t i = 0; i < destLength; ++i) {
                    result += static_cast<juce::juce_wchar>(bidiBuffer_[i]);
                }
                ubidi_close(bidi);
                return result;
            }
        }

        ubidi_close(bidi);
    }
#endif

    // Fallback: return original text
    return text;
}

int InternationalizationSystem::getPluralForm(int count, Language language) const {
    // Simplified plural form rules
    // In production, use ICU plural rules

    switch (language) {
        case Language::English:
        case Language::Spanish:
        case Language::Italian:
        case Language::German:
        case Language::Swedish:
        case Language::Norwegian:
        case Language::Danish:
        case Language::Finnish:
        case Language::Greek:
        case Language::Hungarian:
        case Language::ChineseSimplified:
        case Language::ChineseTraditional:
        case Language::Japanese:
        case Language::Korean:
        case Language::Vietnamese:
        case Language::Thai:
        case Language::Indonesian:
        case Language::Malaysian:
            // 1 form: everything (0, 1, 2+)
            return 0;

        case Language::French:
        case Language::Portuguese:
            // 2 forms: singular (0, 1), plural (2+)
            return (count <= 1) ? 0 : 1;

        case Language::Russian:
        case Language::Ukrainian:
        case Language::Polish:
        case Language::Czech:
        case Language::Slovak:
        case Language::Croatian:
        case Language::Serbian:
        case Language::Bulgarian:
            // 3 forms: special rules
            if (count == 1) return 0;
            if (count % 10 >= 2 && count % 10 <= 4 && (count % 100 < 10 || count % 100 >= 20)) return 1;
            return 2;

        case Language::Arabic:
            // 6 forms: complex Arabic rules
            if (count == 0) return 0;
            if (count == 1) return 1;
            if (count == 2) return 2;
            if (count % 100 >= 3 && count % 100 <= 10) return 3;
            if (count % 100 >= 11 && count % 100 <= 99) return 4;
            return 5;

        default:
            // Default to 2 forms
            return (count == 1) ? 0 : 1;
    }
}

} // namespace zenith::ui
