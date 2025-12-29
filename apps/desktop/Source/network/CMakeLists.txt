target_sources(ZenithDAW PRIVATE

    GrokDAWClient.cpp
    GrokDAWController.cpp
    GrokUtils.cpp
    SecureKeyStore.cpp
    AudioAnalysisService.cpp
    AITools.cpp
    AIPrompts.cpp
    CollaborationManager.cpp
    MCPServer.cpp
)

# Platform-specific sources are handled via Source/platform/[os]/master

