/*
  ==============================================================================
    SecureWebSocketClient.cpp
    WebSocket client implementation
  ==============================================================================
*/

#include "SecureWebSocketClient.h"
#include <random>

namespace zenith {
namespace network {

// SecureWebSocketClient Implementation
SecureWebSocketClient::SecureWebSocketClient() : juce::Thread("WebSocketClient") {
}

SecureWebSocketClient::~SecureWebSocketClient() {
    disconnect();
    stopThread(1000);
}

bool SecureWebSocketClient::connect(const juce::String& url) {
    serverUrl = url;
    
    // Parse URL
    juce::URL parsedUrl(url);
    serverHost = parsedUrl.getDomain();
    serverPort = parsedUrl.getPort();
    serverPath = parsedUrl.getPath();
    
    if (serverPort == 0) {
        serverPort = parsedUrl.getScheme() == "wss" ? 443 : 80;
    }
    
    return connect(serverHost, serverPort, serverPath);
}

bool SecureWebSocketClient::connect(const juce::String& host, int port, const juce::String& path) {
    serverHost = host;
    serverPort = port;
    serverPath = path;
    
    // Start connection thread
    startThread();
    
    return true;
}

void SecureWebSocketClient::disconnect() {
    shouldDisconnect.store(true);
    notify();
    stopThread(1000);
}

bool SecureWebSocketClient::sendMessage(const juce::MemoryBlock& message) {
    return sendWebSocketFrame(0x2, message, true); // Binary frame
}

bool SecureWebSocketClient::sendMessage(const juce::String& message) {
    return sendWebSocketFrame(0x1, message.toUTF8(), true); // Text frame
}

bool SecureWebSocketClient::sendBinary(const juce::MemoryBlock& data) {
    return sendWebSocketFrame(0x2, data, true); // Binary frame
}

void SecureWebSocketClient::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void SecureWebSocketClient::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void SecureWebSocketClient::run() {
    // Create WebSocket URL
    juce::String wsUrl = (serverPort == 443 ? "wss://" : "ws://") + serverHost + ":" + 
                         juce::String(serverPort) + serverPath;
    
    // Open connection
    webStream = std::make_unique<juce::WebInputStream>(wsUrl, false);
    
    if (!webStream->connect(nullptr)) {
        notifyConnectionError("Failed to connect to server");
        webStream.reset();
        return;
    }
    
    // Perform WebSocket handshake
    if (!performHandshake()) {
        notifyConnectionError("WebSocket handshake failed");
        webStream.reset();
        return;
    }
    
    isConnectedFlag.store(true);
    notifyConnectionOpened();
    
    // Message loop
    while (!shouldDisconnect.load() && !threadShouldExit()) {
        WebSocketFrame frame;
        if (readWebSocketFrame(frame)) {
            if (frame.opcode == 0x1 || frame.opcode == 0x2) { // Text or binary
                notifyMessageReceived(frame.payload);
            } else if (frame.opcode == 0x8) { // Close
                break;
            }
        }
        
        wait(10); // Small delay to prevent busy waiting
    }
    
    // Cleanup
    isConnectedFlag.store(false);
    webStream.reset();
    notifyConnectionClosed(1000, "Normal closure");
}

void SecureWebSocketClient::handleAsyncUpdate() {
    // Handle async updates if needed
}

bool SecureWebSocketClient::performHandshake() {
    if (!webStream) return false;
    
    // Generate WebSocket key
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    juce::String key;
    for (int i = 0; i < 16; ++i) {
        key += juce::String::formatted("%02x", dis(gen));
    }
    
    // Send handshake request
    juce::String request = "GET " + serverPath + " HTTP/1.1\r\n";
    request += "Host: " + serverHost + ":" + juce::String(serverPort) + "\r\n";
    request += "Upgrade: websocket\r\n";
    request += "Connection: Upgrade\r\n";
    request += "Sec-WebSocket-Key: " + key + "\r\n";
    request += "Sec-WebSocket-Version: 13\r\n";
    request += "\r\n";
    
    webStream->write(request.toRawUTF8(), request.length());
    
    // Read response
    juce::MemoryBlock responseBuffer(1024);
    int bytesRead = webStream->read(responseBuffer.getData(), responseBuffer.getSize(), false);
    
    if (bytesRead <= 0) return false;
    
    juce::String response(reinterpret_cast<const char*>(responseBuffer.getData()), bytesRead);
    
    // Check response
    if (!response.startsWith("HTTP/1.1 101")) {
        return false;
    }
    
    return true;
}

bool SecureWebSocketClient::sendWebSocketFrame(int opcode, const juce::MemoryBlock& payload, bool mask) {
    if (!webStream || !isConnectedFlag.load()) return false;
    
    juce::MemoryBlock frame = createFrame(opcode, payload, mask);
    int bytesWritten = webStream->write(frame.getData(), frame.getSize());
    
    return bytesWritten == frame.getSize();
}

bool SecureWebSocketClient::readWebSocketFrame(WebSocketFrame& frame) {
    if (!webStream) return false;
    
    // Read header
    uint8_t header[2];
    if (webStream->read(header, 2, false) != 2) return false;
    
    frame.fin = (header[0] & 0x80) != 0;
    frame.opcode = header[0] & 0x0F;
    frame.masked = (header[1] & 0x80) != 0;
    
    // Read payload length
    uint64_t payloadLength = header[1] & 0x7F;
    
    if (payloadLength == 126) {
        uint8_t extended[2];
        if (webStream->read(extended, 2, false) != 2) return false;
        payloadLength = (extended[0] << 8) | extended[1];
    } else if (payloadLength == 127) {
        uint8_t extended[8];
        if (webStream->read(extended, 8, false) != 8) return false;
        payloadLength = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLength = (payloadLength << 8) | extended[i];
        }
    }
    
    frame.payloadLength = payloadLength;
    
    // Read masking key if present
    if (frame.masked) {
        frame.maskingKey.resize(4);
        if (webStream->read(frame.maskingKey.getData(), 4, false) != 4) return false;
    }
    
    // Read payload
    frame.payload.resize(payloadLength);
    if (payloadLength > 0) {
        if (webStream->read(frame.payload.getData(), payloadLength, false) != static_cast<int>(payloadLength)) {
            return false;
        }
        
        // Unmask payload if needed
        if (frame.masked) {
            uint8_t* payloadData = static_cast<uint8_t*>(frame.payload.getData());
            const uint8_t* maskData = static_cast<const uint8_t*>(frame.maskingKey.getData());
            
            for (uint64_t i = 0; i < payloadLength; ++i) {
                payloadData[i] ^= maskData[i % 4];
            }
        }
    }
    
    return true;
}

juce::MemoryBlock SecureWebSocketClient::createFrame(int opcode, const juce::MemoryBlock& payload, bool mask) {
    juce::MemoryBlock frame;
    uint64_t payloadLength = payload.getSize();
    
    // Calculate header size
    size_t headerSize = 2;
    if (payloadLength > 65535) {
        headerSize += 8;
    } else if (payloadLength > 125) {
        headerSize += 2;
    }
    
    if (mask) {
        headerSize += 4;
    }
    
    frame.setSize(headerSize + payloadLength);
    uint8_t* frameData = static_cast<uint8_t*>(frame.getData());
    
    // Write frame header
    frameData[0] = 0x80 | opcode; // FIN + opcode
    frameData[1] = mask ? 0x80 : 0;
    
    // Write payload length
    if (payloadLength <= 125) {
        frameData[1] |= static_cast<uint8_t>(payloadLength);
    } else if (payloadLength <= 65535) {
        frameData[1] |= 126;
        frameData[2] = (payloadLength >> 8) & 0xFF;
        frameData[3] = payloadLength & 0xFF;
    } else {
        frameData[1] |= 127;
        for (int i = 0; i < 8; ++i) {
            frameData[2 + i] = (payloadLength >> (56 - i * 8)) & 0xFF;
        }
    }
    
    size_t maskOffset = 2 + (payloadLength > 125 ? (payloadLength > 65535 ? 8 : 2) : 0);
    
    // Write masking key if needed
    if (mask) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (int i = 0; i < 4; ++i) {
            frameData[maskOffset + i] = dis(gen);
        }
        
        // Write and mask payload
        const uint8_t* maskKey = frameData + maskOffset;
        uint8_t* payloadData = frameData + maskOffset + 4;
        const uint8_t* sourceData = static_cast<const uint8_t*>(payload.getData());
        
        for (uint64_t i = 0; i < payloadLength; ++i) {
            payloadData[i] = sourceData[i] ^ maskKey[i % 4];
        }
    } else {
        // Write payload unmasked
        if (payloadLength > 0) {
            std::memcpy(frameData + maskOffset, payload.getData(), payloadLength);
        }
    }
    
    return frame;
}

void SecureWebSocketClient::notifyConnectionOpened() {
    triggerAsyncUpdate();
    for (auto* listener : listeners) {
        listener->connectionOpened();
    }
}

void SecureWebSocketClient::notifyConnectionClosed(int statusCode, const juce::String& reason) {
    triggerAsyncUpdate();
    for (auto* listener : listeners) {
        listener->connectionClosed(statusCode, reason);
    }
}

void SecureWebSocketClient::notifyConnectionError(const juce::String& error) {
    triggerAsyncUpdate();
    for (auto* listener : listeners) {
        listener->connectionError(error);
    }
}

void SecureWebSocketClient::notifyMessageReceived(const juce::MemoryBlock& message) {
    triggerAsyncUpdate();
    for (auto* listener : listeners) {
        listener->messageReceived(message);
    }
}

// SecureWebSocketServer Implementation
SecureWebSocketServer::SecureWebSocketServer() : juce::Thread("WebSocketServer") {
}

SecureWebSocketServer::~SecureWebSocketServer() {
    stop();
    stopThread(1000);
}

bool SecureWebSocketServer::start(int port) {
    serverPort = port;
    startThread();
    return true;
}

void SecureWebSocketServer::stop() {
    isRunningFlag.store(false);
    notify();
    stopThread(1000);
    
    // Close all connections
    clientSockets.clear();
    clientIds.clear();
    
    if (serverSocket) {
        serverSocket->close();
        serverSocket.reset();
    }
}

void SecureWebSocketServer::broadcast(const juce::MemoryBlock& message) {
    for (size_t i = 0; i < clientSockets.size(); ++i) {
        if (clientSockets[i] && clientSockets[i]->isConnected()) {
            // Simplified - would need proper WebSocket framing
            clientSockets[i]->write(message.getData(), message.getSize());
        }
    }
}

void SecureWebSocketServer::sendToClient(const juce::String& clientId, const juce::MemoryBlock& message) {
    for (size_t i = 0; i < clientIds.size(); ++i) {
        if (clientIds[i] == clientId && clientSockets[i] && clientSockets[i]->isConnected()) {
            // Simplified - would need proper WebSocket framing
            clientSockets[i]->write(message.getData(), message.getSize());
            break;
        }
    }
}

void SecureWebSocketServer::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void SecureWebSocketServer::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void SecureWebSocketServer::run() {
    // Create server socket
    serverSocket = std::make_unique<juce::StreamingSocket>();
    
    if (!serverSocket->createListener(serverPort)) {
        return;
    }
    
    isRunningFlag.store(true);
    
    while (isRunningFlag.load() && !threadShouldExit()) {
        acceptNewClients();
        
        // Handle client data
        for (int i = 0; i < static_cast<int>(clientSockets.size()); ++i) {
            if (clientSockets[i] && clientSockets[i]->isConnected()) {
                handleClientData(i);
            } else {
                removeClient(i);
                --i; // Adjust index after removal
            }
        }
        
        wait(10); // Small delay
    }
}

void SecureWebSocketServer::handleAsyncUpdate() {
    // Handle async updates if needed
}

void SecureWebSocketServer::acceptNewClients() {
    if (!serverSocket) return;
    
    auto clientSocket = std::make_unique<juce::StreamingSocket>();
    
    if (clientSocket->waitForNextConnection()) {
        // Generate client ID
        juce::String clientId = juce::Uuid().toString();
        
        // Add to client list
        clientSockets.push_back(std::move(clientSocket));
        clientIds.push_back(clientId);
        
        notifyClientConnected(clientId);
    }
}

void SecureWebSocketServer::handleClientData(int clientIndex) {
    if (clientIndex < 0 || clientIndex >= static_cast<int>(clientSockets.size())) return;
    
    auto& socket = clientSockets[clientIndex];
    if (!socket || !socket->isConnected()) return;
    
    char buffer[1024];
    int bytesRead = socket->read(buffer, sizeof(buffer), false);
    
    if (bytesRead > 0) {
        juce::MemoryBlock message(buffer, bytesRead);
        notifyMessageReceived(clientIds[clientIndex], message);
    }
}

void SecureWebSocketServer::removeClient(int index) {
    if (index < 0 || index >= static_cast<int>(clientSockets.size())) return;
    
    juce::String clientId = clientIds[index];
    
    clientSockets.erase(clientSockets.begin() + index);
    clientIds.erase(clientIds.begin() + index);
    
    notifyClientDisconnected(clientId);
}

void SecureWebSocketServer::notifyClientConnected(const juce::String& clientId) {
    triggerAsyncUpdate();
    for (auto* listener : listeners) {
        listener->clientConnected(clientId);
    }
}

void SecureWebSocketServer::notifyClientDisconnected(const juce::String& clientId) {
    triggerAsyncUpdate();
    for (auto* listener : listeners) {
        listener->clientDisconnected(clientId);
    }
}

void SecureWebSocketServer::notifyMessageReceived(const juce::String& clientId, const juce::MemoryBlock& message) {
    triggerAsyncUpdate();
    for (auto* listener : listeners) {
        listener->messageReceived(clientId, message);
    }
}

} // namespace network
} // namespace zenith
