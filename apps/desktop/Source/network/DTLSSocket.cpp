#include "DTLSSocket.h"
#include <vector>
#include <algorithm>

namespace zenith {

#ifdef ZENITH_ENABLE_COLLAB
#include <openssl/err.h>

// Certificate validation callback
static int verify_certificate_callback(int preverify_ok, X509_STORE_CTX* ctx) {
    // Get the SSL object associated with this connection
    SSL* ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    DTLSSocket* socket = (DTLSSocket*)SSL_get_ex_data(ssl, 0);

    if (!socket) {
        DBG("DTLS: No socket context in verify callback");
        return 0;  // Reject - no context
    }

    // In production, we require preverify_ok (certificate chain validated)
    // For development, we can be more lenient if self-signed certs are used
#ifndef DEBUG
    if (!preverify_ok) {
        char buf[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx);
        int err = X509_STORE_CTX_get_error(ctx);
        int depth = X509_STORE_CTX_get_error_depth(ctx);

        X509_NAME_oneline(X509_get_subject_name(cert), buf, 256);
        DBG("DTLS: Certificate verification failed at depth " + juce::String(depth) + ": " + buf);
        DBG("DTLS: Error: " + juce::String(X509_verify_cert_error_string(err)));

        return 0;  // Reject invalid certificates in production
    }
#endif

    // Additional verification: check certificate expiration
    X509* cert = X509_STORE_CTX_get_current_cert(ctx);
    if (cert) {
        time_t now = time(nullptr);
        if (X509_cmp_time(X509_get0_notBefore(cert), &now) > 0) {
            DBG("DTLS: Certificate not yet valid");
            return 0;
        }
        if (X509_cmp_time(X509_get0_notAfter(cert), &now) < 0) {
            DBG("DTLS: Certificate expired");
            return 0;
        }
    }

    return 1;  // Accept certificate
}

// Helper to load self-signed certs (for development)
void configure_ssl_context_with_self_signed(SSL_CTX* ctx) {
    auto certFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory().getChildFile("cert.pem");
    auto keyFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory().getChildFile("key.pem");

    if (!certFile.existsAsFile() || !keyFile.existsAsFile()) {
        DBG("DTLS: cert.pem or key.pem not found. Will use peer certificate verification only.");
        // Don't return - continue to set up verification
    } else {
        if (SSL_CTX_use_certificate_file(ctx, certFile.getFullPathName().toRawUTF8(), SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            DBG("DTLS: Failed to load certificate file");
            return;
        }

        if (SSL_CTX_use_PrivateKey_file(ctx, keyFile.getFullPathName().toRawUTF8(), SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            DBG("DTLS: Failed to load private key file");
            return;
        }

        if (!SSL_CTX_check_private_key(ctx)) {
            DBG("DTLS Error: Private key does not match the public certificate");
            return;
        }
    }

    // Set proper verification mode with callback
    // SSL_VERIFY_PEER: Request peer certificate
    // SSL_VERIFY_FAIL_IF_NO_PEER_CERT: Fail if peer doesn't provide cert (for server-side)
    // For client-side DTLS, we use VERIFY_PEER
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_certificate_callback);

    // Load default CA certificates for production use
    if (!SSL_CTX_set_default_verify_paths(ctx)) {
        DBG("DTLS: Warning - Could not load default CA certificates");
        // Continue anyway - we may be using self-signed certs
    }

    DBG("DTLS: SSL context configured with certificate verification");
}


DTLSSocket::DTLSSocket() {
    initializeSSLContext();
}

DTLSSocket::~DTLSSocket() {
    shutdown();
}

void DTLSSocket::initializeSSLContext() {
    if (sslContext) return;
    sslContext = SSL_CTX_new(DTLS_client_method());
    if (!sslContext) {
        logOpenSSLErrors("Failed to create SSL_CTX");
        currentState = State::Failed;
        return;
    }
    configure_ssl_context_with_self_signed(sslContext);
}

void DTLSSocket::freeSSL() {
    if (ssl) {
        SSL_free(ssl);
        ssl = nullptr;
        readBio = nullptr; 
        writeBio = nullptr;
    }
}

void DTLSSocket::shutdown() {
    freeSSL();
    if (sslContext) {
        SSL_CTX_free(sslContext);
        sslContext = nullptr;
    }
    udpSocket.shutdown();
    currentState = State::Disconnected;
}

bool DTLSSocket::bind(int localPort) {
    // juce::DatagramSocket::bindToPort is the correct method
    return udpSocket.bindToPort(localPort);
}

bool DTLSSocket::connect(const juce::String& peerAddr, int pPort) {
    if (currentState != State::Disconnected && currentState != State::Failed) {
        return false;
    }
    if (!sslContext) {
        return false;
    }

    peerAddress = peerAddr;
    peerPort = pPort;

    freeSSL();

    ssl = SSL_new(sslContext);
    if (!ssl) {
        currentState = State::Failed;
        return false;
    }

    // Store 'this' pointer in SSL ex_data for verify callback
    SSL_set_ex_data(ssl, 0, this);

    readBio = BIO_new(BIO_s_mem());
    writeBio = BIO_new(BIO_s_mem());
    SSL_set_bio(ssl, readBio, writeBio);

    SSL_set_connect_state(ssl);

    currentState = State::Handshaking;
    return performHandshake();
}

bool DTLSSocket::performHandshake() {
    if (currentState != State::Handshaking) {
        return (currentState == State::Connected);
    }

    // Use dynamic allocation to prevent stack overflow with large packets
    // DTLS packets can exceed MTU due to fragmentation
    std::vector<char> buffer(2048);

    // Maximum iterations to prevent infinite loop
    // Each iteration processes one handshake step
    const int MAX_HANDSHAKE_ITERATIONS = 100;
    int iterations = 0;

    while (iterations < MAX_HANDSHAKE_ITERATIONS && currentState == State::Handshaking) {
        int ret = SSL_do_handshake(ssl);

        if (ret == 1) {
            currentState = State::Connected;
            return true;
        }

        int err = SSL_get_error(ssl, ret);
        if (err == SSL_ERROR_WANT_READ) {
            int pending = BIO_ctrl_pending(writeBio);
            if (pending > 0) {
                // Cap write to buffer size
                int bytesToWrite = std::min(pending, static_cast<int>(buffer.size()));
                int bytes_written = BIO_read(writeBio, buffer.data(), bytesToWrite);
                if (bytes_written > 0) {
                    udpSocket.write(peerAddress, peerPort, buffer.data(), bytes_written);
                }
            }

            int bytes_read = udpSocket.read(buffer.data(), buffer.size(), false);
            if (bytes_read > 0) {
                BIO_write(readBio, buffer.data(), bytes_read);
                // Continue loop instead of recursing
                iterations++;
                continue;
            }

            // No data available yet, return false to try again later
            return false;
        } else {
            logOpenSSLErrors("DTLS Handshake Failed");
            currentState = State::Failed;
            return false;
        }

        iterations++;
    }

    // Either handshake completed or hit max iterations
    return (currentState == State::Connected);
}


int DTLSSocket::write(const void* data, int bytes) {
    if (currentState != State::Connected) {
        return -1;
    }

    int bytesWritten = SSL_write(ssl, data, bytes);
    if (bytesWritten <= 0) {
        return -1;
    }

    // Use dynamic allocation to prevent stack overflow
    std::vector<char> buffer(2048);
    int pending = BIO_ctrl_pending(writeBio);
    if (pending > 0) {
        // Cap write to buffer size
        int toSend = std::min(pending, static_cast<int>(buffer.size()));
        int bytesRead = BIO_read(writeBio, buffer.data(), toSend);
        if (bytesRead > 0) {
            if (udpSocket.write(peerAddress, peerPort, buffer.data(), bytesRead) < 0) {
                return -1;
            }
        }
    }
    return bytesWritten;
}

int DTLSSocket::read(void* buffer, int bufferSize, juce::String& senderAddress, int& senderPort) {
    if (currentState != State::Connected) {
        return 0;
    }

    // Use dynamic allocation to prevent stack overflow
    std::vector<char> tempBuffer(2048);
    int bytesRead = udpSocket.read(tempBuffer.data(), tempBuffer.size(), false, senderAddress, senderPort);

    if (bytesRead > 0) {
        BIO_write(readBio, tempBuffer.data(), bytesRead);

        int decryptedBytes = SSL_read(ssl, buffer, bufferSize);
        if (decryptedBytes > 0) {
            return decryptedBytes;
        }
    }

    return 0;
}


void DTLSSocket::logOpenSSLErrors(const juce::String& message) {
    unsigned long errCode;
    char errBuf[256];
    DBG("DTLS Error: " + message);
    while ((errCode = ERR_get_error()) != 0) {
        ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
        DBG("  " + juce::String(errBuf));
    }
}

#endif

} // namespace zenith
