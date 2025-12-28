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

void OAuthRedirectServer::startAndWait(int port, CodeReceivedCallback callback, int timeoutSeconds) {
    if (running_.load()) {
        ZENITH_LOG_WARNING("[OAuth] Server already running");
        return;
    }
    
    shouldStop_.store(false);
    running_.store(true);
    
    // Run server on background thread
    serverThread_ = std::make_unique<std::thread>([this, port, callback, timeoutSeconds]() {
        runServer(port, callback, timeoutSeconds);
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

void OAuthRedirectServer::runServer(int port, CodeReceivedCallback callback, int timeoutSeconds) {
    ZENITH_LOG_INFO("[OAuth] Starting redirect server on port " + juce::String(port));
    
    serverSocket_ = std::make_unique<juce::StreamingSocket>();
    
    if (!serverSocket_->createListener(port, "127.0.0.1")) {
        ZENITH_LOG_ERROR("[OAuth] Failed to bind to port " + juce::String(port));
        callback("", "Failed to bind to port " + juce::String(port));
        running_.store(false);
        return;
    }
    
    ZENITH_LOG_INFO("[OAuth] Waiting for Google callback on port " + juce::String(port) + "...");
    
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
            callback("", "Timeout waiting for OAuth callback");
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
        
        // Parse authorization code or error
        juce::String code = parseAuthCode(request);
        juce::String error = parseError(request);
        
        if (code.isNotEmpty()) {
            ZENITH_LOG_INFO("[OAuth] Authorization code received!");
            sendResponse(clientSocket.get(), true);
            
            // Post callback to message thread
            juce::MessageManager::callAsync([callback, code]() {
                callback(code, "");
            });
        } else if (error.isNotEmpty()) {
            ZENITH_LOG_ERROR("[OAuth] Error received: " + error);
            sendResponse(clientSocket.get(), false);
            
            juce::MessageManager::callAsync([callback, error]() {
                callback("", error);
            });
        } else {
            ZENITH_LOG_ERROR("[OAuth] No code or error in callback");
            sendResponse(clientSocket.get(), false);
            
            juce::MessageManager::callAsync([callback]() {
                callback("", "Invalid OAuth callback response");
            });
        }
    } else {
        ZENITH_LOG_ERROR("[OAuth] Failed to read from client socket");
        juce::MessageManager::callAsync([callback]() {
            callback("", "Failed to read OAuth callback");
        });
    }
    
    clientSocket->close();
    serverSocket_->close();
    
    running_.store(false);
    ZENITH_LOG_INFO("[OAuth] Redirect server stopped");
}

juce::String OAuthRedirectServer::parseAuthCode(const juce::String& request) {
    // Request looks like: GET /oauth2callback?code=4/ABC123&scope=... HTTP/1.1
    int codeStart = request.indexOf("code=");
    if (codeStart < 0) return "";
    
    codeStart += 5; // Skip "code="
    int codeEnd = request.indexOfAnyOf("& ", codeStart);
    if (codeEnd < 0) codeEnd = request.length();
    
    return request.substring(codeStart, codeEnd);
}

juce::String OAuthRedirectServer::parseError(const juce::String& request) {
    int errorStart = request.indexOf("error=");
    if (errorStart < 0) return "";
    
    errorStart += 6;
    int errorEnd = request.indexOfAnyOf("& ", errorStart);
    if (errorEnd < 0) errorEnd = request.length();
    
    return juce::URL::removeEscapeChars(request.substring(errorStart, errorEnd));
}

void OAuthRedirectServer::sendResponse(juce::StreamingSocket* clientSocket, bool success) {
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

juce::String OAuthRedirectServer::parseAuthCode(const juce::String& request) {
    // Request looks like: GET /oauth2callback?code=4/ABC123&scope=... HTTP/1.1
    int codeStart = request.indexOf("code=");
    if (codeStart < 0) return "";
    
    codeStart += 5; // Skip "code="
    int codeEnd = request.indexOfAnyOf("& ", codeStart);
    if (codeEnd < 0) codeEnd = request.length();
    
    return request.substring(codeStart, codeEnd);
}

juce::String OAuthRedirectServer::parseError(const juce::String& request) {
    int errorStart = request.indexOf("error=");
    if (errorStart < 0) return "";
    
    errorStart += 6;
    int errorEnd = request.indexOfAnyOf("& ", errorStart);
    if (errorEnd < 0) errorEnd = request.length();
    
    return juce::URL::removeEscapeChars(request.substring(errorStart, errorEnd));
}

void OAuthRedirectServer::sendResponse(int clientSocket, bool success) {
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
    
    send(clientSocket, response.toRawUTF8(), (int)response.length(), 0);
}

} // namespace zenith
