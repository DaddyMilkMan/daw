#ifdef ENABLE_CPP_HTTP_LIB

#include "EmbeddedMCPHttpServer.h"
#include <httplib.h>
#include <iostream>
#include <fstream>

namespace zenith {
namespace network {

class HTTPLibServerWrapper {
public:
    HTTPLibServerWrapper(Engine &engine, const std::string &host, int port, const std::string &certPath, const std::string &keyPath, const std::string &token)
        : engine_(engine), host_(host), port_(port), cert_(certPath), key_(keyPath), token_(token) {}

    bool start() {
        if (running_) return false;
        if (!cert_.empty() && !key_.empty()) {
            svr_ = std::make_unique<httplib::SSLServer>(cert_.c_str(), key_.c_str());
        } else {
            svr_ = std::make_unique<httplib::Server>();
        }
        if (!svr_) return false;

        // Agents endpoint (mirror Python server style)
        svr_->Get("/agents", [&](const httplib::Request &req, httplib::Response &res) {
            // Return static agents list (reuse Python AGENTS may be added)
            res.set_content("{\"agents\":[]}", "application/json");
        });

        // Embedded endpoints
        svr_->Get(R"(/mcp/metrics)", [&](const httplib::Request &req, httplib::Response &res){
            // use EngineMCPExporter on message thread would be ideal; here for demo we'll read snapshot file
            std::ifstream f("tools/mcp_server/engine_snapshot.json");
            if (!f) { res.status = 404; return; }
            std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            res.set_content(s, "application/json");
        });

        svr_->Get(R"(/mcp/plugins)", [&](const httplib::Request &req, httplib::Response &res){
            std::ifstream f("tools/mcp_server/engine_snapshot.json");
            if (!f) { res.set_content("[]", "application/json"); return; }
            res.set_content("[]", "application/json");
        });

        svr_->Get(R"(/mcp/snapshot)", [&](const httplib::Request &req, httplib::Response &res){
            std::ifstream f("tools/mcp_server/engine_snapshot.json");
            if (!f) { res.status = 404; return; }
            std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            res.set_content(s, "application/json");
        });

        svr_->set_error_handler([](const httplib::Request &req, httplib::Response &res){ res.status = 500; });

        server_thread_ = std::thread([this](){ this->svr_->listen(this->host_.c_str(), this->port_); });
        // Wait briefly to allow binding
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        running_ = true;
        return true;
    }

    void stop() {
        if (!running_) return;
        if (svr_) svr_->stop();
        if (server_thread_.joinable()) server_thread_.join();
        running_ = false;
    }

private:
    Engine &engine_;
    std::string host_;
    int port_;
    std::string cert_, key_, token_;
    std::unique_ptr<httplib::Server> svr_;
    std::thread server_thread_;
    bool running_ = false;
};

} // namespace network
} // namespace zenith


// Expose a small C API so EmbeddedMCPHttpServer can optionally create and control the wrapper
extern "C" {
    void* zenith_network_create_http_wrapper(void* enginePtr, const char* host, int port, const char* cert, const char* key, const char* token) {
        if (!enginePtr) return nullptr;
        try {
            Engine* e = reinterpret_cast<Engine*>(enginePtr);
            auto* w = new zenith::network::HTTPLibServerWrapper(*e, std::string(host ? host : "127.0.0.1"), port,
                                                                  std::string(cert ? cert : ""), std::string(key ? key : ""), std::string(token ? token : ""));
            return reinterpret_cast<void*>(w);
        } catch (...) {
            return nullptr;
        }
    }

    bool zenith_network_start_http_wrapper(void* wrapper) {
        if (!wrapper) return false;
        try {
            auto* w = reinterpret_cast<zenith::network::HTTPLibServerWrapper*>(wrapper);
            return w->start();
        } catch (...) { return false; }
    }

    void zenith_network_stop_http_wrapper(void* wrapper) {
        if (!wrapper) return;
        try {
            auto* w = reinterpret_cast<zenith::network::HTTPLibServerWrapper*>(wrapper);
            w->stop();
        } catch (...) {}
    }

    void zenith_network_destroy_http_wrapper(void* wrapper) {
        if (!wrapper) return;
        try { delete reinterpret_cast<zenith::network::HTTPLibServerWrapper*>(wrapper); } catch (...) {}
    }
}

#endif // ENABLE_CPP_HTTP_LIB
