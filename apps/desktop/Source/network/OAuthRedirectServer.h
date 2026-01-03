/*
  ==============================================================================

    OAuthRedirectServer.h
    Created: 2025-12-28
    Author:  Zenith DAW Team

    Local HTTP server to catch OAuth redirect callbacks.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <thread>
#include <atomic>

namespace zenith {

/**
 * @class OAuthRedirectServer
 * @brief Simple HTTP server to catch OAuth redirect with authorization code.
 * 
 * This server runs on a dynamically allocated localhost port (for security)
 * and listens for the OAuth callback from the OAuth provider.
 * When it receives the code, it validates the state parameter and calls the callback.
 * 
 * SECURITY NOTES:
 * - Uses ephemeral ports (randomly selected) instead of hardcoded port 8888
 * - Validates state parameter to prevent CSRF attacks
 * - The backend at sylorlabs.com MUST validate that redirect_uri matches the registered URI
 * - The backend MUST also validate the state parameter matches what was sent in the auth request
 */
class OAuthRedirectServer {
public:
    using CodeReceivedCallback = std::function<void(const juce::String& code, const juce::String& token, const juce::String& error, const juce::String& state)>;
    
    OAuthRedirectServer() = default;
    ~OAuthRedirectServer();
    
    /**
     * @brief Find and allocate a free ephemeral port
     * @return Port number if successful, -1 if no port available
     * 
     * Attempts to bind to a random port in the ephemeral range (49152-65535).
     * This is more secure than using a hardcoded port as it reduces the attack surface.
     */
    static int findFreePort();
    
    /**
     * @brief Start the server and wait for OAuth callback
     * @param port Port to listen on (use findFreePort() to get a dynamic port)
     * @param expectedState The expected state parameter value (for CSRF protection)
     * @param callback Called when authorization code is received (or error)
     * @param timeoutSeconds How long to wait before timing out (default 120)
     */
    void startAndWait(int port, const juce::String& expectedState, CodeReceivedCallback callback, int timeoutSeconds = 120);
    
    /**
     * @brief Stop the server if running
     */
    void stop();
    
    /**
     * @brief Check if server is currently running
     */
    bool isRunning() const { return running_.load(); }

private:
    std::atomic<bool> running_{false};
    std::atomic<bool> shouldStop_{false};
    std::unique_ptr<std::thread> serverThread_;
    
    std::unique_ptr<juce::StreamingSocket> serverSocket_;
    juce::String expectedState_;
    
    void runServer(int port, const juce::String& expectedState, CodeReceivedCallback callback, int timeoutSeconds);
    void sendStaticResponse(juce::StreamingSocket* clientSocket, bool success);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OAuthRedirectServer)
};

} // namespace zenith
