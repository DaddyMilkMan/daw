# =============================================================================
# ONNX RUNTIME INTEGRATION
# =============================================================================

# ONNX Runtime for AI features (Linux only currently)
option(ENABLE_ONNX "Enable ONNX Runtime for AI features" ON)

if(ENABLE_ONNX)
    if(UNIX AND NOT APPLE)
        include(FetchContent)
        FetchContent_Declare(
            onnxruntime
            URL https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-linux-x64-1.17.1.tgz
        )
        FetchContent_MakeAvailable(onnxruntime)
        
        # Global compile definitions
        add_compile_definitions(ZENITH_USE_ONNX_RUNTIME=1)
        message(STATUS "Zenith DAW: ONNX Runtime support ENABLED (Linux)")
        
        # Set up variables for main CMakeLists.txt
        set(ONNX_RUNTIME_LIBRARIES ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so)
        set(ONNX_RUNTIME_INCLUDE_DIRS ${onnxruntime_SOURCE_DIR}/include)
    else()
        message(STATUS "Zenith DAW: ONNX Runtime support disabled on this platform")
        set(ENABLE_ONNX OFF)
    endif()
else()
    message(STATUS "Zenith DAW: ONNX Runtime support disabled by user")
endif()
