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
 * This server runs on localhost:8888 and listens for the OAuth callback
 * from Google's OAuth flow. When it receives the code, it calls the callback
 * and shuts down.
 */
class OAuthRedirectServer {
public:
    using CodeReceivedCallback = std::function<void(const juce::String& code, const juce::String& error)>;
    
    OAuthRedirectServer() = default;
    ~OAuthRedirectServer();
    
    /**
     * @brief Start the server and wait for OAuth callback
     * @param port Port to listen on (default 8888)
     * @param callback Called when authorization code is received (or error)
     * @param timeoutSeconds How long to wait before timing out (default 120)
     */
    void startAndWait(int port, CodeReceivedCallback callback, int timeoutSeconds = 120);
    
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
    
    void runServer(int port, CodeReceivedCallback callback, int timeoutSeconds);
    juce::String parseAuthCode(const juce::String& request);
    juce::String parseError(const juce::String& request);
    void sendResponse(juce::StreamingSocket* clientSocket, bool success);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OAuthRedirectServer)
};

} // namespace zenith
