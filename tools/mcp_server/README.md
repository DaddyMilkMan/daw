MCP Server

Run a local MCP server for development and testing of agent integrations.

Start server:

python3 tools/mcp_server/server.py

Run unit tests:

python3 -m unittest discover -s tools/mcp_server/tests -v

The server listens on 127.0.0.1:8008 by default when launched directly. Tests start an ephemeral port automatically.
