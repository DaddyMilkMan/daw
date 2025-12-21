/*
  ==============================================================================

    IdentityManager.h
    Created: 2025-12-20
    Author:  Zenith DAW Team

    Central authority for user identity, authentication, and licensing.
    Handles JWT storage, hardware fingerprinting, and offline verification.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "SecureKeyStore.h"
#include <atomic>
#include <memory>

namespace zenith {

/**
 * @brief Manages the user's authentication and license state.
 */
class IdentityManager : public juce::ChangeBroadcaster {
public:
  struct UserProfile {
    juce::String username;
    juce::String email;
    juce::String displayName;
    bool hasPremiumAccess = false;
  };

  /** @brief Get the singleton instance */
  static IdentityManager& getInstance();

  /** 
   * @brief Attempt to log in with credentials
   * @param usernameOrEmail User's identifier
   * @param password Plaintext password (sent over HTTPS)
   */
  void login(const juce::String& usernameOrEmail, const juce::String& password);

  /** @brief Log out and clear all secure tokens */
  void logout();

  /** @brief Check if currently authenticated with a valid session */
  bool isLoggedIn() const { return isLoggedIn_; }

  /** @brief Get the summary of the current user */
  const UserProfile& getUserProfile() const { return currentUser_; }

  /** @brief Generate a unique hardware ID for this machine */
  static juce::String getMachineID();

  /** 
   * @brief Perform a background license check (heartbeat)
   * This updates the hasPremiumAccess flag.
   */
  void verifyLicenseAsync();

private:
  IdentityManager();
  ~IdentityManager();

  void loadSavedSession();
  void handleAuthFailure();
  void handleAuthSuccess(const juce::var& jsonResponse);

  std::atomic<bool> isLoggedIn_{false};
  UserProfile currentUser_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IdentityManager)
};

} // namespace zenith
