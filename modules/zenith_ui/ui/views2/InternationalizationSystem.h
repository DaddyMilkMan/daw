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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <memory>
#include <unicode/ubidi.h> // For bidirectional text support
#include <unicode/ustring.h> // For Unicode string handling

namespace zenith::ui {

//==============================================================================
// Locale and Language Support
//==============================================================================

/**
 * @brief Supported language enumeration
 */
enum class Language {
    English,          // English (United States)
    EnglishUK,        // English (United Kingdom)
    Spanish,          // Spanish (España)
    French,           // French (France)
    German,           // German (Germany)
    Italian,          // Italian (Italy)
    Portuguese,       // Portuguese (Brazil)
    Russian,          // Russian (Россия)
    ChineseSimplified, // Chinese (Simplified, 中国)
    ChineseTraditional, // Chinese (Traditional, 台灣)
    Japanese,         // Japanese (日本)
    Korean,           // Korean (한국)
    Arabic,           // Arabic (العربية)
    Hebrew,           // Hebrew (עברית)
    Persian,          // Persian (فارسی)
    Urdu,             // Urdu (اردو)
    Hindi,            // Hindi (हिन्दी)
    Thai,             // Thai (ไทย)
    Vietnamese,       // Vietnamese (Tiếng Việt)
    Turkish,          // Turkish (Türkçe)
    Dutch,            // Dutch (Nederland)
    Swedish,          // Swedish (Sverige)
    Norwegian,        // Norwegian (Norge)
    Finnish,          // Finnish (Suomi)
    Polish,           // Polish (Polska)
    Czech,            // Czech (Česká republika)
    Hungarian,        // Hungarian (Magyarország)
    Romanian,         // Romanian (România)
    Greek,            // Greek (Ελλάδα)
    Bulgarian,        // Bulgarian (България)
    Ukrainian,        // Ukrainian (Україна)
    Serbian,          // Serbian (Србија)
    Croatian,         // Croatian (Hrvatska)
    Slovak,           // Slovak (Slovensko)
    Slovenian,        // Slovenian (Slovenija)
    Estonian,         // Estonian (Eesti)
    Latvian,          // Latvian (Latvija)
    Lithuanian,       // Lithuanian (Lietuva)
    Danish,           // Danish (Danmark)
    Icelandic,        // Icelandic (Ísland)
    Albanian,         // Albanian (Shqipëri)
    Macedonian,       // Macedonian (Македонија)
    Georgian,         // Georgian (საქართველო)
    Armenian,         // Armenian (Հայաստան)
    Azerbaijani,      // Azerbaijani (Azərbaycan)
    Kazakh,           // Kazakh (Қазақстан)
    Uzbek,            // Uzbek (Oʻzbekiston)
    Mongolian,        // Mongolian (Монгол)
    Nepali,           // Nepali (नेपाल)
    Bengali,          // Bengali (বাংলা)
    Punjabi,          // Punjabi (ਪੰਜਾਬੀ)
    Tamil,            // Tamil (தமிழ்)
    Telugu,           // Telugu (తెలుగు)
    Kannada,          // Kannada (ಕನ್ನಡ)
    Malayalam,        // Malayalam (മലയാളം)
    Sinhala,          // Sinhala (සිංහල)
    Khmer,            // Khmer (ភាសាខ្មែរ)
    Lao,              // Lao (ລາວ)
    Burmese,          // Burmese (မြန်မာ)
    Amharic,          // Amharic (አማርኛ)
    Swahili,          // Swahili (Kiswahili)
    Hausa,            // Hausa (هَوُسَ)
    Yoruba,           // Yoruba (Yorùbá)
    Zulu,             // Zulu (isiZulu)
    Xhosa,            // Xhosa (isiXhosa)
    Afrikaans,        // Afrikaans (Afrikaans)
    Filipino,         // Filipino (Filipino)
    Malaysian,        // Malaysian (Bahasa Melayu)
    Indonesian,       // Indonesian (Bahasa Indonesia)
    Brunei,           // Brunei (Bahasa Brunei)
    Singapore,        // Singapore (English/Chinese/Malay/Tamil)
    HongKong,         // Hong Kong (中文/English)
    Taiwan,           // Taiwan (中文/English)
    Macau,            // Macau (中文/English)
    Default = English
};

/**
 * @brief Writing direction enumeration
 */
enum class WritingDirection {
    LeftToRight,      // LTR languages (English, Spanish, etc.)
    RightToLeft,      // RTL languages (Arabic, Hebrew, etc.)
    TopToBottom,      // TT languages (Japanese, Chinese, etc.)
    BottomToTop       // BT languages (Mongolian, etc.)
};

/**
 * @brief Text shaping information
 */
struct TextShapingInfo {
    bool isComplexScript;     // True for complex scripts (Arabic, Indic, etc.)
    bool requiresShaping;      // True for scripts that require shaping
    bool hasLigatures;        // True for scripts with ligatures
    bool hasDiacritics;      // True for scripts with diacritics
    int baseDirection;       // UBIDI_DIRECTION_LTR, UBIDI_DIRECTION_RTL, etc.
    std::vector<int> glyphIndices; // Glyph indices for shaped text
    std::vector<float> glyphAdvances; // Glyph advances
};

//==============================================================================
// Translation System
//==============================================================================

/**
 * @brief Translation data structure
 */
struct TranslationData {
    juce::String source;      // Source text (English)
    juce::String translation; // Translated text
    juce::String context;     // Context/usage information
    juce::String pluralForm;  // Plural form rule
    bool isGendered;         // True if gender-specific translation needed
    juce::String gender;      // Gender specification
    juce::String comment;     // Translator comments
};

/**
 * @brief Translation memory (TM) entry
 */
struct TranslationMemory {
    juce::String sourceText;
    juce::String translatedText;
    juce::String language;
    juce::String context;
    juce::uint64 timestamp;
    float confidence;         // Match confidence (0-1)
    bool isAutoTranslated;    // True if machine translation
};

//==============================================================================
// Internationalization Configuration
//==============================================================================

/**
 * @brief Internationalization configuration
 */
struct I18nConfig {
    Language defaultLanguage = Language::English;
    Language currentLanguage = Language::English;
    juce::String locale;      // Locale string (e.g., "en-US", "es-ES")
    juce::String countryCode; // Country code
    juce::String languageCode; // Language code

    // Text direction
    bool autoDetectDirection = true;
    WritingDirection forceDirection = WritingDirection::LeftToRight;

    // Font settings
    juce::String defaultFontFamily;
    float fontSizeMultiplier = 1.0f;
    bool useSystemFonts = true;

    // Text shaping
    bool enableComplexTextShaping = true;
    bool enableLigatures = true;
    bool enableDiacriticSupport = true;

    // Translation settings
    bool enableTranslationMemory = true;
    bool enableAutoTranslation = false;
    bool enableFallbackToEnglish = true;

    // RTL support
    bool enableRTLSupport = true;
    float rtlMargin = 8.0f; // Margin for RTL text

    // Performance settings
    bool cacheTranslations = true;
    int maxCacheSize = 10000;

    // Debug settings
    bool enableDebugMode = false;
    bool logTranslationIssues = true;
};

//==============================================================================
// Internationalization System Class
//==============================================================================

/**
 * @brief Complete internationalization system with bidirectional support
 */
class InternationalizationSystem : public juce::Component,
                                 public juce::Timer {
public:
    //==========================================================================
    // Construction
    //==========================================================================

    InternationalizationSystem();
    ~InternationalizationSystem() override;

    //==========================================================================
    // Language Management
    //==========================================================================

    /**
     * @brief Set current language
     * @param language Language to set
     */
    void setLanguage(Language language);

    /**
     * @brief Get current language
     */
    Language getCurrentLanguage() const { return config_.currentLanguage; }

    /**
     * @brief Get language display name
     * @param language Language
     */
    juce::String getLanguageDisplayName(Language language) const;

    /**
     * @brief Get language direction
     * @param language Language
     */
    WritingDirection getLanguageDirection(Language language) const;

    /**
     * @brief Get language code
     * @param language Language
     */
    juce::String getLanguageCode(Language language) const;

    /**
     * @brief Get all supported languages
     */
    std::vector<Language> getSupportedLanguages() const;

    //==========================================================================
    // Translation Management
    //==========================================================================

    /**
     * @brief Load translations for a language
     * @param language Language to load
     * @param filePath Path to translation file
     * @return True if successful
     */
    bool loadTranslations(Language language, const juce::String& filePath);

    /**
     * @brief Add translation manually
     * @param translation Translation data
     */
    void addTranslation(const TranslationData& translation);

    /**
     * @brief Translate text
     * @param text Text to translate
     * @param context Context/usage
     * @param gender Gender (for gender-specific translations)
     * @return Translated text
     */
    juce::String translate(const juce::String& text,
                           const juce::String& context = "",
                           const juce::String& gender = "");

    /**
     * @brief Translate with plural handling
     * @param text Text to translate
     * @param count Count for plural determination
     * @param context Context/usage
     * @return Translated text with proper plural form
     */
    juce::String translatePlural(const juce::String& text,
                                int count,
                                const juce::String& context = "");

    /**
     * @ Get translation confidence
     * @param sourceText Source text
     * @param translatedText Translated text
     * @return Confidence score (0-1)
     */
    float getTranslationConfidence(const juce::String& sourceText,
                                  const juce::String& translatedText) const;

    //==========================================================================
    // Text Processing
    //==========================================================================

    /**
     * @brief Shape text for complex scripts
     * @param text Text to shape
     * @param language Language of text
     * @return Text shaping information
     */
    TextShapingInfo shapeText(const juce::String& text, Language language) const;

    /**
     * @ Process bidirectional text
     * @param text Text to process
     * @param language Language of text
     * @return Processed text with proper direction
     */
    juce::String processBidirectionalText(const juce::String& text,
                                        Language language) const;

    /**
     * @ Adjust text position based on direction
     * @param x X position
     * @param y Y position
     * @param width Text width
     * @param direction Text direction
     * @return Adjusted position
     */
    juce::Point<float> adjustTextPosition(float x, float y, float width,
                                        WritingDirection direction) const;

    /**
     * @ Get proper text alignment for language
     * @param language Language
     * @return Justification for text
     */
    juce::Justification getTextAlignment(Language language) const;

    /**
     * @ Get font for language
     * @param language Language
     * @return Font for the language
     */
    juce::Font getFontForLanguage(Language language) const;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set internationalization configuration
     * @param config Configuration parameters
     */
    void setConfig(const I18nConfig& config);

    /**
     * @brief Get current configuration
     */
    const I18nConfig& getConfig() const { return config_; }

    /**
     * @brief Save configuration to file
     * @param filePath Path to save configuration
     * @return True if successful
     */
    bool saveConfig(const juce::String& filePath);

    /**
     * @brief Load configuration from file
     * @param filePath Path to load configuration
     * @return True if successful
     */
    bool loadConfig(const juce::String& filePath);

    //==========================================================================
    // Translation Memory
    //==========================================================================

    /**
     * @brief Add entry to translation memory
     * @param sourceText Source text
     * @param translatedText Translated text
     * @param context Context
     */
    void addToTranslationMemory(const juce::String& sourceText,
                               const juce::String& translatedText,
                               const juce::String& context = "");

    /**
     * @ Search translation memory
     * @param sourceText Source text to search for
     * @param context Context to search within
     * @return Best matching translation memory entry
     */
    TranslationMemory searchTranslationMemory(const juce::String& sourceText,
                                            const juce::String& context = "") const;

    /**
     * @ Clear translation memory
     */
    void clearTranslationMemory();

    //==========================================================================
    // Callbacks
    //==========================================================================

    /**
     * @brief Callback for language change
     */
    std::function<void(Language)> onLanguageChanged;

    /**
     * @brief Callback for translation loaded
     */
    std::function<void(Language, int)> onTranslationsLoaded;

    /**
     * @brief Callback for translation missing
     */
    std::function<void(juce::String, juce::String)> onTranslationMissing;

    //==========================================================================
    // Debug Methods
    //==========================================================================

    /**
     * @brief Enable/disable debug mode
     * @param enabled True to enable debug mode
     */
    void setDebugMode(bool enabled);

    /**
     * @brief Get translation statistics
     */
    juce::String getTranslationStatistics() const;

    /**
     * @brief Get unsupported text warnings
     */
    std::vector<juce::String> getUnsupportedTextWarnings() const;

    //==========================================================================
    // Timer Callback
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // File Methods
    //==========================================================================

    /**
     * @ Export translations to file
     * @param filePath Path to export to
     * @return True if successful
     */
    bool exportTranslations(const juce::String& filePath);

    /**
     * @ Import translations from file
     * @param filePath Path to import from
     * @return True if successful
     */
    bool importTranslations(const juce::String& filePath);

private:
    //==========================================================================
    // Language Management
    //==========================================================================

    /**
     * @ Initialize language data
     */
    void initializeLanguages();

    /**
     * @ Initialize language direction mapping
     */
    void initializeLanguageDirections();

    /**
     * @ Initialize font mappings
     */
    void initializeFontMappings();

    //==========================================================================
    // Translation Management
    //==========================================================================

    /**
     * @ Load translation from JSON file
     * @param filePath Path to JSON file
     * @return True if successful
     */
    bool loadTranslationsFromJSON(const juce::String& filePath);

    /**
     * @ Load translation from CSV file
     * @param filePath Path to CSV file
     * @return True if successful
     */
    bool loadTranslationsFromCSV(const juce::String& filePath);

    /**
     * @ Create translation hash for caching
     * @param text Source text
     * @param context Context
     * @param gender Gender
     * @return Hash key
     */
    juce::String createTranslationHash(const juce::String& text,
                                     const juce::String& context,
                                     const juce::String& gender) const;

    /**
     * @ Get translation from cache
     * @param hash Translation hash
     * @return Translated text or empty string
     */
    juce::String getTranslationFromCache(const juce::String& hash) const;

    /**
     * @ Add translation to cache
     * @param hash Translation hash
     * @param translation Translated text
     */
    void addTranslationToCache(const juce::String& hash,
                               const juce::String& translation);

    /**
     * @ Check if text needs shaping
     * @param text Text to check
     * @param language Language
     * @return True if shaping needed
     */
    bool needsTextShaping(const juce::String& text, Language language) const;

    /**
     * @ Shape complex text using ICU
     * @param text Text to shape
     * @param language Language
     * @return Shaping information
     */
    TextShapingInfo shapeComplexText(const juce::String& text,
                                    Language language) const;

    /**
     * @ Apply Unicode bidirectional algorithm
     * @param text Text to process
     * @param direction Text direction
     * @return Processed text
     */
    juce::String applyBidiAlgorithm(const juce::String& text,
                                   WritingDirection direction) const;

    /**
     * @ Get plural form for language
     * @param count Count
     * @param language Language
     * @return Plural form index (0, 1, 2, etc.)
     */
    int getPluralForm(int count, Language language) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Configuration
    I18nConfig config_;

    // Language data
    std::map<Language, juce::String> languageNames_;
    std::map<Language, WritingDirection> languageDirections_;
    std::map<Language, juce::String> languageCodes_;
    std::map<Language, juce::String> fontMappings_;

    // Translation data
    std::map<juce::String, TranslationData> translations_;
    std::map<juce::String, juce::String> translationCache_;
    std::vector<TranslationMemory> translationMemory_;

    // Unicode support
    mutable std::unique_ptr<UChar[]> unicodeBuffer_;
    mutable std::unique_ptr<UChar[]> bidiBuffer_;

    // Debug
    bool debugMode_;
    std::vector<juce::String> unsupportedTextWarnings_;
    juce::uint64 lastTranslationLoadTime_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternationalizationSystem)
};

} // namespace zenith::ui