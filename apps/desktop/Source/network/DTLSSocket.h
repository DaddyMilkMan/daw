#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#ifdef ZENITH_ENABLE_COLLAB
#include <openssl/ssl.h>
#endif

namespace zenith {

#ifdef ZENITH_ENABLE_COLLAB
// A wrapper for a DTLS-secured UDP socket using OpenSSL.
class DTLSSocket {
public:
    enum class State {
        Disconnected,
        Handshaking,
        Connected,
        Failed
    };

    DTLSSocket();
    ~DTLSSocket();

    // Initialization and Connection
    bool bind(int localPort);
    void bindToPort(int p) { bind(p); } 
    void shutdown();
    
    // Initiate a DTLS handshake with a peer
    bool connect(const juce::String& peerAddress, int peerPort);

    // Perform a single step of the DTLS handshake. Returns true if the handshake is complete.
    bool performHandshake();

    // I/O Operations
    int write(const void* data, int bytes);
    int read(void* buffer, int bufferSize, juce::String& senderAddress, int& senderPort);

    // State
    State getState() const { return currentState; }
    juce::DatagramSocket& getInternalSocket() { return udpSocket; }

private:
    void initializeSSLContext();
    void freeSSL();

    juce::DatagramSocket udpSocket;
    SSL_CTX* sslContext = nullptr;
    SSL* ssl = nullptr;
    BIO* readBio = nullptr;
    BIO* writeBio = nullptr;
    
    State currentState = State::Disconnected;
    
    // Store peer address info during connection
    juce::String peerAddress;
    int peerPort = 0;

    // Helper to get OpenSSL errors
    void logOpenSSLErrors(const juce::String& message);
};

#else 

// STUB IMPLEMENTATION
class DTLSSocket {
public:
    enum class State {
        Disconnected,
        Handshaking,
        Connected,
        Failed
    };

    DTLSSocket() {}
    ~DTLSSocket() {}

    bool bind(int) { return false; }
    void bindToPort(int) {} 
    void shutdown() {}
    
    bool connect(const juce::String&, int) { return false; }

    bool performHandshake() { return false; }

    int write(const void*, int) { return 0; }
    int read(void*, int, juce::String&, int&) { return 0; }

    State getState() const { return State::Disconnected; }
    
    // Return reference to member socket to satisfy caller
    juce::DatagramSocket& getInternalSocket() { return udpSocket; }

private:
    juce::DatagramSocket udpSocket;
};

#endif

} // namespace zenith
