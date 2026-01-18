#include "EmbeddedMCPHttpServer.h"
#include "../engine/PluginHost.h"
#include "../mcp/EngineMCPExporter.h"
#include "../engine/ZenithLogger.h"

#include <map>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <cstdlib>

namespace zenith {
namespace network {

EmbeddedMCPHttpServer::EmbeddedMCPHttpServer(Engine &engine) noexcept : engine_(engine) {}

EmbeddedMCPHttpServer::~EmbeddedMCPHttpServer() { stop(); }

bool EmbeddedMCPHttpServer::start(int port, const juce::String &host, const juce::String &token) {
    if (running_.load())
        return false;
    shouldStop_.store(false);
    serverThread_ = std::thread(&EmbeddedMCPHttpServer::runServer, this, port, host, token);
    return true;
}

void EmbeddedMCPHttpServer::stop() {
    shouldStop_.store(true);
    try {
        if (serverSocket_)
            serverSocket_->close();
    } catch (...) {}
    if (serverThread_.joinable())
        serverThread_.join();
    running_.store(false);
}

static inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

void EmbeddedMCPHttpServer::runServer(int port, const juce::String &host, const juce::String &token) {
    ZENITH_LOG_INFO("[MCP HTTP] Starting embedded HTTP API on " + juce::String(host) + ":" + juce::String(port));

    serverSocket_.reset(new juce::StreamingSocket());
    if (!serverSocket_->createListener(port, host.toRawUTF8())) {
        ZENITH_LOG_ERROR("[MCP HTTP] Failed to bind to " + juce::String(host) + ":" + juce::String(port));
        running_.store(false);
        return;
    }

    running_.store(true);

    // Optionally enable TLS if MCP_HTTP_CERT / MCP_HTTP_KEY env vars are set (JUCE does not provide OpenSSL here, so we use a simple TLS wrapper if available)
    const char* certEnv = std::getenv("MCP_HTTP_CERT");
    const char* keyEnv = std::getenv("MCP_HTTP_KEY");
    bool useTls = false;
    void* wrapperHandle = nullptr;
    if (certEnv && keyEnv) {
        // Attempt to use the optional cpp-httplib wrapper if it was compiled in.
        ZENITH_LOG_INFO("[MCP HTTP] TLS certificate and key provided; attempting to enable embedded TLS via httplib wrapper.");
        // The C API functions are weakly referenced at runtime; try to dlopen the current binary and resolve the symbols.
        typedef void* (*create_fn_t)(void*, const char*, int, const char*, const char*, const char*);
        typedef bool (*start_fn_t)(void*);
        typedef void (*stop_fn_t)(void*);
        typedef void (*destroy_fn_t)(void*);

        void* handle = nullptr;
        // Try resolving from current process
        handle = dlopen(nullptr, RTLD_NOW | RTLD_GLOBAL);
        if (handle) {
            create_fn_t create_fn = (create_fn_t)dlsym(handle, "zenith_network_create_http_wrapper");
            start_fn_t start_fn = (start_fn_t)dlsym(handle, "zenith_network_start_http_wrapper");
            stop_fn_t stop_fn = (stop_fn_t)dlsym(handle, "zenith_network_stop_http_wrapper");
            destroy_fn_t destroy_fn = (destroy_fn_t)dlsym(handle, "zenith_network_destroy_http_wrapper");
            if (create_fn && start_fn && stop_fn && destroy_fn) {
                std::string hostStr = host.toStdString();
                std::string certStr = certEnv ? std::string(certEnv) : std::string();
                std::string keyStr = keyEnv ? std::string(keyEnv) : std::string();
                std::string tokenStr = token.toStdString();
                wrapperHandle = create_fn(reinterpret_cast<void*>(&engine_), hostStr.c_str(), port, certStr.c_str(), keyStr.c_str(), tokenStr.c_str());
                if (wrapperHandle) {
                    if (start_fn(wrapperHandle)) {
                        ZENITH_LOG_INFO("[MCP HTTP] Started TLS-enabled httplib wrapper on " + juce::String(host) + ":" + juce::String(port));
                        useTls = true;
                    } else {
                        ZENITH_LOG_WARN("[MCP HTTP] httplib wrapper failed to start");
                        destroy_fn(wrapperHandle);
                        wrapperHandle = nullptr;
                    }
                }
            } else {
                ZENITH_LOG_INFO("[MCP HTTP] httplib wrapper symbols not available in binary");
            }
            dlclose(handle);
        }
    }

    while (!shouldStop_.load()) {
        // Wait for an incoming connection (500ms poll)
        int ready = serverSocket_->waitUntilReady(true, 500);
        if (shouldStop_.load()) break;
        if (ready != 1) continue;

        std::unique_ptr<juce::StreamingSocket> client(serverSocket_->waitForNextConnection());
        if (!client) continue;

        // Read request headers (basic, stop at CRLFCRLF)
        std::string accum;
        char buf[2048];
        int totalRead = 0;
        while (true) {
            int r = client->read(buf, sizeof(buf) - 1, false);
            if (r <= 0) break;
            buf[r] = '\0';
            accum += buf;
            totalRead += r;
            if (accum.find("\r\n\r\n") != std::string::npos) break;
            if (totalRead > 64 * 1024) break;
        }

        if (accum.empty()) { client->close(); continue; }

        // Parse request line
        size_t lineEnd = accum.find("\r\n");
        if (lineEnd == std::string::npos) { client->close(); continue; }
        std::string requestLine = accum.substr(0, lineEnd);
        std::istringstream rl(requestLine);
        std::string method, path;
        rl >> method >> path;

        // Parse headers
        std::map<std::string, std::string> headers;
        size_t pos = lineEnd + 2;
        while (pos < accum.size()) {
            size_t next = accum.find("\r\n", pos);
            if (next == std::string::npos) break;
            if (next == pos) break; // empty line
            std::string hdr = accum.substr(pos, next - pos);
            pos = next + 2;
            size_t colon = hdr.find(":");
            if (colon != std::string::npos) {
                std::string key = toLower(hdr.substr(0, colon));
                std::string val = hdr.substr(colon + 1);
                // trim
                while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(0, 1);
                while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) val.pop_back();
                headers[key] = val;
            }
        }

        // Auth check
        std::string requiredToken = token.toStdString();
        if (!requiredToken.empty()) {
            std::string provided;
            auto it = headers.find("x-mcp-token");
            if (it != headers.end()) provided = it->second;
            else if ((it = headers.find("authorization")) != headers.end()) {
                std::string v = it->second;
                const std::string bearer = "Bearer ";
                if (v.rfind(bearer, 0) == 0) provided = v.substr(bearer.size());
                else provided = v;
            }
            if (provided != requiredToken) {
                std::string resp = "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                client->write(resp.c_str(), (int)resp.size());
                client->close();
                continue;
            }
        }

        juce::String jsonText;

        if (path == "/mcp/metrics" || path == "/mcp/metrics/") {
            // Fetch snapshot on message thread (synchronous via condvar)
            juce::var snapshotVar;
            std::mutex m;
            std::condition_variable cv;
            bool done = false;

            juce::MessageManager::callAsync([&]() {
                try {
                    zenith::mcp::EngineMCPExporter exp(engine_);
                    snapshotVar = exp.getMeteringSnapshot();
                } catch (...) {
                    snapshotVar = juce::var();
                }
                {
                    std::lock_guard<std::mutex> lk(m);
                    done = true;
                }
                cv.notify_one();
            });

            std::unique_lock<std::mutex> lk(m);
            if (!cv.wait_for(lk, std::chrono::milliseconds(300), [&]{ return done; })) {
                std::string resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                client->write(resp.c_str(), (int)resp.size());
                client->close();
                continue;
            }
            jsonText = juce::JSON::toString(snapshotVar, false);
        } else if (path == "/mcp/plugins" || path == "/mcp/plugins/") {
            juce::var pluginVar;
            std::mutex m;
            std::condition_variable cv;
            bool done = false;

            juce::MessageManager::callAsync([&]() {
                try {
                    zenith::mcp::EngineMCPExporter exp(engine_);
                    pluginVar = exp.getPluginList();
                } catch (...) {
                    pluginVar = juce::var();
                }
                {
                    std::lock_guard<std::mutex> lk(m);
                    done = true;
                }
                cv.notify_one();
            });

            std::unique_lock<std::mutex> lk(m);
            if (!cv.wait_for(lk, std::chrono::milliseconds(300), [&]{ return done; })) {
                std::string resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                client->write(resp.c_str(), (int)resp.size());
                client->close();
                continue;
            }
            jsonText = juce::JSON::toString(pluginVar, false);
        } else if (path == "/mcp/snapshot" || path == "/mcp/snapshot/") {
            juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile("tools/mcp_server/engine_snapshot.json");
            if (f.existsAsFile()) {
                jsonText = f.loadFileAsString();
            } else {
                std::string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                client->write(resp.c_str(), (int)resp.size());
                client->close();
                continue;
            }
        } else {
            std::string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            client->write(resp.c_str(), (int)resp.size());
            client->close();
            continue;
        }

        // Send response
        int bodyLen = (int)jsonText.getNumBytesAsUTF8();
        std::string respHeaders = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(bodyLen) + "\r\nConnection: close\r\n\r\n";
        client->write(respHeaders.c_str(), (int)respHeaders.size());
        client->write(jsonText.toRawUTF8(), bodyLen);
        client->close();
    }

    try { serverSocket_->close(); } catch (...) {}

    // If we started an httplib wrapper, ensure it's stopped/destroyed
    if (wrapperHandle) {
        void* h = dlopen(nullptr, RTLD_NOW | RTLD_GLOBAL);
        if (h) {
            typedef void (*stop_fn_t)(void*);
            typedef void (*destroy_fn_t)(void*);
            stop_fn_t stop_fn = (stop_fn_t)dlsym(h, "zenith_network_stop_http_wrapper");
            destroy_fn_t destroy_fn = (destroy_fn_t)dlsym(h, "zenith_network_destroy_http_wrapper");
            if (stop_fn) stop_fn(wrapperHandle);
            if (destroy_fn) destroy_fn(wrapperHandle);
            dlclose(h);
        }
        wrapperHandle = nullptr;
    }

    running_.store(false);
    ZENITH_LOG_INFO("[MCP HTTP] Embedded HTTP API stopped");
}

} // namespace network
} // namespace zenith
