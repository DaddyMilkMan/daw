AI Agent Quickstart for MCP

This quickstart shows how an external AI agent can discover and interact with local MCP servers (embedded C++ or Python) using the provided ai_agent_client.py helper.

1) Discovery

The helper will look for a launcher file (/tmp/zenith_mcp_env) written by the DAW launcher, then probe common ports (49152-49162, 8090, 8008, 8443).

2) Example usage (Python agent)

from mcp_server.ai_agent_client import get_agents, invoke_agent, get_metrics, press_button

# List agents
agents = get_agents()
print(agents)

# Call agent 'metering' to fetch last snapshot
resp = invoke_agent('metering', 'get')
print(resp)

# Press Play button
press_button('play', 'click')

# Fetch live metrics
metrics = get_metrics()
print(metrics)

Notes:
- The helper uses X-MCP-Token header if the DAW launcher set a token. If the token is not present in the environment file, it will use MCP_HTTP_TOKEN or MCP_SERVER_TOKEN env vars.
- The helper prefers 'requests' if installed but falls back to the standard library.

Security:
- The MCP endpoints are intended for local agent usage; ensure the DAW is bound to 127.0.0.1 and use tokens for protection.
- Avoid exposing the MCP server to public networks.
