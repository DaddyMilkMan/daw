MCP Server Quick Reference

Overview
- Python MCP server: tools/mcp_server/server.py — development server exposing /agents, /ui/state, /ui/button, and asset previews. Supports optional TLS via MCP_SERVER_CERT/MCP_SERVER_KEY and token auth via MCP_SERVER_TOKEN.
- Embedded C++ MCP server: in-process server exposing /mcp/metrics, /mcp/plugins, /mcp/snapshot. Automatically binds to 49152..49162 if MCP_HTTP_PORT not set; use MCP_HTTP_TOKEN or MCP_SERVER_TOKEN to secure.

AI Agent Integration
- Use tools/mcp_server/ai_agent_client.py from external agents to discover and interact with servers. It probes /tmp/zenith_mcp_env, ephemeral ports, and common ports.
- Example in tools/mcp_server/ai_agent_quickstart.md

How to enable embedded C++ TLS (dev only)
- Generate certs with tools/mcp_server/generate_self_signed_cert.sh
- Set MCP_HTTP_CERT and MCP_HTTP_KEY env vars when launching DAW. Embedded server will attempt to enable TLS if platform supports it (note: production-grade TLS may require linking OpenSSL or using a TLS-enabled HTTP library).

CI
- GitHub Action: .github/workflows/mcp_integration.yml runs Python MCP server and basic discovery/tests.

Security
- This tooling is intended for local dev only. Keep servers bound to 127.0.0.1 and use tokens to prevent accidental exposure.

Contact
- For API changes or agent design questions, open an issue in the repo under the 'mcp' label.
