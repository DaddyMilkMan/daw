#!/usr/bin/env python3
"""
MCP stdio filesystem server (full access) for a specific root directory.

WARNING: This enables read/write/delete within the root. Use only on trusted
machines and keep it local.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path


PROTOCOL_VERSION = "2024-11-05"
SERVER_NAME = "zenith-fs-stdio"
SERVER_VERSION = "1.0.0"


def _send(obj: dict) -> None:
    sys.stdout.write(json.dumps(obj) + "\n")
    sys.stdout.flush()


def _send_error(req_id, code: int, message: str) -> None:
    _send({"jsonrpc": "2.0", "id": req_id, "error": {"code": code, "message": message}})


def _read_params(msg: dict) -> dict:
    params = msg.get("params")
    if not isinstance(params, dict):
        return {}
    return params


def _resolve(root: Path, subpath: str | None) -> Path:
    target = (root / (subpath or ".")).resolve()
    if not str(target).startswith(str(root)):
        raise PermissionError("Path escapes root")
    return target


def _list_dir(root: Path, subpath: str | None) -> str:
    target = _resolve(root, subpath)
    if not target.is_dir():
        raise NotADirectoryError(str(target))
    entries = []
    for entry in sorted(target.iterdir(), key=lambda p: (p.is_file(), p.name.lower())):
        entries.append(entry.name + ("/" if entry.is_dir() else ""))
    return "\n".join(entries)


def _read_file(root: Path, subpath: str) -> str:
    target = _resolve(root, subpath)
    if not target.is_file():
        raise FileNotFoundError(str(target))
    return target.read_text(encoding="utf-8", errors="replace")


def _write_file(root: Path, subpath: str, content: str) -> str:
    target = _resolve(root, subpath)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8")
    return "ok"


def _delete_path(root: Path, subpath: str) -> str:
    target = _resolve(root, subpath)
    if target.is_dir():
        for child in target.rglob("*"):
            if child.is_file():
                child.unlink()
        for child in sorted(target.rglob("*"), reverse=True):
            if child.is_dir():
                child.rmdir()
        target.rmdir()
    else:
        target.unlink()
    return "ok"


def _make_tools():
    return [
        {
            "name": "list_directory",
            "description": "List entries in a directory (relative to root).",
            "inputSchema": {
                "type": "object",
                "properties": {"path": {"type": "string"}},
            },
        },
        {
            "name": "read_file",
            "description": "Read a text file (relative to root).",
            "inputSchema": {
                "type": "object",
                "properties": {"path": {"type": "string"}},
                "required": ["path"],
            },
        },
        {
            "name": "write_file",
            "description": "Write a text file (relative to root).",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "path": {"type": "string"},
                    "content": {"type": "string"},
                },
                "required": ["path", "content"],
            },
        },
        {
            "name": "delete_path",
            "description": "Delete a file or directory (relative to root).",
            "inputSchema": {
                "type": "object",
                "properties": {"path": {"type": "string"}},
                "required": ["path"],
            },
        },
    ]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=os.environ.get("MCP_FS_ROOT", ""))
    args = parser.parse_args()

    if not args.root:
        sys.stderr.write("Missing --root or MCP_FS_ROOT\n")
        return 2

    root = Path(args.root).resolve()
    if not root.exists():
        sys.stderr.write(f"Root does not exist: {root}\n")
        return 2

    initialized = False

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
        params = _read_params(msg)

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
            _send({"jsonrpc": "2.0", "id": req_id, "result": {"tools": _make_tools()}})
            continue

        if method == "tools/call":
            name = params.get("name", "")
            args = params.get("arguments", {}) if isinstance(params.get("arguments"), dict) else {}
            try:
                if name == "list_directory":
                    text = _list_dir(root, args.get("path"))
                    _send({"jsonrpc": "2.0", "id": req_id, "result": {"content": [{"type": "text", "text": text}]}})
                elif name == "read_file":
                    text = _read_file(root, args.get("path", ""))
                    _send({"jsonrpc": "2.0", "id": req_id, "result": {"content": [{"type": "text", "text": text}]}})
                elif name == "write_file":
                    status = _write_file(root, args.get("path", ""), args.get("content", ""))
                    _send({"jsonrpc": "2.0", "id": req_id, "result": {"content": [{"type": "text", "text": status}]}})
                elif name == "delete_path":
                    status = _delete_path(root, args.get("path", ""))
                    _send({"jsonrpc": "2.0", "id": req_id, "result": {"content": [{"type": "text", "text": status}]}})
                else:
                    _send_error(req_id, -32002, f"Tool not found: {name}")
            except Exception as e:
                _send_error(req_id, -32003, f"Tool failed: {e}")
            continue

        _send_error(req_id, -32601, f"Method not found: {method}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
