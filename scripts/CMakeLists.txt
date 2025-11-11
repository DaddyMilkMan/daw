# ============================================================================
# Vexel Scripting Layer - Lua Bindings for Automation
# ============================================================================

if(NOT VEXEL_ENABLE_LUA)
    return()
endif()

# LuaBridge - C++ to Lua binding library
include(FetchContent)
FetchContent_Declare(
    LuaBridge
    GIT_REPOSITORY https://github.com/vinniefalco/LuaBridge.git
    GIT_TAG 2.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(LuaBridge)

# Scripting engine library
add_library(VexelScripting STATIC
    ScriptEngine.cpp
    ScriptEngine.h
    AudioEngineBindings.cpp
    AudioEngineBindings.h
)

target_include_directories(VexelScripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(VexelScripting PUBLIC
    VexelAudioEngine
    ${LUA_LIBRARIES}
)

# Install example scripts
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/automate_volume.lua
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/create_tracks.lua
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/midi_mapper.lua
    DESTINATION share/vexel/scripts/examples
)
