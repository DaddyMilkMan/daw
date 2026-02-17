/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "EmbeddedMCPHttpServer.h"
#include "../mcp/MCPServer.h"
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith::network {

EmbeddedMCPHttpServer::EmbeddedMCPHttpServer(Engine& engine)
    : juce::Thread("EmbeddedMCPHttpServer"), engine_(engine) {
}

EmbeddedMCPHttpServer::~EmbeddedMCPHttpServer() {
    stop();
}

bool EmbeddedMCPHttpServer::start(int port, const juce::String& host, const juce::String& path) {
    if (running_.load()) {
        return false;
    }

    serverPort_ = port;
    host_ = host;
    basePath_ = path;
    running_.store(true);

    startThread();

    return true;
}

void EmbeddedMCPHttpServer::stop() {
    if (!running_.load() && !isThreadRunning()) {
        return;
    }

    signalThreadShouldExit();
    running_.store(false);
    stopThread(1000);
}

void EmbeddedMCPHttpServer::run() {
    // Create server socket
    juce::StreamingSocket serverSocket;

    if (!serverSocket.bindToPort(serverPort_, host_)) {
        DBG("MCP HTTP Server: Failed to bind to " + host_ + ":" + juce::String(serverPort_));
        running_.store(false);
        return;
    }

    DBG("MCP HTTP Server: Listening on " + host_ + ":" + juce::String(serverPort_) + basePath_);

    while (running_.load() && !threadShouldExit()) {
        const int ready = serverSocket.waitUntilReady(true, 250);
        if (ready <= 0) {
            continue;
        }

        std::unique_ptr<juce::StreamingSocket> clientSocket(serverSocket.waitForNextConnection());
        if (clientSocket != nullptr && clientSocket->isConnected()) {
            handleConnection(clientSocket.get());
        }
    }

    serverSocket.close();
    DBG("MCP HTTP Server: Stopped");
}

void EmbeddedMCPHttpServer::handleConnection(juce::StreamingSocket* socket) {
    if (socket == nullptr || !socket->isConnected()) {
        return;
    }

    constexpr int kBufferSize = 4096;
    char buffer[kBufferSize];
    juce::MemoryOutputStream requestStream;

    int contentLength = 0;
    int headerEndIndex = -1;

    while (requestStream.getDataSize() < 1024 * 1024) {
        const int bytesRead = socket->read(buffer, kBufferSize, true);
        if (bytesRead <= 0) {
            break;
        }

        requestStream.write(buffer, static_cast<size_t>(bytesRead));
        const juce::String requestSoFar(
            static_cast<const char*>(requestStream.getData()),
            static_cast<int>(requestStream.getDataSize()));

        if (headerEndIndex < 0) {
            headerEndIndex = requestSoFar.indexOf("\r\n\r\n");
            if (headerEndIndex >= 0) {
                const juce::String header = requestSoFar.substring(0, headerEndIndex);
                juce::StringArray headerLines;
                headerLines.addLines(header);
                for (const auto& line : headerLines) {
                    if (line.startsWithIgnoreCase("Content-Length:")) {
                        contentLength = line.fromFirstOccurrenceOf(":", false, false)
                                            .trim()
                                            .getIntValue();
                        break;
                    }
                }
            }
        }

        if (headerEndIndex >= 0) {
            const int bodyBytes = static_cast<int>(requestStream.getDataSize()) - (headerEndIndex + 4);
            if (bodyBytes >= contentLength) {
                break;
            }
        }
    }

    const juce::String request(
        static_cast<const char*>(requestStream.getData()),
        static_cast<int>(requestStream.getDataSize()));

    // Parse HTTP request
    juce::StringArray lines;
    lines.addLines(request);

    if (lines.isEmpty()) {
        return;
    }

    juce::StringArray requestParts;
    requestParts.addTokens(lines[0], " ", "");

    if (requestParts.size() < 2) {
        return;
    }

    juce::String method = requestParts[0];
    juce::String path = requestParts[1];

    // Only handle POST requests to MCP endpoints
    if (method != "POST" || !path.startsWith(basePath_)) {
        // Return 404
        juce::String response = "HTTP/1.1 404 Not Found\r\n"
                               "Content-Type: text/plain\r\n"
                               "Connection: close\r\n"
                               "\r\n"
                               "Not Found";
        socket->write(response.toRawUTF8(), response.length());
        return;
    }

    // Handle CORS preflight
    if (method == "OPTIONS" && path.startsWith(basePath_)) {
        juce::String response = "HTTP/1.1 204 No Content\r\n"
                               "Connection: close\r\n"
                               "Access-Control-Allow-Origin: *\r\n"
                               "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                               "Access-Control-Allow-Headers: Content-Type\r\n"
                               "\r\n";
        socket->write(response.toRawUTF8(), response.length());
        return;
    }

    // Read request body from the already-buffered request
    juce::String body;
    const int headerEnd = request.indexOf("\r\n\r\n");
    if (headerEnd >= 0) {
        body = request.substring(headerEnd + 4);
        if (contentLength > 0 && body.getNumBytesAsUTF8() > contentLength) {
            body = body.substring(0, contentLength);
        }
    }

    juce::var requestJson;
    juce::var requestId;
    if (body.isNotEmpty()) {
        requestJson = juce::JSON::parse(body);
        if (auto* obj = requestJson.getDynamicObject()) {
            requestId = obj->getProperty("id");
        }
    }

    juce::DynamicObject::Ptr resultObj = new juce::DynamicObject();
    resultObj->setProperty("status", "ok");
    resultObj->setProperty("message", "Embedded MCP HTTP server online");

    juce::DynamicObject::Ptr responseObj = new juce::DynamicObject();
    responseObj->setProperty("jsonrpc", "2.0");
    responseObj->setProperty("result", juce::var(resultObj.get()));
    responseObj->setProperty("id", requestId);

    const juce::String responseBody = juce::JSON::toString(juce::var(responseObj.get()), false);

    // Send HTTP response
    juce::String httpResponse = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: application/json\r\n"
                               "Content-Length: " + juce::String(responseBody.length()) + "\r\n"
                               "Connection: close\r\n"
                               "Access-Control-Allow-Origin: *\r\n"
                               "\r\n" + responseBody;

    socket->write(httpResponse.toRawUTF8(), httpResponse.length());
}

} // namespace zenith::network
