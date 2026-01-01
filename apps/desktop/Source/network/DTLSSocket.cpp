#include "DTLSSocket.h"
#include <openssl/err.h>

namespace zenith {

#ifdef ZENITH_ENABLE_COLLAB

// Helper to load self-signed certs (for development)
void configure_ssl_context_with_self_signed(SSL_CTX* ctx) {
    auto certFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory().getChildFile("cert.pem");
    auto keyFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory().getChildFile("key.pem");

    if (!certFile.existsAsFile() || !keyFile.existsAsFile()) {
        DBG("DTLS Error: cert.pem or key.pem not found. DTLS will likely fail.");
        return;
    }

    if (SSL_CTX_use_certificate_file(ctx, certFile.getFullPathName().toRawUTF8(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, keyFile.getFullPathName().toRawUTF8(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        DBG("DTLS Error: Private key does not match the public certificate");
        return;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
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

    char buffer[2048];
    int ret = SSL_do_handshake(ssl);

    if (ret == 1) {
        currentState = State::Connected;
        return true;
    }
    
    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ) {
        int pending = BIO_ctrl_pending(writeBio);
        if (pending > 0) {
            int bytes_written = BIO_read(writeBio, buffer, sizeof(buffer));
            if (bytes_written > 0) {
                udpSocket.write(peerAddress, peerPort, buffer, bytes_written);
            }
        }

        int bytes_read = udpSocket.read(buffer, sizeof(buffer), false);
        if (bytes_read > 0) {
            BIO_write(readBio, buffer, bytes_read);
            return performHandshake();
        }
    } else {
        logOpenSSLErrors("DTLS Handshake Failed");
        currentState = State::Failed;
        return false;
    }

    return false; 
}


int DTLSSocket::write(const void* data, int bytes) {
    if (currentState != State::Connected) {
        return -1;
    }

    int bytesWritten = SSL_write(ssl, data, bytes);
    if (bytesWritten <= 0) {
        return -1;
    }

    char buffer[2048];
    int pending = BIO_ctrl_pending(writeBio);
    if (pending > 0) {
        int toSend = BIO_read(writeBio, buffer, sizeof(buffer));
        if (toSend > 0) {
            if (udpSocket.write(peerAddress, peerPort, buffer, toSend) < 0) {
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

    char tempBuffer[2048];
    int bytesRead = udpSocket.read(tempBuffer, sizeof(tempBuffer), false, senderAddress, senderPort);

    if (bytesRead > 0) {
        BIO_write(readBio, tempBuffer, bytesRead);

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
