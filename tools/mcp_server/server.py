#!/usr/bin/env python3
"""Simple MCP test server.

This HTTP JSON server exposes a set of stubbed agents and a tiny UI
interaction API so other agents can press buttons and receive structured
responses. It's intentionally lightweight and dependency-free so it can be
used during development and testing.
"""

import json
from http.server import BaseHTTPRequestHandler, HTTPServer
import socketserver
from urllib.parse import urlparse
import threading
import time
import sys
import os

# Try to import local agents implementation (tests add this directory to sys.path)
try:
    from agents import AGENTS, invoke_agent
except Exception:
    AGENTS = {}
    def invoke_agent(name, action, params):
        return {'error': 'agents module not available'}

# In-memory UI state (buttons, assets)
UI_STATE = {
    'buttons': {
        'play': False,
        'stop': False,
        'record': False,
        'settings': False,
        'wingman': False,
    },
    'assets': {
        'svg_sample': "<svg xmlns='http://www.w3.org/2000/svg' width='160' height='48'><rect width='160' height='48' fill='#0a0a0a'/><text x='8' y='30' fill='#7ff'>SVG PREVIEW</text></svg>"
    }
}

class ThreadingHTTPServer(socketserver.ThreadingMixIn, HTTPServer):
    daemon_threads = True
    allow_reuse_address = True

class MCPHandler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def _send_json(self, data, code=200):
        body = json.dumps(data).encode('utf-8')
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == '/agents':
            agents_list = []
            for key, a in AGENTS.items():
                agents_list.append({'id': key, 'name': a.get('name', key), 'description': a.get('description', '')})
            self._send_json({'agents': agents_list})
            return
        if parsed.path == '/ui/state':
            self._send_json(UI_STATE)
            return
        if parsed.path == '/docs':
            self._send_json({'endpoints': ['/agents', '/agents/{id}/invoke (POST)', '/ui/state', '/ui/button (POST)']})
            return
        # Assets preview (simple SVG text)
        if parsed.path.startswith('/assets/'):
            name = parsed.path.split('/')[-1]
            if name in UI_STATE['assets']:
                svg = UI_STATE['assets'][name]
                body = svg.encode('utf-8')
                self.send_response(200)
                self.send_header('Content-Type', 'image/svg+xml')
                self.send_header('Content-Length', str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                return
            self._send_json({'error': 'asset not found'}, code=404)
            return
        self.send_response(404)
        self.end_headers()

    def _read_json(self):
        length = int(self.headers.get('Content-Length') or 0)
        if length == 0:
            return {}
        raw = self.rfile.read(length)
        return json.loads(raw.decode('utf-8'))

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path.startswith('/agents/'):
            parts = parsed.path.strip('/').split('/')
            if len(parts) >= 2:
                agent_id = parts[1]
                body = self._read_json()
                action = body.get('action')
                params = body.get('params', {})
                try:
                    result = invoke_agent(agent_id, action, params)
                    self._send_json({'result': result})
                except Exception as e:
                    self._send_json({'error': str(e)}, code=500)
                return
        if parsed.path == '/ui/button':
            body = self._read_json()
            bid = body.get('id')
            action = body.get('action', 'click')
            if bid not in UI_STATE['buttons']:
                self._send_json({'error': 'unknown button id'}, code=400)
                return
            if action == 'press':
                UI_STATE['buttons'][bid] = True
            elif action == 'release':
                UI_STATE['buttons'][bid] = False
            elif action == 'click':
                UI_STATE['buttons'][bid] = not UI_STATE['buttons'][bid]
            else:
                self._send_json({'error': 'unknown action'}, code=400)
                return
            # Notify ui_inspector if present (best-effort)
            if 'ui_inspector' in AGENTS:
                try:
                    AGENTS['ui_inspector']['handler']('button_event', {'id': bid, 'action': action})
                except Exception:
                    pass
            self._send_json({'buttons': UI_STATE['buttons']})
            return
        self.send_response(404)
        self.end_headers()

def start_server_in_thread(host='127.0.0.1', port=0):
    server = ThreadingHTTPServer((host, port), MCPHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server

def run_server(host='127.0.0.1', port=8008):
    print(f'Starting MCP server on {host}:{port}')
    server = ThreadingHTTPServer((host, port), MCPHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        server.shutdown()

if __name__ == '__main__':
    run_server()
