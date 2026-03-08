target_sources(ZenithDAW PRIVATE

    GrokDAWClient.cpp
    GrokDAWController.cpp
    GrokUtils.cpp
    SecureKeyStore.cpp
    AudioAnalysisService.cpp
    AITools.cpp
    AIPrompts.cpp
    CollaborationManager.cpp
    ICECandidate.cpp
    STUNClient.cpp
    TURNClient.cpp
    MCPServer.cpp
    EmbeddedMCPHttpServer.cpp
    EmbeddedMCPHttpServer_httplib.cpp
    UpdateService.cpp
)

# Enable optional cpp-httplib server when available
if (TARGET httplib::httplib)
    target_compile_definitions(ZenithDAW PRIVATE ENABLE_CPP_HTTP_LIB)
    target_link_libraries(ZenithDAW PRIVATE httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
endif()

# Platform-specific sources are handled via Source/platform/[os]/master
