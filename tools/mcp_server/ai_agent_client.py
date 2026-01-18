#!/usr/bin/env python3
"""
ai_agent_client.py

Lightweight helper for AI agents to discover and interact with local MCP servers (embedded C++ or Python).
Provides discovery, agent invocation, UI actions and snapshot/metrics helpers.

This module prefers 'requests' if available and falls back to urllib.
"""

from __future__ import annotations
import os
import json
import socket
import time
from typing import Optional, Tuple, Dict, Any

ENV_FILE = "/tmp/zenith_mcp_env"

try:
    import requests
    _HAS_REQUESTS = True
except Exception:
    import urllib.request as _urllib
    _HAS_REQUESTS = False


def _probe_port(port: int, timeout: float = 0.2) -> bool:
    try:
        with socket.create_connection(("127.0.0.1", port), timeout=timeout):
            return True
    except Exception:
        return False


def discover() -> Tuple[Optional[str], Optional[str]]:
    """Discover a local MCP HTTP server.

    Returns (base_url, token) or (None, None) if not found.
    """
    # 1) check launcher env file
    if os.path.exists(ENV_FILE):
        data = {}
        with open(ENV_FILE, 'r') as f:
            for line in f:
                if '=' in line:
                    k, v = line.strip().split('=', 1)
                    data[k] = v
        if 'MCP_HTTP_PORT' in data:
            port = data['MCP_HTTP_PORT']
            token = data.get('MCP_HTTP_TOKEN')
            return (f"http://127.0.0.1:{port}", token)

    # 2) probe ephemeral and common ports
    ports = list(range(49152, 49163)) + [8090, 8008, 8443]
    for p in ports:
        if _probe_port(p):
            scheme = 'https' if p == 8443 else 'http'
            token = os.environ.get('MCP_HTTP_TOKEN') or os.environ.get('MCP_SERVER_TOKEN')
            return (f"{scheme}://127.0.0.1:{p}", token)
    return (None, None)


def _get(url: str, token: Optional[str] = None, timeout: float = 5.0, verify: bool = True) -> Any:
    headers = {'X-MCP-Token': token} if token else {}
    if _HAS_REQUESTS:
        r = requests.get(url, headers=headers, timeout=timeout, verify=verify)
        r.raise_for_status()
        return r.json()
    else:
        req = _urllib.Request(url, headers=headers)
        with _urllib.urlopen(req, timeout=timeout) as resp:
            data = resp.read()
            return json.loads(data.decode('utf-8'))


def _post(url: str, payload: Dict[str, Any], token: Optional[str] = None, timeout: float = 10.0, verify: bool = True) -> Any:
    headers = {'X-MCP-Token': token, 'Content-Type': 'application/json'} if token else {'Content-Type': 'application/json'}
    data = json.dumps(payload).encode('utf-8')
    if _HAS_REQUESTS:
        r = requests.post(url, headers=headers, data=data, timeout=timeout, verify=verify)
        r.raise_for_status()
        return r.json()
    else:
        req = _urllib.Request(url, data=data, headers=headers)
        with _urllib.urlopen(req, timeout=timeout) as resp:
            data = resp.read()
            return json.loads(data.decode('utf-8'))


# High-level helpers

def get_agents(base_url: Optional[str] = None, token: Optional[str] = None):
    if base_url is None:
        base_url, token = discover()
        if base_url is None:
            raise RuntimeError('MCP server not found')
    return _get(base_url + '/agents', token=token)


def invoke_agent(agent_id: str, action: str, params: Optional[Dict[str, Any]] = None, base_url: Optional[str] = None, token: Optional[str] = None):
    if base_url is None:
        base_url, token = discover()
        if base_url is None:
            raise RuntimeError('MCP server not found')
    payload = {'action': action, 'params': params or {}}
    return _post(f"{base_url}/agents/{agent_id}/invoke", payload, token=token)


def get_ui_state(base_url: Optional[str] = None, token: Optional[str] = None):
    if base_url is None:
        base_url, token = discover()
        if base_url is None:
            raise RuntimeError('MCP server not found')
    return _get(base_url + '/ui/state', token=token)


def press_button(button_id: str, action: str = 'click', base_url: Optional[str] = None, token: Optional[str] = None):
    if base_url is None:
        base_url, token = discover()
        if base_url is None:
            raise RuntimeError('MCP server not found')
    payload = {'id': button_id, 'action': action}
    return _post(base_url + '/ui/button', payload, token=token)


def get_metrics(base_url: Optional[str] = None, token: Optional[str] = None):
    if base_url is None:
        base_url, token = discover()
        if base_url is None:
            raise RuntimeError('MCP server not found')
    return _get(base_url + '/mcp/metrics', token=token, verify=False)


def get_plugins(base_url: Optional[str] = None, token: Optional[str] = None):
    if base_url is None:
        base_url, token = discover()
        if base_url is None:
            raise RuntimeError('MCP server not found')
    return _get(base_url + '/mcp/plugins', token=token, verify=False)


def get_snapshot(base_url: Optional[str] = None, token: Optional[str] = None):
    if base_url is None:
        base_url, token = discover()
        if base_url is None:
            raise RuntimeError('MCP server not found')
    # snapshot may be raw JSON text
    if _HAS_REQUESTS:
        r = requests.get(base_url + '/mcp/snapshot', headers={'X-MCP-Token': token} if token else {}, timeout=5, verify=False)
        r.raise_for_status()
        try:
            return r.json()
        except Exception:
            return r.text
    else:
        req = _urllib.Request(base_url + '/mcp/snapshot', headers={'X-MCP-Token': token} if token else {})
        with _urllib.urlopen(req, timeout=5) as resp:
            data = resp.read()
            try:
                return json.loads(data.decode('utf-8'))
            except Exception:
                return data.decode('utf-8')


if __name__ == '__main__':
    # simple CLI demo for AI agents or debugging
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument('--discover', action='store_true')
    p.add_argument('--agents', action='store_true')
    p.add_argument('--metrics', action='store_true')
    args = p.parse_args()
    if args.discover:
        print(discover())
    elif args.agents:
        print(json.dumps(get_agents(), indent=2))
    elif args.metrics:
        print(json.dumps(get_metrics(), indent=2))
    else:
        print('Use --agents or --metrics')
