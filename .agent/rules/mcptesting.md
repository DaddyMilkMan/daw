# Use Embedded MCP Server for UI Verification

When verifying UI changes or implementing new features that affect the visual hierarchy:

1.  **Prefer Programmatic Verification**: Use the embedded MCP server tools (`get_ui_tree`, `click_component`, `screenshot`) to verify state rather than relying solely on visual inspection or unit tests.
2.  **Use `verify_mcp.py` Pattern**: Adapt the standard verification script to launch the DAW, send JSON-RPC commands, and assert expected responses.
3.  **Ensure Clean Transport**: Verify that the application does not pollute `stdout` with logs, as this breaks the MCP JSON-RPC channel. Use `std::cerr` for all application logging.
4.  **Test Accessibility**: The `get_ui_tree` tool exposes the accessibility tree. Use it to confirm that all interactive elements are properly exposed to the AI/accessibility layer.
