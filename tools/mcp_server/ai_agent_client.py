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
import ssl
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


def _http_get_raw(url: str, token: Optional[str] = None, timeout: float = 1.0, verify: bool = True):
    headers = {'X-MCP-Token': token} if token else {}
    if _HAS_REQUESTS:
        try:
            r = requests.get(url, headers=headers, timeout=timeout, verify=verify)
            return r.status_code, r
        except Exception as e:
            return None, e
    else:
        try:
            ctx = None
            if url.startswith('https://') and not verify:
                ctx = ssl.create_default_context()
                ctx.check_hostname = False
                ctx.verify_mode = ssl.CERT_NONE
            req = _urllib.Request(url, headers=headers)
            with _urllib.urlopen(req, timeout=timeout, context=ctx) as resp:
                return resp.getcode(), resp.read()
        except Exception as e:
            return None, e


def _http_post_raw(url: str, payload: Dict[str, Any], token: Optional[str] = None, timeout: float = 3.0, verify: bool = True):
    headers = {'X-MCP-Token': token, 'Content-Type': 'application/json'} if token else {'Content-Type': 'application/json'}
    data = json.dumps(payload).encode('utf-8')
    if _HAS_REQUESTS:
        try:
            r = requests.post(url, headers=headers, data=data, timeout=timeout, verify=verify)
            return r.status_code, r
        except Exception as e:
            return None, e
    else:
        try:
            ctx = None
            if url.startswith('https://') and not verify:
                ctx = ssl.create_default_context()
                ctx.check_hostname = False
                ctx.verify_mode = ssl.CERT_NONE
            req = _urllib.Request(url, data=data, headers=headers)
            with _urllib.urlopen(req, timeout=timeout, context=ctx) as resp:
                return resp.getcode(), resp.read()
        except Exception as e:
            return None, e


def discover_all() -> Dict[str, Optional[Dict[str, Any]]]:
    """Discover available local MCP servers and classify them.

    Returns a dict like:
      { 'agents': {'url':..., 'token':..., 'verify': bool}, 'embedded': {...} }
    """
    result = {'agents': None, 'embedded': None}

    candidates = []
    # prefer env file if present
    if os.path.exists(ENV_FILE):
        data = {}
        with open(ENV_FILE, 'r') as f:
            for line in f:
                if '=' in line:
                    k, v = line.strip().split('=', 1)
                    data[k] = v
        if 'MCP_HTTP_PORT' in data:
            candidates.append((int(data['MCP_HTTP_PORT']), data.get('MCP_HTTP_TOKEN')))

    # probe ephemeral and common ports
    for p in list(range(49152, 49163)) + [8090, 8008, 8443]:
        candidates.append((p, None))

    seen = set()
    for port, token in candidates:
        if port in seen: continue
        seen.add(port)
        if not _probe_port(port):
            continue
        scheme = 'https' if port == 8443 else 'http'
        base = f"{scheme}://127.0.0.1:{port}"
        # Try agents endpoint first (python MCP server)
        code, resp = _http_get_raw(base + '/agents', token=token, timeout=0.8, verify=(scheme!='https'))
        if code == 200:
            result['agents'] = {'url': base, 'token': token, 'verify': (scheme!='https')}
            # also check /ui/state
            # continue probing to find embedded as well
        # Try embedded endpoints
        code_m, resp_m = _http_get_raw(base + '/mcp/metrics', token=token, timeout=0.8, verify=(scheme!='https'))
        if code_m == 200:
            result['embedded'] = {'url': base, 'token': token, 'verify': (scheme!='https')}
        # If both found, we can break
        if result['agents'] and result['embedded']:
            break

    return result


# Backwards-compatible discover that returns a single base_url+token (prefer agents server)
def discover():
    all_found = discover_all()
    if all_found.get('agents'):
        a = all_found['agents']
        return (a['url'], a.get('token'))
    if all_found.get('embedded'):
        e = all_found['embedded']
        return (e['url'], e.get('token'))
    return (None, None)


# Convenience getters to fetch the appropriate server info

def _choose_agents_server():
    all_found = discover_all()
    return all_found.get('agents') or all_found.get('embedded')


def _choose_embedded_server():
    all_found = discover_all()
    return all_found.get('embedded') or all_found.get('agents')


# High-level helpers updated to prefer correct server

def get_agents(base_url: Optional[str] = None, token: Optional[str] = None):
    server = None
    if base_url:
        server = {'url': base_url, 'token': token, 'verify': not base_url.startswith('https://')}
    else:
        server = _choose_agents_server()
    if not server:
        raise RuntimeError('agents server not found')
    code, resp = _http_get_raw(server['url'] + '/agents', token=server.get('token'), timeout=3, verify=server.get('verify', True))
    if code == 200:
        if _HAS_REQUESTS and hasattr(resp, 'json'):
            return resp.json()
        else:
            return json.loads(resp.decode('utf-8'))
    raise RuntimeError(f'GET /agents failed: {code} {resp}')


def invoke_agent(agent_id: str, action: str, params: Optional[Dict[str, Any]] = None, base_url: Optional[str] = None, token: Optional[str] = None):
    server = None
    if base_url:
        server = {'url': base_url, 'token': token, 'verify': not base_url.startswith('https://')}
    else:
        server = _choose_agents_server()
    if not server:
        raise RuntimeError('agents server not found')
    code, resp = _http_post_raw(f"{server['url']}/agents/{agent_id}/invoke", {'action': action, 'params': params or {}}, token=server.get('token'), timeout=10, verify=server.get('verify', True))
    if code == 200:
        if _HAS_REQUESTS and hasattr(resp, 'json'):
            return resp.json()
        else:
            return json.loads(resp.decode('utf-8'))
    raise RuntimeError(f'POST /agents/{agent_id}/invoke failed: {code} {resp}')


def get_ui_state(base_url: Optional[str] = None, token: Optional[str] = None):
    server = None
    if base_url:
        server = {'url': base_url, 'token': token, 'verify': not base_url.startswith('https://')}
    else:
        server = _choose_agents_server()
    if not server:
        raise RuntimeError('agents server not found')
    code, resp = _http_get_raw(server['url'] + '/ui/state', token=server.get('token'), timeout=3, verify=server.get('verify', True))
    if code == 200:
        if _HAS_REQUESTS and hasattr(resp, 'json'):
            return resp.json()
        else:
            return json.loads(resp.decode('utf-8'))
    raise RuntimeError(f'GET /ui/state failed: {code} {resp}')


def press_button(button_id: str, action: str = 'click', base_url: Optional[str] = None, token: Optional[str] = None):
    server = None
    if base_url:
        server = {'url': base_url, 'token': token, 'verify': not base_url.startswith('https://')}
    else:
        server = _choose_agents_server()
    if not server:
        raise RuntimeError('agents server not found')
    code, resp = _http_post_raw(server['url'] + '/ui/button', {'id': button_id, 'action': action}, token=server.get('token'), timeout=5, verify=server.get('verify', True))
    if code == 200:
        if _HAS_REQUESTS and hasattr(resp, 'json'):
            return resp.json()
        else:
            return json.loads(resp.decode('utf-8'))
    raise RuntimeError(f'POST /ui/button failed: {code} {resp}')


def get_metrics(base_url: Optional[str] = None, token: Optional[str] = None):
    server = None
    if base_url:
        server = {'url': base_url, 'token': token, 'verify': not base_url.startswith('https://')}
    else:
        server = _choose_embedded_server()
    if not server:
        raise RuntimeError('embedded MCP server not found')
    code, resp = _http_get_raw(server['url'] + '/mcp/metrics', token=server.get('token'), timeout=3, verify=server.get('verify', True))
    if code == 200:
        if _HAS_REQUESTS and hasattr(resp, 'json'):
            return resp.json()
        else:
            return json.loads(resp.decode('utf-8'))
    raise RuntimeError(f'GET /mcp/metrics failed: {code} {resp}')


def get_plugins(base_url: Optional[str] = None, token: Optional[str] = None):
    server = None
    if base_url:
        server = {'url': base_url, 'token': token, 'verify': not base_url.startswith('https://')}
    else:
        server = _choose_embedded_server()
    if not server:
        raise RuntimeError('embedded MCP server not found')
    code, resp = _http_get_raw(server['url'] + '/mcp/plugins', token=server.get('token'), timeout=3, verify=server.get('verify', True))
    if code == 200:
        if _HAS_REQUESTS and hasattr(resp, 'json'):
            return resp.json()
        else:
            return json.loads(resp.decode('utf-8'))
    raise RuntimeError(f'GET /mcp/plugins failed: {code} {resp}')


def get_snapshot(base_url: Optional[str] = None, token: Optional[str] = None):
    server = None
    if base_url:
        server = {'url': base_url, 'token': token, 'verify': not base_url.startswith('https://')}
    else:
        server = _choose_embedded_server()
    if not server:
        raise RuntimeError('embedded MCP server not found')
    code, resp = _http_get_raw(server['url'] + '/mcp/snapshot', token=server.get('token'), timeout=5, verify=server.get('verify', True))
    if code == 200:
        # try to parse JSON when possible
        try:
            if _HAS_REQUESTS and hasattr(resp, 'json'):
                return resp.json()
            else:
                return json.loads(resp.decode('utf-8'))
        except Exception:
            # return raw text
            if _HAS_REQUESTS and hasattr(resp, 'text'):
                return resp.text
            else:
                return resp.decode('utf-8')
    raise RuntimeError(f'GET /mcp/snapshot failed: {code} {resp}')


if __name__ == '__main__':
    # simple CLI demo for AI agents or debugging
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument('--discover', action='store_true')
    p.add_argument('--discover-all', action='store_true')
    p.add_argument('--agents', action='store_true')
    p.add_argument('--metrics', action='store_true')
    args = p.parse_args()
    if args.discover_all:
        print(json.dumps(discover_all(), indent=2))
    elif args.discover:
        print(discover())
    elif args.agents:
        print(json.dumps(get_agents(), indent=2))
    elif args.metrics:
        print(json.dumps(get_metrics(), indent=2))
    else:
        print('Use --agents or --metrics or --discover-all')
