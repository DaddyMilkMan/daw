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
#include <cstring>

namespace zenith {

OAuthRedirectServer::~OAuthRedirectServer() {
    stop();
}

void OAuthRedirectServer::startAndWait(int port, CodeReceivedCallback callback, int timeoutSeconds) {
    fprintf(stderr, "[OAuth] startAndWait called for port %d\n", port);
    if (running_.load()) {
        ZENITH_LOG_WARNING("[OAuth] Server already running");
        return;
    }
    
    shouldStop_.store(false);
    running_.store(true);
    
    fprintf(stderr, "[OAuth] Spawning server thread...\n");
    // Run server on background thread
    serverThread_ = std::make_unique<std::thread>([this, port, callback, timeoutSeconds]() {
        runServer(port, callback, timeoutSeconds);
    });
    fprintf(stderr, "[OAuth] Server thread spawned\n");
}

void OAuthRedirectServer::stop() {
    fprintf(stderr, "[OAuth] stop() called\n");
    shouldStop_.store(true);
    
#if JUCE_LINUX || JUCE_MAC
    if (serverSocket_ >= 0) {
        fprintf(stderr, "[OAuth] Shutting down socket %d\n", serverSocket_);
        shutdown(serverSocket_, SHUT_RDWR);
        close(serverSocket_);
        serverSocket_ = -1;
    }
#elif JUCE_WINDOWS
    if (serverSocket_ != INVALID_SOCKET) {
        shutdown(serverSocket_, SD_BOTH);
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }
#endif
    
    if (serverThread_ && serverThread_->joinable()) {
        fprintf(stderr, "[OAuth] Joining server thread...\n");
        serverThread_->join();
        fprintf(stderr, "[OAuth] Server thread joined\n");
    }
    serverThread_.reset();
    running_.store(false);
    fprintf(stderr, "[OAuth] stop() complete\n");
}

void OAuthRedirectServer::runServer(int port, CodeReceivedCallback callback, int timeoutSeconds) {
    ZENITH_LOG_INFO("[OAuth] Starting redirect server on port " + juce::String(port));
    
#if JUCE_WINDOWS
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ZENITH_LOG_ERROR("[OAuth] WSAStartup failed");
        callback("", "Failed to initialize Winsock");
        running_.store(false);
        return;
    }
#endif
    
    // Create socket
#if JUCE_LINUX || JUCE_MAC
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        ZENITH_LOG_ERROR("[OAuth] Failed to create socket");
        callback("", "Failed to create socket");
        running_.store(false);
        return;
    }
    
    // Allow reuse
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1 only
    addr.sin_port = htons(port);
    
    if (bind(serverSocket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ZENITH_LOG_ERROR("[OAuth] Failed to bind to port " + juce::String(port));
        close(serverSocket_);
        serverSocket_ = -1;
        callback("", "Failed to bind to port " + juce::String(port));
        running_.store(false);
        return;
    }
    
    // Listen
    if (listen(serverSocket_, 1) < 0) {
        ZENITH_LOG_ERROR("[OAuth] Failed to listen on socket");
        close(serverSocket_);
        serverSocket_ = -1;
        callback("", "Failed to listen on socket");
        running_.store(false);
        return;
    }
    
    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    setsockopt(serverSocket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    ZENITH_LOG_INFO("[OAuth] Waiting for Google callback on port " + juce::String(port) + "...");
    
    // Accept connection
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
    
    if (clientSocket < 0) {
        if (!shouldStop_.load()) {
            ZENITH_LOG_WARNING("[OAuth] Accept timed out or failed");
            callback("", "Timeout waiting for OAuth callback");
        }
        close(serverSocket_);
        serverSocket_ = -1;
        running_.store(false);
        return;
    }
    
    // Read request
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead > 0) {
        juce::String request(buffer, (size_t)bytesRead);
        ZENITH_LOG_INFO("[OAuth] Received callback request");
        
        // Parse authorization code or error
        juce::String code = parseAuthCode(request);
        juce::String error = parseError(request);
        
        if (code.isNotEmpty()) {
            ZENITH_LOG_INFO("[OAuth] Authorization code received!");
            sendResponse(clientSocket, true);
            
            // Post callback to message thread
            juce::MessageManager::callAsync([callback, code]() {
                callback(code, "");
            });
        } else if (error.isNotEmpty()) {
            ZENITH_LOG_ERROR("[OAuth] Error received: " + error);
            sendResponse(clientSocket, false);
            
            juce::MessageManager::callAsync([callback, error]() {
                callback("", error);
            });
        } else {
            ZENITH_LOG_ERROR("[OAuth] No code or error in callback");
            sendResponse(clientSocket, false);
            
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
    
    close(clientSocket);
    close(serverSocket_);
    serverSocket_ = -1;
    
#elif JUCE_WINDOWS
    // Windows implementation similar but with winsock functions
    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == INVALID_SOCKET) {
        callback("", "Failed to create socket");
        WSACleanup();
        running_.store(false);
        return;
    }
    
    // Similar bind/listen/accept logic for Windows...
    // (Simplified for brevity - production would need full impl)
    closesocket(serverSocket_);
    WSACleanup();
#endif
    
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
