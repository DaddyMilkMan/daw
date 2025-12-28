# MCP Server sources
target_sources(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MCPServer.cpp
)

target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
