/*
  ==============================================================================
    SecureWebSocketClient.h
    WebSocket client implementation for collaborative editing
    Replaces non-existent juce::WebSocketClient
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <functional>

namespace zenith {
namespace network {

// Simple WebSocket client implementation using juce::WebInputStream
class SecureWebSocketClient : public juce::Thread,
                              public juce::AsyncUpdater {
public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void connectionOpened() {}
        virtual void connectionClosed(int statusCode, const juce::String& reason) {}
        virtual void connectionError(const juce::String& error) {}
        virtual void messageReceived(const juce::MemoryBlock& message) {}
    };
    
    SecureWebSocketClient();
    ~SecureWebSocketClient() override;
    
    // Connection
    bool connect(const juce::String& url);
    bool connect(const juce::String& host, int port, const juce::String& path = "/");
    void disconnect();
    bool isConnected() const { return isConnectedFlag.load(); }
    
    // Messaging
    bool sendMessage(const juce::MemoryBlock& message);
    bool sendMessage(const juce::String& message);
    bool sendBinary(const juce::MemoryBlock& data);
    
    // Listeners
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // Thread
    void run() override;
    void handleAsyncUpdate() override;
    
private:
    juce::String serverUrl;
    juce::String serverHost;
    int serverPort = 0;
    juce::String serverPath;
    
    std::atomic<bool> isConnectedFlag{false};
    std::atomic<bool> shouldDisconnect{false};
    
    std::unique_ptr<juce::WebInputStream> webStream;
    juce::MemoryBlock receiveBuffer;
    
    std::vector<Listener*> listeners;
    
    // WebSocket framing
    struct WebSocketFrame {
        bool fin = false;
        int opcode = 0;
        bool masked = false;
        uint64_t payloadLength = 0;
        juce::MemoryBlock payload;
        juce::MemoryBlock maskingKey;
    };
    
    bool performHandshake();
    bool sendWebSocketFrame(int opcode, const juce::MemoryBlock& payload, bool mask = true);
    bool readWebSocketFrame(WebSocketFrame& frame);
    juce::MemoryBlock createFrame(int opcode, const juce::MemoryBlock& payload, bool mask);
    
    void notifyConnectionOpened();
    void notifyConnectionClosed(int statusCode, const juce::String& reason);
    void notifyConnectionError(const juce::String& error);
    void notifyMessageReceived(const juce::MemoryBlock& message);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SecureWebSocketClient)
};

// WebSocket server implementation
class SecureWebSocketServer : public juce::Thread,
                              public juce::AsyncUpdater {
public:
    struct Listener {
        virtual ~Listener() = default;
        virtual void clientConnected(const juce::String& clientId) {}
        virtual void clientDisconnected(const juce::String& clientId) {}
        virtual void messageReceived(const juce::String& clientId, const juce::MemoryBlock& message) {}
    };
    
    SecureWebSocketServer();
    ~SecureWebSocketServer() override;
    
    // Server control
    bool start(int port);
    void stop();
    bool isRunning() const { return isRunningFlag.load(); }
    
    // Broadcasting
    void broadcast(const juce::MemoryBlock& message);
    void sendToClient(const juce::String& clientId, const juce::MemoryBlock& message);
    
    // Listeners
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // Thread
    void run() override;
    void handleAsyncUpdate() override;
    
private:
    int serverPort = 0;
    std::atomic<bool> isRunningFlag{false};
    
    std::unique_ptr<juce::StreamingSocket> serverSocket;
    std::vector<std::unique_ptr<juce::StreamingSocket>> clientSockets;
    std::vector<juce::String> clientIds;
    
    std::vector<Listener*> listeners;
    
    void acceptNewClients();
    void handleClientData(int clientIndex);
    void removeClient(int index);
    
    void notifyClientConnected(const juce::String& clientId);
    void notifyClientDisconnected(const juce::String& clientId);
    void notifyMessageReceived(const juce::String& clientId, const juce::MemoryBlock& message);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SecureWebSocketServer)
};

} // namespace network
} // namespace zenith
