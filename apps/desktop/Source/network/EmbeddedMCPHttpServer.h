/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <juce_core/juce_core.h>
#include "../engine/Engine.h"

namespace zenith::network {

/**
 * @brief HTTP server for embedded MCP (Model Context Protocol) interface
 *
 * Provides an HTTP-based JSON-RPC interface to MCP functionality.
 * Wraps the stdio-based MCPServer with an HTTP layer.
 */
class EmbeddedMCPHttpServer : private juce::Thread {
public:
    EmbeddedMCPHttpServer(Engine& engine);
    ~EmbeddedMCPHttpServer() override;

    /**
     * @brief Start the HTTP server on specified port
     * @param port Port number to listen on
     * @param host Host interface to bind to
     * @param path URL path prefix for MCP endpoints
     * @return true if server started successfully
     */
    bool start(int port, const juce::String& host = "127.0.0.1", const juce::String& path = "/mcp");

    /**
     * @brief Stop the HTTP server
     */
    void stop();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return running_.load(); }

private:
    void run() override;

    Engine& engine_;
    std::atomic<bool> running_{false};
    int serverPort_ = 0;
    juce::String host_;
    juce::String basePath_;

    void handleConnection(juce::StreamingSocket* socket);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmbeddedMCPHttpServer)
};

} // namespace zenith::network
