target_sources(ZenithDAW PRIVATE
    GrokAPIClient.cpp
    GrokDAWController.cpp
    GrokUtils.cpp
    SecureKeyStore.cpp
    AudioAnalysisService.cpp
    AITools.cpp
    AIPrompts.cpp
    CollaborationManager.cpp
)

# Platform-specific sources are handled via Source/platform/[os]/master

