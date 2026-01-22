import unittest
import sys
import os
import time
import json
import urllib.request

# Ensure the server directory is importable
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

import server

class MCPServerTests(unittest.TestCase):
    def setUp(self):
        # Start server on an ephemeral port
        self.server = server.start_server_in_thread('127.0.0.1', 0)
        host, port = self.server.server_address
        self.base_url = f'http://{host}:{port}'
        # allow server to start
        time.sleep(0.05)

    def tearDown(self):
        try:
            self.server.shutdown()
        except Exception:
            pass

    def post_json(self, path, payload):
        data = json.dumps(payload).encode('utf-8')
        req = urllib.request.Request(self.base_url + path, data=data, headers={'Content-Type': 'application/json'})
        with urllib.request.urlopen(req, timeout=5) as resp:
            return json.load(resp)

    def test_agents_list(self):
        with urllib.request.urlopen(self.base_url + '/agents') as resp:
            data = json.load(resp)
        self.assertIn('agents', data)
        self.assertTrue(len(data['agents']) > 0)

    def test_ui_button(self):
        res = self.post_json('/ui/button', {'id': 'play', 'action': 'click'})
        self.assertIn('buttons', res)
        self.assertIn('play', res['buttons'])

    def test_invoke_agent(self):
        res = self.post_json('/agents/debugger/invoke', {'action': 'status', 'params': {}})
        self.assertIn('result', res)
        self.assertIn('agent', res['result'])

if __name__ == '__main__':
    unittest.main()
