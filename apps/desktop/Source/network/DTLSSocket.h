#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <mutex>

#ifdef ZENITH_ENABLE_COLLAB
#include <openssl/ssl.h>
#endif

namespace zenith {

#ifdef ZENITH_ENABLE_COLLAB

// Production-ready DTLS socket with server/client modes
class DTLSSocket {
public:
    enum class State {
        Disconnected,
        Handshaking,
        Connected,
        Failed
    };
    
    enum class Mode {
        Client,
        Server
    };
    
    struct Config {
        Mode mode = Mode::Client;
        juce::String certificatePath;
        juce::String privateKeyPath;
        juce::String caCertificatePath;  // For client: verify server; For server: verify client
        bool verifyPeer = true;
        int handshakeTimeoutMs = 15000;
        int mtu = 1200;  // Conservative default for DTLS (avoid fragmentation)
    };

    DTLSSocket();
    explicit DTLSSocket(const Config& config);
    ~DTLSSocket();

    // Non-copyable
    DTLSSocket(const DTLSSocket&) = delete;
    DTLSSocket& operator=(const DTLSSocket&) = delete;
    
    // Movable
    DTLSSocket(DTLSSocket&& other) noexcept;
    DTLSSocket& operator=(DTLSSocket&& other) noexcept;

    // Initialization
    bool initialize(const Config& config);
    bool bind(int localPort);
    void bindToPort(int p) { bind(p); }  // NOLINT
    void shutdown();
    
    // Connection
    bool connect(const juce::String& peerAddress, int peerPort);
    bool acceptConnection();  // For server mode
    
    // Handshake (non-blocking, call repeatedly)
    bool performHandshake();
    bool isHandshakeComplete() const;
    
    // I/O (thread-safe)
    int write(const void* data, int bytes);
    int read(void* buffer, int bufferSize, juce::String& senderAddress, int& senderPort);
    
    // State
    State getState() const { return currentState.load(std::memory_order_acquire); }
    Mode getMode() const { return mode_; }
    juce::DatagramSocket& getInternalSocket() { return udpSocket; }
    const juce::DatagramSocket& getInternalSocket() const { return udpSocket; }
    
    // Statistics
    struct Stats {
        juce::uint64 bytesSent = 0;
        juce::uint64 bytesReceived = 0;
        juce::uint64 packetsSent = 0;
        juce::uint64 packetsReceived = 0;
        juce::uint64 handshakeAttempts = 0;
        juce::uint64 handshakeFailures = 0;
    };
    Stats getStats() const;
    void resetStats();

private:
    void initializeSSLContext(const Config& config);
    void freeSSL();
    bool loadCertificates(const Config& config);
    bool setupDtlsBios();
    void logOpenSSLErrors(const juce::String& message);
    void setFailedState();
    
    // MTU handling
    static constexpr int MAX_DTLS_PAYLOAD = 16384;  // Max DTLS record size
    static constexpr int UDP_HEADER_SIZE = 8;
    static constexpr int IP_HEADER_SIZE = 20;
    static constexpr int IPv6_HEADER_SIZE = 40;
    int getMaxPayloadSize() const;

    juce::DatagramSocket udpSocket;
    
    // SSL state - protected by mutex for thread safety
    mutable std::mutex sslMutex;
    SSL_CTX* sslContext = nullptr;
    SSL* ssl = nullptr;
    BIO* readBio = nullptr;
    BIO* writeBio = nullptr;
    
    std::atomic<State> currentState{State::Disconnected};
    Mode mode_ = Mode::Client;
    int mtu_ = 1200;
    
    // Peer info
    juce::String peerAddress;
    int peerPort = 0;
    
    // Handshake tracking
    juce::int64 handshakeStartTime = 0;
    int handshakeTimeoutMs_ = 15000;
    
    // Statistics
    mutable std::mutex statsMutex;
    Stats stats_;
};

#else 

// Stub IMPLEMENTATION
class DTLSSocket {
public:
    enum class State { Disconnected, Handshaking, Connected, Failed };
    enum class Mode { Client, Server };
    struct Config { Mode mode = Mode::Client; };
    struct Stats {};

    DTLSSocket() = default;
    explicit DTLSSocket(const Config&) {}
    ~DTLSSocket() = default;

    bool initialize(const Config&) { return false; }
    bool bind(int) { return false; }
    void bindToPort(int) {}  // NOLINT
    void shutdown() {}
    bool connect(const juce::String&, int) { return false; }
    bool acceptConnection() { return false; }
    bool performHandshake() { return false; }
    bool isHandshakeComplete() const { return false; }
    int write(const void*, int) { return 0; }
    int read(void*, int, juce::String&, int&) { return 0; }
    State getState() const { return State::Disconnected; }
    Mode getMode() const { return Mode::Client; }
    juce::DatagramSocket& getInternalSocket() { return udpSocket; }
    const juce::DatagramSocket& getInternalSocket() const { return udpSocket; }
    Stats getStats() const { return {}; }
    void resetStats() {}

private:
    juce::DatagramSocket udpSocket;
};

#endif

} // namespace zenith
