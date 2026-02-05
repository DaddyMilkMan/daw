/*
  ==============================================================================
    DTLSSocket.cpp
    Production-ready DTLS 1.2 implementation with client/server modes
  ==============================================================================
*/

#include "DTLSSocket.h"

namespace zenith {

#ifdef ZENITH_ENABLE_COLLAB

#include <openssl/err.h>
#include <openssl/x509_vfy.h>

//==============================================================================
// Certificate Loading Helpers
//==============================================================================

static bool loadCertificateChain(SSL_CTX* ctx, const juce::String& certPath) {
    if (certPath.isEmpty()) return false;
    
    juce::File certFile(certPath);
    if (!certFile.existsAsFile()) {
        DBG("DTLS Error: Certificate file not found: " + certPath);
        return false;
    }
    
    if (SSL_CTX_use_certificate_chain_file(ctx, certFile.getFullPathName().toRawUTF8()) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

static bool loadPrivateKey(SSL_CTX* ctx, const juce::String& keyPath) {
    if (keyPath.isEmpty()) return false;
    
    juce::File keyFile(keyPath);
    if (!keyFile.existsAsFile()) {
        DBG("DTLS Error: Private key file not found: " + keyPath);
        return false;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, keyFile.getFullPathName().toRawUTF8(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }
    
    if (!SSL_CTX_check_private_key(ctx)) {
        DBG("DTLS Error: Private key does not match certificate");
        return false;
    }
    return true;
}

static bool loadCAStore(SSL_CTX* ctx, const juce::String& caPath) {
    if (caPath.isEmpty()) return true;  // Optional
    
    juce::File caFile(caPath);
    if (!caFile.existsAsFile()) {
        DBG("DTLS Warning: CA certificate not found: " + caPath);
        return false;
    }
    
    if (SSL_CTX_load_verify_locations(ctx, caFile.getFullPathName().toRawUTF8(), nullptr) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

//==============================================================================
// DTLSSocket Implementation
//==============================================================================

DTLSSocket::DTLSSocket() = default;

DTLSSocket::DTLSSocket(const Config& config) {
    initialize(config);
}

DTLSSocket::~DTLSSocket() {
    shutdown();
}

DTLSSocket::DTLSSocket(DTLSSocket&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.sslMutex);
    
    sslContext = other.sslContext;
    ssl = other.ssl;
    readBio = other.readBio;
    writeBio = other.writeBio;
    currentState.store(other.currentState.load(std::memory_order_acquire));
    mode_ = other.mode_;
    mtu_ = other.mtu_;
    peerAddress = other.peerAddress;
    peerPort = other.peerPort;
    handshakeTimeoutMs_ = other.handshakeTimeoutMs_;
    stats_ = other.stats_;
    
    other.sslContext = nullptr;
    other.ssl = nullptr;
    other.readBio = nullptr;
    other.writeBio = nullptr;
    other.currentState.store(State::Disconnected);
}

DTLSSocket& DTLSSocket::operator=(DTLSSocket&& other) noexcept {
    if (this != &other) {
        shutdown();
        
        std::lock_guard<std::mutex> lock(other.sslMutex);
        
        sslContext = other.sslContext;
        ssl = other.ssl;
        readBio = other.readBio;
        writeBio = other.writeBio;
        currentState.store(other.currentState.load(std::memory_order_acquire));
        mode_ = other.mode_;
        mtu_ = other.mtu_;
        peerAddress = other.peerAddress;
        peerPort = other.peerPort;
        handshakeTimeoutMs_ = other.handshakeTimeoutMs_;
        stats_ = other.stats_;
        
        other.sslContext = nullptr;
        other.ssl = nullptr;
        other.readBio = nullptr;
        other.writeBio = nullptr;
        other.currentState.store(State::Disconnected);
    }
    return *this;
}

bool DTLSSocket::initialize(const Config& config) {
    std::lock_guard<std::mutex> lock(sslMutex);
    
    if (sslContext) {
        DBG("DTLS: Already initialized");
        return false;
    }
    
    mode_ = config.mode;
    mtu_ = juce::jlimit(576, 1500, config.mtu);  // Reasonable MTU range
    handshakeTimeoutMs_ = config.handshakeTimeoutMs;
    
    initializeSSLContext(config);
    
    if (!sslContext) {
        return false;
    }
    
    return true;
}

void DTLSSocket::initializeSSLContext(const Config& config) {
    const SSL_METHOD* method = (config.mode == Mode::Server) 
        ? DTLS_server_method() 
        : DTLS_client_method();
    
    sslContext = SSL_CTX_new(method);
    if (!sslContext) {
        logOpenSSLErrors("Failed to create SSL context");
        return;
    }
    
    // Set DTLS 1.2 minimum
    SSL_CTX_set_min_proto_version(sslContext, DTLS1_2_VERSION);
    
    // Load certificates
    if (!loadCertificates(config)) {
        SSL_CTX_free(sslContext);
        sslContext = nullptr;
        return;
    }
    
    // Configure verification
    int verifyMode = config.verifyPeer ? SSL_VERIFY_PEER : SSL_VERIFY_NONE;
    if (config.mode == Mode::Server && config.verifyPeer) {
        verifyMode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
    SSL_CTX_set_verify(sslContext, verifyMode, nullptr);
    
    // Set MTU for DTLS (per-context not available in all OpenSSL versions)
    #if OPENSSL_VERSION_NUMBER >= 0x10002000L
    SSL_CTX_set_options(sslContext, SSL_OP_NO_QUERY_MTU);
    #endif
    
    // Enable partial write for non-blocking operation
    SSL_CTX_set_mode(sslContext, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
}

bool DTLSSocket::loadCertificates(const Config& config) {
    // Load our certificate and private key
    if (!config.certificatePath.isEmpty() && !config.privateKeyPath.isEmpty()) {
        if (!loadCertificateChain(sslContext, config.certificatePath)) {
            return false;
        }
        if (!loadPrivateKey(sslContext, config.privateKeyPath)) {
            return false;
        }
    } else if (config.mode == Mode::Server) {
        DBG("DTLS Error: Server mode requires certificate and private key");
        return false;
    }
    
    // Load CA store for verification
    if (!loadCAStore(sslContext, config.caCertificatePath)) {
        // Non-fatal for client mode without custom CA
        if (config.mode == Mode::Server && config.verifyPeer) {
            return false;
        }
    }
    
    return true;
}

bool DTLSSocket::setupDtlsBios() {
    readBio = BIO_new(BIO_s_mem());
    writeBio = BIO_new(BIO_s_mem());
    
    if (!readBio || !writeBio) {
        logOpenSSLErrors("Failed to create BIOs");
        return false;
    }
    
    BIO_set_mem_eof_return(readBio, -1);
    BIO_set_mem_eof_return(writeBio, -1);
    
    SSL_set_bio(ssl, readBio, writeBio);
    return true;
}

void DTLSSocket::freeSSL() {
    std::lock_guard<std::mutex> lock(sslMutex);
    
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
        readBio = nullptr;
        writeBio = nullptr;
    }
}

void DTLSSocket::shutdown() {
    freeSSL();
    
    {
        std::lock_guard<std::mutex> lock(sslMutex);
        if (sslContext) {
            SSL_CTX_free(sslContext);
            sslContext = nullptr;
        }
    }
    
    udpSocket.shutdown();
    currentState.store(State::Disconnected, std::memory_order_release);
}

bool DTLSSocket::bind(int localPort) {
    return udpSocket.bindToPort(localPort);
}

//==============================================================================
// Connection
//==============================================================================

bool DTLSSocket::connect(const juce::String& peerAddr, int pPort) {
    std::lock_guard<std::mutex> lock(sslMutex);
    
    if (mode_ != Mode::Client) {
        DBG("DTLS Error: Cannot connect in server mode");
        return false;
    }
    
    State state = currentState.load(std::memory_order_acquire);
    if (state != State::Disconnected && state != State::Failed) {
        DBG("DTLS Error: Already connected or connecting");
        return false;
    }
    
    if (!sslContext) {
        DBG("DTLS Error: Not initialized");
        return false;
    }
    
    peerAddress = peerAddr;
    peerPort = pPort;
    
    freeSSL();
    
    ssl = SSL_new(sslContext);
    if (!ssl) {
        logOpenSSLErrors("Failed to create SSL");
        setFailedState();
        return false;
    }
    
    #if OPENSSL_VERSION_NUMBER >= 0x10001000L
    SSL_set_mtu(ssl, static_cast<long>(mtu_));
    #else
    (void)mtu_;  // Unused in older OpenSSL
    #endif
    
    if (!setupDtlsBios()) {
        setFailedState();
        return false;
    }
    
    SSL_set_connect_state(ssl);
    currentState.store(State::Handshaking, std::memory_order_release);
    handshakeStartTime = juce::Time::currentTimeMillis();
    
    return performHandshake();
}

bool DTLSSocket::acceptConnection() {
    std::lock_guard<std::mutex> lock(sslMutex);
    
    if (mode_ != Mode::Server) {
        DBG("DTLS Error: Cannot accept in client mode");
        return false;
    }
    
    State state = currentState.load(std::memory_order_acquire);
    if (state != State::Disconnected && state != State::Failed) {
        return false;
    }
    
    if (!sslContext) {
        DBG("DTLS Error: Not initialized");
        return false;
    }
    
    freeSSL();
    
    ssl = SSL_new(sslContext);
    if (!ssl) {
        logOpenSSLErrors("Failed to create SSL");
        setFailedState();
        return false;
    }
    
    #if OPENSSL_VERSION_NUMBER >= 0x10001000L
    SSL_set_mtu(ssl, static_cast<long>(mtu_));
    #else
    (void)mtu_;  // Unused in older OpenSSL
    #endif
    
    if (!setupDtlsBios()) {
        setFailedState();
        return false;
    }
    
    SSL_set_accept_state(ssl);
    currentState.store(State::Handshaking, std::memory_order_release);
    handshakeStartTime = juce::Time::currentTimeMillis();
    
    return true;
}

//==============================================================================
// Handshake
//==============================================================================

bool DTLSSocket::performHandshake() {
    std::lock_guard<std::mutex> lock(sslMutex);
    
    if (!ssl) return false;
    
    State state = currentState.load(std::memory_order_acquire);
    if (state != State::Handshaking) {
        return (state == State::Connected);
    }
    
    // Check timeout
    if (juce::Time::currentTimeMillis() - handshakeStartTime > handshakeTimeoutMs_) {
        logOpenSSLErrors("Handshake timeout");
        setFailedState();
        return false;
    }
    
    std::vector<char> buffer(mtu_);
    
    int ret = SSL_do_handshake(ssl);
    
    if (ret == 1) {
        currentState.store(State::Connected, std::memory_order_release);
        
        {
            std::lock_guard<std::mutex> statsLock(statsMutex);
            stats_.handshakeAttempts++;
        }
        return true;
    }
    
    int err = SSL_get_error(ssl, ret);
    
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        // Send any pending data
        int pending = BIO_ctrl_pending(writeBio);
        if (pending > 0) {
            int toSend = BIO_read(writeBio, buffer.data(), static_cast<int>(buffer.size()));
            if (toSend > 0) {
                int sent = udpSocket.write(peerAddress, peerPort, buffer.data(), toSend);
                if (sent < 0) {
                    logOpenSSLErrors("Failed to send handshake data");
                    setFailedState();
                    return false;
                }
            }
        }
        
        // Non-blocking - need more data
        return false;
    } else {
        logOpenSSLErrors("Handshake failed");
        
        {
            std::lock_guard<std::mutex> statsLock(statsMutex);
            stats_.handshakeAttempts++;
            stats_.handshakeFailures++;
        }
        
        setFailedState();
        return false;
    }
}

bool DTLSSocket::isHandshakeComplete() const {
    return currentState.load(std::memory_order_acquire) == State::Connected;
}

//==============================================================================
// I/O Operations
//==============================================================================

int DTLSSocket::write(const void* data, int bytes) {
    if (bytes <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(sslMutex);
    
    if (currentState.load(std::memory_order_acquire) != State::Connected) {
        return -1;
    }
    
    if (!ssl) return -1;
    
    // Check max payload
    int maxPayload = getMaxPayloadSize();
    if (bytes > maxPayload) {
        DBG("DTLS Warning: Packet too large (" + juce::String(bytes) + 
            " bytes), truncating to " + juce::String(maxPayload));
        bytes = maxPayload;
    }
    
    int bytesWritten = SSL_write(ssl, data, bytes);
    if (bytesWritten <= 0) {
        int err = SSL_get_error(ssl, bytesWritten);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return 0;  // Would block
        }
        logOpenSSLErrors("SSL_write failed");
        setFailedState();
        return -1;
    }
    
    // Flush encrypted data to network
    std::vector<char> buffer(mtu_);
    int pending = BIO_ctrl_pending(writeBio);
    int totalSent = 0;
    
    while (pending > 0) {
        int toSend = BIO_read(writeBio, buffer.data(), static_cast<int>(buffer.size()));
        if (toSend <= 0) break;
        
        int sent = udpSocket.write(peerAddress, peerPort, buffer.data(), toSend);
        if (sent < 0) {
            logOpenSSLErrors("UDP write failed");
            setFailedState();
            return -1;
        }
        totalSent += sent;
        pending = BIO_ctrl_pending(writeBio);
    }
    
    if (totalSent > 0) {
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats_.bytesSent += totalSent;
        stats_.packetsSent++;
    }
    
    return bytesWritten;
}

int DTLSSocket::read(void* buffer, int bufferSize, juce::String& senderAddress, int& senderPort) {
    if (bufferSize <= 0) return 0;
    
    std::lock_guard<std::mutex> lock(sslMutex);
    
    if (currentState.load(std::memory_order_acquire) != State::Connected) {
        return 0;
    }
    
    if (!ssl) return 0;
    
    // Read from network
    std::vector<char> tempBuffer(mtu_);
    int bytesRead = udpSocket.read(tempBuffer.data(), static_cast<int>(tempBuffer.size()), 
                                   false, senderAddress, senderPort);
    
    if (bytesRead > 0) {
        // Check if this is from our peer (client mode) or update peer info (server mode)
        if (mode_ == Mode::Server && (peerAddress != senderAddress || peerPort != senderPort)) {
            // In server mode, update peer info on first packet
            peerAddress = senderAddress;
            peerPort = senderPort;
        } else if (mode_ == Mode::Client && (peerAddress != senderAddress || peerPort != senderPort)) {
            // In client mode, ignore packets from other sources
            DBG("DTLS Warning: Received packet from unexpected source");
            return 0;
        }
        
        // Feed to BIO
        BIO_write(readBio, tempBuffer.data(), bytesRead);
        
        {
            std::lock_guard<std::mutex> statsLock(statsMutex);
            stats_.bytesReceived += bytesRead;
        }
    }
    
    // Decrypt
    int decryptedBytes = SSL_read(ssl, buffer, bufferSize);
    
    if (decryptedBytes > 0) {
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats_.packetsReceived++;
    } else {
        int err = SSL_get_error(ssl, decryptedBytes);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_ZERO_RETURN) {
            if (err != SSL_ERROR_SYSCALL || errno != EAGAIN) {
                logOpenSSLErrors("SSL_read failed");
            }
        }
    }
    
    return decryptedBytes > 0 ? decryptedBytes : 0;
}

int DTLSSocket::getMaxPayloadSize() const {
    // Account for IP, UDP, and DTLS overhead
    return mtu_ - UDP_HEADER_SIZE - IP_HEADER_SIZE - 13;  // 13 bytes DTLS record header
}

//==============================================================================
// State Management
//==============================================================================

void DTLSSocket::setFailedState() {
    currentState.store(State::Failed, std::memory_order_release);
}

void DTLSSocket::logOpenSSLErrors(const juce::String& message) {
    DBG("DTLS Error: " + message);
    
    unsigned long errCode;
    char errBuf[256];
    while ((errCode = ERR_get_error()) != 0) {
        ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
        DBG("  OpenSSL: " + juce::String(errBuf));
    }
}

//==============================================================================
// Statistics
//==============================================================================

DTLSSocket::Stats DTLSSocket::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats_;
}

void DTLSSocket::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats_ = Stats{};
}

#endif  // ZENITH_ENABLE_COLLAB

} // namespace zenith
