/*
  ==============================================================================

    IdentityManager.cpp
    Created: 2025-12-20
    Author:  Zenith DAW Team

  ==============================================================================
*/

#include "IdentityManager.h"
#include "../engine/ZenithLogger.h"

namespace zenith {

IdentityManager::IdentityManager() {
  loadSavedSession();
}

IdentityManager::~IdentityManager() = default;

IdentityManager& IdentityManager::getInstance() {
  static IdentityManager instance;
  return instance;
}

void IdentityManager::login(const juce::String& usernameOrEmail, const juce::String& password) {
  ZENITH_LOG_INFO("IdentityManager: Attempting login for " + usernameOrEmail);
  
  // NOTE: In a real implementation, this would be a juce::URL post request
  // to https://api.zenithaudio.com/auth/login.
  // For now, we mock success for testing while the backend is initialized.
  
  juce::DynamicObject::Ptr mockResponse = new juce::DynamicObject();
  mockResponse->setProperty("status", "success");
  mockResponse->setProperty("token", "mock_jwt_token_12345");
  mockResponse->setProperty("user_name", usernameOrEmail);
  mockResponse->setProperty("display_name", "Zenith Power User");
  mockResponse->setProperty("premium", true);

  handleAuthSuccess(juce::var(mockResponse.get()));
}

void IdentityManager::logout() {
  ZENITH_LOG_INFO("IdentityManager: Logging out");
  SecureKeyStore::deleteKey(SecureKeyStore::ZenithAuthToken);
  isLoggedIn_ = false;
  currentUser_ = UserProfile();
  sendChangeMessage();
}

juce::String IdentityManager::getMachineID() {
  // Combine multiple hardware markers for a robust fingerprint
  auto cpu = juce::SystemStats::getCpuVendor();
  auto device = juce::SystemStats::getDeviceDescription();
  auto machine = juce::SystemStats::getComputerName();
  
  // Hash the combination to get a unique 64-character ID
  juce::String rawID = cpu + "|" + device + "|" + machine;
  return juce::String::toHexString(rawID.hashCode64());
}

void IdentityManager::verifyLicenseAsync() {
  // Background check of the stored token
  juce::String token;
  if (SecureKeyStore::retrieveKey(SecureKeyStore::ZenithAuthToken, token)) {
    ZENITH_LOG_INFO("IdentityManager: Verifying stored license token...");
    // Mock license verification
    currentUser_.hasPremiumAccess = true;
    sendChangeMessage();
  }
}

void IdentityManager::loadSavedSession() {
  juce::String token;
  if (SecureKeyStore::retrieveKey(SecureKeyStore::ZenithAuthToken, token)) {
    ZENITH_LOG_INFO("IdentityManager: Found saved session token");
    isLoggedIn_ = true;
    // We would typically verify the token here
    verifyLicenseAsync();
  }
}

void IdentityManager::handleAuthSuccess(const juce::var& jsonResponse) {
  isLoggedIn_ = true;
  
  auto token = jsonResponse["token"].toString();
  SecureKeyStore::storeKey(SecureKeyStore::ZenithAuthToken, token);
  
  currentUser_.username = jsonResponse["user_name"].toString();
  currentUser_.displayName = jsonResponse["display_name"].toString();
  currentUser_.hasPremiumAccess = (bool)jsonResponse["premium"];
  
  ZENITH_LOG_INFO("IdentityManager: Login successful for " + currentUser_.displayName);
  sendChangeMessage();
}

void IdentityManager::handleAuthFailure() {
  logout();
}

} // namespace zenith
