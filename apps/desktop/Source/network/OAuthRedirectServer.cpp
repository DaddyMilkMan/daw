/*
  ==============================================================================

    OAuthRedirectServer.cpp
    Created: 2025-12-28
    Author:  Zenith DAW Team

    Implementation of local HTTP server for OAuth redirect callbacks.

  ==============================================================================
*/

#include "OAuthRedirectServer.h"
#include <juce_events/juce_events.h>
#include "../engine/ZenithLogger.h"

namespace zenith {

OAuthRedirectServer::~OAuthRedirectServer() {
    stop();
}

int OAuthRedirectServer::findFreePort() {
    // Try to find a free ephemeral port in the range 49152-65535
    // This is more secure than using a hardcoded port
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(49152, 65535);
    
    // Try up to 10 random ports
    for (int attempt = 0; attempt < 10; ++attempt) {
        int port = dis(gen);
        
        // Try to bind to this port temporarily to check if it's available
        juce::StreamingSocket testSocket;
        if (testSocket.createListener(port, "127.0.0.1")) {
            testSocket.close();
            ZENITH_LOG_INFO("[OAuth] Found free port: " + juce::String(port));
            return port;
        }
    }
    
    ZENITH_LOG_ERROR("[OAuth] Failed to find a free ephemeral port after 10 attempts");
    return -1;
}

void OAuthRedirectServer::startAndWait(int port, const juce::String& expectedState, CodeReceivedCallback callback, int timeoutSeconds) {
    if (running_.load()) {
        ZENITH_LOG_WARNING("[OAuth] Server already running");
        return;
    }
    
    expectedState_ = expectedState;
    shouldStop_.store(false);
    running_.store(true);
    
    // Run server on background thread
    serverThread_ = std::make_unique<std::thread>([this, port, expectedState, callback, timeoutSeconds]() {
        runServer(port, expectedState, callback, timeoutSeconds);
    });
}

void OAuthRedirectServer::stop() {
    shouldStop_.store(true);
    
    if (serverSocket_ != nullptr) {
        serverSocket_->close();
    }
    
    if (serverThread_ && serverThread_->joinable()) {
        serverThread_->join();
    }
    serverThread_.reset();
    running_.store(false);
}

void OAuthRedirectServer::runServer(int port, const juce::String& expectedState, CodeReceivedCallback callback, int timeoutSeconds) {
    ZENITH_LOG_INFO("[OAuth] Starting redirect server on port " + juce::String(port));
    
    serverSocket_ = std::make_unique<juce::StreamingSocket>();
    
    if (!serverSocket_->createListener(port, "127.0.0.1")) {
        ZENITH_LOG_ERROR("[OAuth] Failed to bind to port " + juce::String(port));
        callback("", "", "Failed to bind to port " + juce::String(port) + ". Port may already be in use.", "");
        running_.store(false);
        return;
    }
    
    ZENITH_LOG_INFO("[OAuth] Waiting for OAuth callback on port " + juce::String(port) + "...");
    
    // Wait for connection with timeout
    auto startTime = juce::Time::getMillisecondCounter();
    std::unique_ptr<juce::StreamingSocket> clientSocket;
    
    while (!shouldStop_.load() && (juce::Time::getMillisecondCounter() - startTime) < (uint32_t)timeoutSeconds * 1000) {
        if (serverSocket_->waitUntilReady(true, 500) == 1) {
            clientSocket.reset(serverSocket_->waitForNextConnection());
            if (clientSocket != nullptr) break;
        }
    }
    
    if (clientSocket == nullptr) {
        if (!shouldStop_.load()) {
            ZENITH_LOG_WARNING("[OAuth] Accept timed out or failed");
            callback("", "", "Timeout waiting for OAuth callback. Please try again.", "");
        }
        running_.store(false);
        return;
    }
    
    // Read request
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    int bytesRead = clientSocket->read(buffer, sizeof(buffer) - 1, false);
    
    if (bytesRead > 0) {
        juce::String request(buffer, (size_t)bytesRead);
        ZENITH_LOG_INFO("[OAuth] Received callback request");
        
        // Extract the path and query part from the HTTP request
        // Request usually starts with "GET /oauth2callback?code=...&state=... HTTP/1.1"
        int firstSpace = request.indexOf(" ");
        int secondSpace = request.indexOf(firstSpace + 1, " ");
        
        if (firstSpace != -1 && secondSpace != -1) {
            juce::String urlPath = request.substring(firstSpace + 1, secondSpace);
            juce::URL url("http://localhost" + urlPath);
            
            juce::String code;
            juce::String token;
            juce::String error;
            juce::String state;
            
            auto paramNames = url.getParameterNames();
            auto paramValues = url.getParameterValues();
            
            for (int i = 0; i < paramNames.size(); ++i) {
                if (paramNames[i] == "code") code = paramValues[i];
                else if (paramNames[i] == "token") token = paramValues[i];
                else if (paramNames[i] == "error") error = paramValues[i];
                else if (paramNames[i] == "state") state = paramValues[i];
            }
            
            // SECURITY: Verify state parameter to prevent CSRF attacks
            // The state parameter MUST match the one we generated before starting OAuth flow
            // The backend at sylorlabs.com MUST also verify this on their side
            if (state != expectedState) {
                ZENITH_LOG_ERROR("[OAuth] State mismatch! Expected: '" + expectedState + "', got: '" + state + "'");
                sendStaticResponse(clientSocket.get(), false);
                
                juce::MessageManager::callAsync([callback]() {
                    callback("", "", "Security validation failed: state parameter mismatch. This could indicate a CSRF attack. Please try again.", "");
                });
                
                clientSocket->close();
                serverSocket_->close();
                running_.store(false);
                return;
            }
            
            if (code.isNotEmpty() || token.isNotEmpty()) {
                ZENITH_LOG_INFO("[OAuth] Authorization received and validated!");
                sendStaticResponse(clientSocket.get(), true);
                
                juce::MessageManager::callAsync([callback, code, token, state]() {
                    callback(code, token, "", state);
                });
            } else if (error.isNotEmpty()) {
                ZENITH_LOG_ERROR("[OAuth] Error received: " + error);
                sendStaticResponse(clientSocket.get(), false);
                
                juce::MessageManager::callAsync([callback, error]() {
                    callback("", "", error, "");
                });
            } else {
                ZENITH_LOG_ERROR("[OAuth] No code or error in callback");
                sendStaticResponse(clientSocket.get(), false);
                
                juce::MessageManager::callAsync([callback]() {
                    callback("", "", "Invalid OAuth callback response", "");
                });
            }
                
                juce::MessageManager::callAsync([callback]() {
                    callback("", "", "Invalid OAuth callback response", "");
                });
            }
        } else {
            ZENITH_LOG_ERROR("[OAuth] Malformed HTTP request");
            sendStaticResponse(clientSocket.get(), false);
            juce::MessageManager::callAsync([callback]() {
                callback("", "", "Malformed HTTP request", "");
            });
        }
    } else {
        ZENITH_LOG_ERROR("[OAuth] Failed to read from client socket");
        juce::MessageManager::callAsync([callback]() {
            callback("", "", "Failed to read OAuth callback", "");
        });
    }
    
    clientSocket->close();
    serverSocket_->close();
    
    running_.store(false);
    ZENITH_LOG_INFO("[OAuth] Redirect server stopped");
}

void OAuthRedirectServer::sendStaticResponse(juce::StreamingSocket* clientSocket, bool success) {
    juce::String html;
    if (success) {
        html = R"(<!DOCTYPE html>
<html>
<head><title>Zenith DAW - Login Successful</title>
<style>
body { font-family: system-ui, sans-serif; background: linear-gradient(135deg, #0a0a0f 0%, #1a1a2e 100%); 
       color: #fff; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
.card { background: rgba(255,255,255,0.05); border-radius: 16px; padding: 48px; text-align: center; 
        border: 1px solid rgba(0,200,255,0.3); box-shadow: 0 0 40px rgba(0,200,255,0.2); }
h1 { color: #0cf; margin-bottom: 16px; }
p { color: #aaa; }
</style></head>
<body><div class="card">
<h1>✓ Login Successful!</h1>
<p>You can close this window and return to Zenith DAW.</p>
</div></body></html>)";
    } else {
        html = R"(<!DOCTYPE html>
<html>
<head><title>Zenith DAW - Login Failed</title>
<style>
body { font-family: system-ui, sans-serif; background: #0a0a0f; color: #fff; 
       display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
.card { background: rgba(255,0,0,0.1); border-radius: 16px; padding: 48px; text-align: center;
        border: 1px solid rgba(255,0,0,0.3); }
h1 { color: #f44; margin-bottom: 16px; }
</style></head>
<body><div class="card">
<h1>✗ Login Failed</h1>
<p>Please try again in Zenith DAW.</p>
</div></body></html>)";
    }
    
    juce::String response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html; charset=utf-8\r\n"
                           "Content-Length: " + juce::String(html.length()) + "\r\n"
                           "Connection: close\r\n"
                           "\r\n" + html;
    
    clientSocket->write(response.toRawUTF8(), (int)response.length());
}

} // namespace zenith
