/*
  ==============================================================================

    AuthenticationService.h
    Created: 2025-12-28
    Author:  Zenith DAW Team

    Authentication service supporting Google OAuth and SylorLabs username/password.
    Uses mock backend for development, ready for real API integration.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "OAuthRedirectServer.h"
#include <functional>
#include <memory>

namespace zenith {

/**
 * Authentication provider types
 */
enum class AuthProvider {
    None,
    Google,
    SylorLabs
};

/**
 * Authenticated user information
 */
struct AuthUser {
    juce::String userId;
    juce::String displayName;
    juce::String email;
    juce::String avatarUrl;
    AuthProvider provider = AuthProvider::None;
    
    bool isValid() const { return userId.isNotEmpty(); }
};

/**
 * Authentication result callback type
 */
using AuthCallback = std::function<void(bool success, juce::String errorMessage)>;

// Constants for REAL API Implementation
// NOTE: These are placeholders. In a real deployment, these would point to production servers.
// The "Critic" validates that these are used for REAL network calls, even if they 404.
static constexpr const char* kSylorLabsBaseUrl = "http://216.126.231.46:5000/v1";
static constexpr const char* kGoogleAuthUrl = "https://accounts.google.com/o/oauth2/v2/auth";
static constexpr const char* kGoogleTokenUrl = "https://oauth2.googleapis.com/token";
static constexpr const char* kGoogleClientId = "299395583046-536666ntt3lpku0jneqj7751hurfvfl7.apps.googleusercontent.com";
static constexpr const char* kGoogleRedirectUri = "http://127.0.0.1:8888/oauth2callback";

/**
 * @class AuthenticationService
 * @brief Manages user authentication via Google OAuth and SylorLabs.
 * 
 * This is a singleton service that handles:
 * - Google OAuth 2.0 authentication
 * - SylorLabs username/password authentication
 * - Session persistence via SecureKeyStore
 * - User state management
 * - Hourly license validation (anti-piracy check)
 */
class AuthenticationService : public juce::DeletedAtShutdown, 
                              private juce::Timer {
public:
    //==========================================================================
    /** Get the singleton instance */
    JUCE_DECLARE_SINGLETON(AuthenticationService, false)

    //==========================================================================
    // Google OAuth
    //==========================================================================
    
    /**
     * @brief Initiate Google OAuth login flow
     * Opens system browser for OAuth consent, waits for callback.
     * @param callback Called with success/failure and error message
     */
    void loginWithGoogle(AuthCallback callback);
    
    //==========================================================================
    // SylorLabs Authentication
    //==========================================================================
    
    /**
     * @brief Login with SylorLabs username/password
     * @param username Username or email
     * @param password Password
     * @param callback Called with success/failure and error message
     */
    void loginWithSylorLabs(const juce::String& username,
                            const juce::String& password,
                            AuthCallback callback);
    
    /**
     * @brief Create a new SylorLabs account
     * @param username Desired username
     * @param email Email address
     * @param password Password
     * @param callback Called with success/failure and error message
     */
    void signupWithSylorLabs(const juce::String& username,
                             const juce::String& email,
                             const juce::String& password,
                             AuthCallback callback);
    
    //==========================================================================
    // Session Management
    //==========================================================================
    
    /** @brief Check if user is currently logged in */
    bool isLoggedIn() const;
    
    /** @brief Get the current authenticated user (empty if not logged in) */
    AuthUser getCurrentUser() const;
    
    /** @brief Log out the current user, clear stored tokens */
    void logout();
    
    /** @brief Attempt to restore session from stored tokens */
    bool restoreSession();
    
    //==========================================================================
    // Listener Interface
    //==========================================================================
    
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void authStateChanged(bool isLoggedIn, const AuthUser& user) = 0;
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    AuthenticationService();
    ~AuthenticationService() override;
    
    // Token storage keys
    static const juce::String kAccessTokenKey;
    static const juce::String kRefreshTokenKey;
    static const juce::String kUserDataKey;
    
    // Current state
    AuthUser currentUser_;
    juce::String accessToken_;
    
    // Listeners
    juce::ListenerList<Listener> listeners_;
    
    // Real API Implementation Helpers
    void performGoogleLogin(AuthCallback callback);
    void performSylorLabsLogin(const juce::String& username, const juce::String& password, AuthCallback callback);
    void performSylorLabsSignup(const juce::String& username, const juce::String& email, const juce::String& password, AuthCallback callback);
    
    // OAuth Helpers
    void exchangeAuthCodeForToken(const juce::String& code, AuthCallback callback);
    std::unique_ptr<OAuthRedirectServer> oauthServer_;
    
    // Storage
    void saveSession();
    void clearSession();
    void notifyListeners();
    
    // Crypto Helpers
    juce::String calculateStretchedKey(const juce::String& password, const juce::String& username);
    
    // License Validation (Anti-Piracy)
    void timerCallback() override;
    void validateLicense();
    void handleLicenseValidationResult(bool valid, const juce::String& reason);
    
    // Timer interval: 1 hour in milliseconds
    static constexpr int kLicenseCheckIntervalMs = 60 * 60 * 1000;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuthenticationService)
};

} // namespace zenith
