#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
// #include <juce_networking/juce_networking.h>  // Temporarily disabled - module not available
#include "../engine/Engine.h"

namespace zenith {
namespace network {

/**
 * @brief Small embedded HTTP server exposing minimal MCP read-only endpoints.
 *
 * Endpoints:
 *  - GET /mcp/metrics  -> JSON metering snapshot (master + per-track levels)
 *  - GET /mcp/plugins  -> JSON plugin list
 *  - GET /mcp/snapshot -> contents of tools/mcp_server/engine_snapshot.json (if present)
 *
 * Token auth: set MCP_HTTP_TOKEN or MCP_SERVER_TOKEN env var to require X-MCP-Token or
 * Authorization: Bearer <token> header.
 */
class EmbeddedMCPHttpServer {
public:
    explicit EmbeddedMCPHttpServer(Engine &engine) noexcept;
    ~EmbeddedMCPHttpServer();

    // Start server on `host:port`. Returns true if listener was created.
    bool start(int port = 8088, const juce::String &host = "127.0.0.1", const juce::String &token = juce::String());
    void stop();
    bool isRunning() const noexcept { return running_.load(); }

private:
    void runServer(int port, const juce::String &host, const juce::String &token);

    Engine &engine_;
    std::unique_ptr<juce::StreamingSocket> serverSocket_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shouldStop_{false};
    std::thread serverThread_;

    // TLS support (opaque pointer to avoid requiring OpenSSL in header)
    void* sslCtx_{nullptr};
    int tlsListenFd_{-1};
    bool useTls_{false};
};

} // namespace network
} // namespace zenith
