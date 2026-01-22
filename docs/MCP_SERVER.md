MCP Server package - developer documentation

This lightweight MCP (Machine Communication Protocol) test server exposes a set
of simulated agent endpoints so other agents can interact with a predictable
service for development and integration testing.

Available endpoints

- GET /agents
  - Returns a list of available agent stubs with id, name and description.

- POST /agents/{id}/invoke
  - Body: { "action": "<action>", "params": { ... } }
  - Invokes the named agent with an action and parameters. Returns { "result": ... }

- GET /ui/state
  - Returns current UI state including buttons and small sample assets.

- POST /ui/button
  - Body: { "id": "play", "action": "click" }
  - Actions: "click" (toggle), "press", "release".
  - Returns updated buttons map.

- GET /assets/{name}
  - Returns a tiny SVG preview for sample assets (Content-Type: image/svg+xml)

Examples (curl)

List agents:

curl -s http://127.0.0.1:8008/agents | jq

Invoke debugger status:

curl -s -X POST http://127.0.0.1:8008/agents/debugger/invoke \
  -H 'Content-Type: application/json' \
  -d '{"action":"status","params":{}}' | jq

Click the play button:

curl -s -X POST http://127.0.0.1:8008/ui/button -H 'Content-Type: application/json' -d '{"id":"play","action":"click"}' | jq

Notes

- The agents are simulated and return deterministic stubbed responses useful for tooling, testing, and UI automation. They do not attach to real processes.
- The server is intentionally dependency-free (uses Python stdlib) so it is easy to run in CI or locally.

Integration tips for agents

- Add the server URL as a remote endpoint and implement lightweight HTTP JSON calls to exercise features.
- Use /ui/button to simulate pressing UI controls; the server will update an in-memory state map and optionally notify the ui_inspector stub.

