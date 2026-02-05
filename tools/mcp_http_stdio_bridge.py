#!/usr/bin/env python3
"""
Minimal MCP stdio server that bridges to Zenith's embedded HTTP MCP endpoints.

Use when the DAW's stdio MCP server isn't available, but the embedded HTTP
server is running. This provides:
  - initialize / notifications/initialized
  - resources/list and resources/read mapped to /mcp/* HTTP endpoints
  - tools/list returns an empty list (no tool execution via HTTP)
"""

import argparse
import json
import os
import sys
import urllib.request
import urllib.error


PROTOCOL_VERSION = "2024-11-05"
SERVER_NAME = "zenith-http-bridge"
SERVER_VERSION = "0.1.0"


def _log(msg: str) -> None:
    sys.stderr.write(msg + "\n")
    sys.stderr.flush()


def _http_get(url: str, token: str | None) -> str:
    headers = {}
    if token:
        headers["X-MCP-Token"] = token
    req = urllib.request.Request(url, headers=headers, method="GET")
    with urllib.request.urlopen(req, timeout=3) as resp:
        return resp.read().decode("utf-8")


def _send(obj: dict) -> None:
    sys.stdout.write(json.dumps(obj) + "\n")
    sys.stdout.flush()


def _send_error(req_id, code: int, message: str) -> None:
    _send({"jsonrpc": "2.0", "id": req_id, "error": {"code": code, "message": message}})


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default=os.environ.get("MCP_HTTP_URL", "").strip())
    parser.add_argument("--port", type=int, default=int(os.environ.get("MCP_HTTP_PORT", "0") or "0"))
    parser.add_argument("--host", default=os.environ.get("MCP_HTTP_HOST", "127.0.0.1"))
    parser.add_argument("--token", default=os.environ.get("MCP_HTTP_TOKEN") or os.environ.get("MCP_SERVER_TOKEN") or "")
    args = parser.parse_args()

    base_url = args.url
    if not base_url:
        if args.port <= 0:
            _log("Missing HTTP target: set --url or MCP_HTTP_PORT.")
            return 2
        base_url = f"http://{args.host}:{args.port}"

    token = args.token or None
    initialized = False

    _log(f"[bridge] MCP HTTP base: {base_url}")

    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            _send_error(None, -32700, "Parse error")
            continue

        req_id = msg.get("id")
        method = msg.get("method")
        params = msg.get("params") or {}

        if not method:
            _send_error(req_id, -32600, "Invalid request: missing method")
            continue

        if method == "initialize":
            result = {
                "protocolVersion": PROTOCOL_VERSION,
                "capabilities": {"tools": {}, "resources": {}},
                "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
            }
            _send({"jsonrpc": "2.0", "id": req_id, "result": result})
            continue

        if method == "notifications/initialized":
            initialized = True
            continue

        if not initialized and method != "ping":
            _send_error(req_id, -32002, "Server not initialized")
            continue

        if method == "ping":
            _send({"jsonrpc": "2.0", "id": req_id, "result": {}})
            continue

        if method == "tools/list":
            _send({"jsonrpc": "2.0", "id": req_id, "result": {"tools": []}})
            continue

        if method == "tools/call":
            _send_error(req_id, -32002, "Tool not available via HTTP bridge")
            continue

        if method == "resources/list":
            resources = [
                {"uri": "zenith://metrics", "name": "Metrics Snapshot", "mimeType": "application/json"},
                {"uri": "zenith://plugins", "name": "Plugin List", "mimeType": "application/json"},
                {"uri": "zenith://snapshot", "name": "Engine Snapshot", "mimeType": "application/json"},
            ]
            _send({"jsonrpc": "2.0", "id": req_id, "result": {"resources": resources}})
            continue

        if method == "resources/read":
            uri = params.get("uri", "")
            if uri == "zenith://metrics":
                path = "/mcp/metrics"
            elif uri == "zenith://plugins":
                path = "/mcp/plugins"
            elif uri == "zenith://snapshot":
                path = "/mcp/snapshot"
            else:
                _send_error(req_id, -32001, "Resource not found")
                continue
            try:
                body = _http_get(base_url + path, token)
                result = {"contents": [{"uri": uri, "mimeType": "application/json", "text": body}]}
                _send({"jsonrpc": "2.0", "id": req_id, "result": result})
            except urllib.error.HTTPError as e:
                _send_error(req_id, -32001, f"HTTP error {e.code}")
            except Exception as e:
                _send_error(req_id, -32603, f"HTTP failure: {e}")
            continue

        _send_error(req_id, -32601, f"Method not found: {method}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
