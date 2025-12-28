/*
  ==============================================================================
    GrokAPIClient.cpp
    Implementation of Phase 2 integrated Grok API client with secure storage
  =============================================================================
*/

#include "GrokAPIClient.h"
#include "PredictiveMastering.h"
#include "RealTimeCollaboration.h"
#include "AudioThreadSafeProcessor.h"
#include "RealNeuralNetwork.h"
#include "AutonomousDecisionEngine.h"
#include <juce_cryptography/juce_cryptography.h>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <wincred.h>
#pragma comment(lib, "credu32.lib")
#elif defined(__APPLE__)
#include <Security/Security.h>
#endif

// Use SecureKeyStore abstraction for cross-platform secure storage
#include "../network/SecureKeyStore.h"

namespace zenith {
namespace ai {

// Background analysis worker implementation
void GrokAPIClient::analysisWorker() {
    while (!shouldStopAnalysis.load()) {
        std::unique_lock<std::mutex> lock(analysisQueueMutex);
        
        // Wait for work or stop signal
        analysisCondition.wait(lock, [this] {
            return !analysisQueue.empty() || shouldStopAnalysis.load();
        });
        
        if (shouldStopAnalysis.load()) break;
        
        // Get next request
        auto request = analysisQueue.front();
        analysisQueue.pop();
        lock.unlock();
        
        // Perform analysis
        try {
            auto result = performAnalysis(request);
            
            // Cache result
            cacheAnalysis(generateCacheKey(request.trackName, *request.audio, request.sampleRate), result);
            
        } catch (const std::exception& e) {
            DBG("Analysis failed: " << e.what());
        }
    }
}

void GrokAPIClient::startBackgroundAnalysis() {
    shouldStopAnalysis.store(false);
    analysisThread = std::make_unique<std::thread>(&GrokAPIClient::analysisWorker, this);
}

void GrokAPIClient::stopBackgroundAnalysis() {
    shouldStopAnalysis.store(true);
    analysisCondition.notify_all();
    
    if (analysisThread && analysisThread->joinable()) {
        analysisThread->join();
    }
    
    analysisThread.reset();
}

void GrokAPIClient::analyzeAudioAsync(const AnalysisRequest& request,
                                     std::function<void(AnalysisResult)> onComplete,
                                     std::function<void(juce::String)> onError) {
    // Check cache first
    juce::String cacheKey = generateCacheKey(request.trackName, *request.audio, request.sampleRate);
    AnalysisResult cachedResult;
    
    if (!request.forceReanalysis && getCachedAnalysis(cacheKey, cachedResult)) {
        juce::MessageManager::callAsync([onComplete, cachedResult]() {
            onComplete(cachedResult);
        });
        return;
    }
    
    // Queue for background analysis
    {
        std::lock_guard<std::mutex> lock(analysisQueueMutex);
        analysisQueue.push(request);
    }
    
    analysisCondition.notify_one();
    
    // Launch a watcher thread that waits for the result without blocking the caller
    std::thread([this, cacheKey, onComplete, onError]() {
        auto startTime = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(30);
        
        while (std::chrono::steady_clock::now() - startTime < timeout) {
            AnalysisResult result;
            if (getCachedAnalysis(cacheKey, result)) {
                juce::MessageManager::callAsync([onComplete, result]() {
                    onComplete(result);
                });
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (onError) {
            juce::MessageManager::callAsync([onError]() {
                onError("Analysis timeout");
            });
        }
    }).detach();
}

GrokAPIClient::AnalysisResult GrokAPIClient::getAnalysis(const juce::String& trackName, 
                                        const juce::AudioBuffer<float>& audio,
                                        double sampleRate) {
    AnalysisRequest request;
    request.audio = std::make_shared<juce::AudioBuffer<float>>(audio);
    request.sampleRate = sampleRate;
    request.trackName = trackName;
    request.forceReanalysis = false;
    
    juce::String cacheKey = generateCacheKey(trackName, audio, sampleRate);
    AnalysisResult result;
    
    if (getCachedAnalysis(cacheKey, result)) {
        return result;
    }
    
    // Perform synchronous analysis
    return performAnalysis(request);
}

GrokAPIClient::AnalysisResult GrokAPIClient::performAnalysis(const AnalysisRequest& request) {
    AnalysisResult result;
    
    try {
        // Visual analysis
        result.visualAnalysis = visualAnalyzer->analyzeAudio(*request.audio, request.sampleRate);
        
        // Genre detection
        result.genrePrediction = genreDetector->detectGenre(*request.audio, request.sampleRate);
        
        // Project context (if available)
        if (projectContext) {
            auto insights = projectContext->getContextualInsights();
            
            // Creative analysis
            result.creativeInsight = creativePartner->analyzeCreatively(
                *request.audio, *projectContext, result.genrePrediction);
            
            // Generate contextual description
            result.contextualDescription = "Track: " + request.trackName + "\n";
            result.contextualDescription += "Genre: " + result.genrePrediction.genre;
            if (result.genrePrediction.confidence > 0.7f) {
                result.contextualDescription += " (" + result.genrePrediction.subgenre + ")";
            }
            result.contextualDescription += "\n" + result.visualAnalysis.visualDescription;
            result.contextualDescription += "\n" + result.creativeInsight.observation;
        }
        
        result.isComplete = true;
        
    } catch (const std::exception& e) {
        DBG("Analysis failed: " << e.what());
        result.isComplete = false;
    }
    
    return result;
}

juce::String GrokAPIClient::callGrokWithContext(const juce::String& prompt,
                                               const juce::String& trackName,
                                               const juce::AudioBuffer<float>& audio,
                                               double sampleRate,
                                               const juce::var& projectState,
                                               ModelType modelType) {
    // Update project context if provided
    if (!projectState.isVoid()) {
        updateProjectContext(projectState);
    }
    
    // Get analysis
    auto analysis = getAnalysis(trackName, audio, sampleRate);
    
    // Build contextual prompt
    auto contextualPrompt = buildContextualPrompt(prompt, analysis, getProjectInsights());
    
    // Call Grok with enhanced context
    return callGrok(contextualPrompt, buildSystemPrompt(), modelType);
}

std::vector<CreativeSuggestion> GrokAPIClient::getCreativeSuggestions(const juce::String& trackName) {
    // Get cached analysis for the track
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    for (const auto& [key, result] : analysisCache) {
        if (key.contains(trackName) && result.isComplete) {
            return creativePartner->generateSuggestions(result.creativeInsight);
        }
    }
    
    return {};
}

juce::String GrokAPIClient::askCreativePartner(const juce::String& question) {
    // Get current project insights
    auto insights = getProjectInsights();
    
    // Create a dummy insight for conversation
    CreativeInsight insight;
    insight.observation = "Based on current project analysis";
    insight.explanation = "I'm analyzing your project context";
    
    return creativePartner->respondToQuestion(question, insight);
}

void GrokAPIClient::provideCreativeFeedback(const CreativeSuggestion& suggestion, 
                                            bool wasHelpful, 
                                            const juce::String& comment) {
    creativePartner->updateFromUserFeedback(suggestion, wasHelpful, comment);
    
    // Update learning system
    if (wasHelpful) {
        // Positive feedback - reinforce this type of suggestion
        UserPreference pref;
        pref.genre = "";  // Would be filled from context
        pref.parameter = suggestion.type;
        pref.value = 1.0;  // Positive feedback
        pref.confidence = wasHelpful ? 1.0 : 0.0;
        pref.timestamp = juce::Time::getCurrentTime();
        
        journey->recordUserAction(pref);
    }
}

void GrokAPIClient::updateProjectContext(const juce::var& dawState) {
    if (projectContext) {
        projectContext->analyzeFromDAWState(dawState);
    }
}

ContextualInsights GrokAPIClient::getProjectInsights() const {
    if (projectContext) {
        return projectContext->getContextualInsights();
    }
    
    ContextualInsights empty;
    return empty;
}

void GrokAPIClient::cacheAnalysis(const juce::String& key, const AnalysisResult& result) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    // Clean cache if too large
    if (analysisCache.size() >= MAX_CACHE_SIZE) {
        cleanupCache();
    }
    
    analysisCache[key] = result;
}

bool GrokAPIClient::getCachedAnalysis(const juce::String& key, AnalysisResult& result) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = analysisCache.find(key);
    if (it != analysisCache.end()) {
        result = it->second;
        return true;
    }
    
    return false;
}

void GrokAPIClient::cleanupCache() {
    // Remove oldest entries (simplified LRU)
    if (analysisCache.size() > MAX_CACHE_SIZE / 2) {
        auto it = analysisCache.begin();
        std::advance(it, analysisCache.size() / 2);
        analysisCache.erase(analysisCache.begin(), it);
    }
}

juce::String GrokAPIClient::generateCacheKey(const juce::String& trackName, 
                                            const juce::AudioBuffer<float>& audio,
                                            double sampleRate) {
    // Create hash from track name, audio content hash, and sample rate
    juce::String key = trackName;
    
    // Robust audio hashing using SHA256
    juce::MemoryBlock audioData;
    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        audioData.append(audio.getReadPointer(ch), static_cast<size_t>(audio.getNumSamples()) * sizeof(float));
    }
    
    juce::SHA256 audioHasher(audioData.getData(), audioData.getSize());
    juce::String audioHash = audioHasher.toHexString();
    
    key += "_" + audioHash + "_" + juce::String(sampleRate, 0);
    return key;
}

void GrokAPIClient::loadAPIKeyFromSecureStorage() {
    apiKey = retrieveAPIKey();
    
    if (apiKey.isEmpty()) {
        // Try environment variable as fallback
        apiKey = juce::SystemStats::getEnvironmentVariable(juce::String("GROK_API_KEY"), juce::String());
        
        if (apiKey.isEmpty()) {
            logMessage("Warning: No API key found in secure storage or environment", LogLevel::Warning);
        }
    }
}

bool GrokAPIClient::setAPIKey(const juce::String& key) {
    if (key.isEmpty()) {
        logMessage("Error: API key cannot be empty", LogLevel::Error);
        return false;
    }
    
    // Validate key format (basic validation)
    if (!key.startsWith("xai-") || key.length() < 20) {
        logMessage("Error: Invalid API key format", LogLevel::Error);
        return false;
    }
    
    // Store securely
    if (storeAPIKey(key)) {
        apiKey = key;
        logMessage("API key stored securely", LogLevel::Info);
        return true;
    }
    
    logMessage("Error: Failed to store API key securely", LogLevel::Error);
    return false;
}

void GrokAPIClient::clearAPIKey() {
    if (deleteAPIKey()) {
        apiKey.clear();
        logMessage("API key removed from secure storage", LogLevel::Info);
    }
}

// Secure storage implementation
bool GrokAPIClient::storeAPIKey(const juce::String& key) {
#ifdef _WIN32
    // Windows Credential Manager
    std::wstring keyName = L"ZenithDAW_GrokAPI_Key";
    std::wstring keyValue = key.toWideCharPointer();
    
    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<wchar_t*>(keyName.c_str());
    cred.CredentialBlob = (LPBYTE)keyValue.c_str();
    cred.CredentialBlobSize = static_cast<DWORD>(keyValue.length() * sizeof(wchar_t));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    
    return CredWriteW(&cred, 0) == TRUE;
    
#elif defined(__APPLE__)
    // macOS Keychain
    CFStringRef keyName = CFStringCreateWithCString(nullptr, "ZenithDAW_GrokAPI_Key", kCFStringEncodingUTF8);
    CFStringRef keyValue = CFStringCreateWithCString(nullptr, key.toUTF8(), kCFStringEncodingUTF8);
    
    SecItemClass itemClass = kSecClassGenericPassword;
    SecAccessRef access = nullptr;
    SecKeychainItemRef item = nullptr;
    
    OSStatus status = SecKeychainItemCreateFromContent(
        itemClass,
        nullptr,
        CFDataGetBytePtr(CFStringCreateExternalRepresentation(nullptr, keyValue, kCFStringEncodingUTF8, 0)),
        keyName,
        nullptr,
        access,
        &item
    );
    
    CFRelease(keyName);
    CFRelease(keyValue);
    
    return status == errSecSuccess;
    
#elif defined(__linux__)
    // Use SecureKeyStore abstraction (handles libsecret + fallback)
    return zenith::SecureKeyStore::storeKey("grok_api_key", key);
#else
    // Fallback to encrypted file
    juce::File keyFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("ZenithDAW")
                          .getChildFile(".grok_key");
    
    // Simple XOR encryption (better than plain text)
    juce::String encrypted = encryptKey(key);
    return keyFile.replaceWithText(encrypted);
#endif
}

juce::String GrokAPIClient::retrieveAPIKey() {
#ifdef _WIN32
    // Windows Credential Manager
    std::wstring keyName = L"ZenithDAW_GrokAPI_Key";
    
    PCREDENTIALW cred = nullptr;
    if (CredReadW(keyName.c_str(), CRED_TYPE_GENERIC, 0, &cred) == TRUE) {
        std::wstring keyValue((wchar_t*)cred->CredentialBlob, cred->CredentialBlobSize / sizeof(wchar_t));
        CredFree(cred);
        return juce::String(keyValue);
    }
    
#elif defined(__APPLE__)
    // macOS Keychain
    CFStringRef keyName = CFStringCreateWithCString(nullptr, "ZenithDAW_GrokAPI_Key", kCFStringEncodingUTF8);
    
    CFDataRef data = nullptr;
    SecKeychainItemRef item = nullptr;
    
    OSStatus status = SecKeychainItemCopyContent(
        nullptr,
        kSecClassGenericPassword,
        nullptr,
        keyName,
        &data,
        nullptr,
        nullptr,
        &item
    );
    
    if (status == errSecSuccess && data) {
        CFStringRef keyValue = CFStringCreateFromExternalRepresentation(nullptr, data, kCFStringEncodingUTF8);
        if (keyValue) {
            char buffer[1024];
            CFStringGetCString(keyValue, buffer, sizeof(buffer), kCFStringEncodingUTF8);
            CFRelease(keyValue);
            CFRelease(data);
            CFRelease(keyName);
            return juce::String(buffer);
        }
    }
    
    if (data) CFRelease(data);
    if (item) CFRelease(item);
    CFRelease(keyName);
    
#elif defined(__linux__)
    // Use SecureKeyStore abstraction (handles libsecret + fallback)
    juce::String key;
    if (zenith::SecureKeyStore::retrieveKey("grok_api_key", key)) {
        return key;
    }
    return {};
#else
    // Fallback to encrypted file
    juce::File keyFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("ZenithDAW")
                          .getChildFile(".grok_key");
    
    if (keyFile.exists()) {
        juce::String encrypted = keyFile.loadFileAsString();
        return decryptKey(encrypted);
    }
#endif
    
    return {};
}

bool GrokAPIClient::deleteAPIKey() {
#ifdef _WIN32
    std::wstring keyName = L"ZenithDAW_GrokAPI_Key";
    return CredDeleteW(keyName.c_str(), CRED_TYPE_GENERIC, 0) == TRUE;
    
#elif defined(__APPLE__)
    CFStringRef keyName = CFStringCreateWithCString(nullptr, "ZenithDAW_GrokAPI_Key", kCFStringEncodingUTF8);
    
    OSStatus status = SecKeychainItemDelete(nullptr, keyName);
    CFRelease(keyName);
    
    return status == errSecSuccess;
    
#elif defined(__linux__)
    // Use SecureKeyStore abstraction (handles libsecret + fallback)
    return zenith::SecureKeyStore::deleteKey("grok_api_key");
#else
    juce::File keyFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("ZenithDAW")
                          .getChildFile(".grok_key");
    
    return keyFile.deleteFile();
#endif
}

juce::String GrokAPIClient::encryptKey(const juce::String& key) {
    // Generate machine-specific salt
    juce::String salt = juce::SystemStats::getComputerName() + 
                       juce::SystemStats::getUserId() + 
                       "Zenith_Secure_Salt";
    
    juce::String encrypted;
    for (int i = 0; i < key.length(); ++i) {
        char encryptedChar = key[i] ^ salt[i % salt.length()];
        encrypted += juce::String::formatted("%02X", static_cast<unsigned char>(encryptedChar));
    }
    
    return encrypted;
}

juce::String GrokAPIClient::decryptKey(const juce::String& encrypted) {
    juce::String salt = juce::SystemStats::getComputerName() + 
                       juce::SystemStats::getUserId() + 
                       "Zenith_Secure_Salt";
                       
    juce::String decrypted;
    
    for (int i = 0; i < encrypted.length(); i += 2) {
        juce::String hexByte = encrypted.substring(i, i + 2);
        char encryptedChar = static_cast<char>(std::strtol(hexByte.toUTF8(), nullptr, 16));
        char decryptedChar = encryptedChar ^ salt[(i / 2) % salt.length()];
        decrypted += decryptedChar;
    }
    
    return decrypted;
}

void GrokAPIClient::logMessage(const juce::String& message, LogLevel level) {
    juce::String levelStr;
    switch (level) {
        case LogLevel::Info: levelStr = "INFO"; break;
        case LogLevel::Warning: levelStr = "WARN"; break;
        case LogLevel::Error: levelStr = "ERROR"; break;
    }
    
    juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S");
    juce::String logEntry = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // Log to debug console
    DBG(logEntry);
    
    // Could also log to file if needed
}

} // namespace ai
} // namespace zenith
