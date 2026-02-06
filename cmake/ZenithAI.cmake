# =============================================================================
# ZENITH AI MODULE
# =============================================================================

include_guard(GLOBAL)

# AI Client sources
set(ZENITH_AI_SOURCES
    apps/desktop/Source/ai_client/AIEventBus.cpp
    apps/desktop/Source/ai_client/AIMasteringAgent.cpp
    apps/desktop/Source/ai_client/AIResponseCache.cpp
    apps/desktop/Source/ai_client/AIStatusManager.cpp
    apps/desktop/Source/ai_client/APILimits.cpp
    apps/desktop/Source/ai_client/AudioFitnessEvaluator.cpp
    apps/desktop/Source/ai_client/AudioThreadSafeProcessor.cpp
    apps/desktop/Source/ai_client/CreativeNeuralNetwork.cpp
    apps/desktop/Source/ai_client/CreativePartner.cpp
    apps/desktop/Source/ai_client/GenreDetector.cpp
    apps/desktop/Source/ai_client/GrokAPIClient.cpp
    apps/desktop/Source/ai_client/GrokAPIReal.cpp
    apps/desktop/Source/ai_client/GrokJourney.cpp
    apps/desktop/Source/ai_client/MIDIPatternGenerator.cpp
    apps/desktop/Source/ai_client/MixingAssistant.cpp
    apps/desktop/Source/ai_client/MLGenreClassifier.cpp
    apps/desktop/Source/ai_client/ModelTrainer.cpp
    apps/desktop/Source/ai_client/NeuralInferenceBridge.cpp
    apps/desktop/Source/ai_client/NeuralPresetGenerator.cpp
    apps/desktop/Source/ai_client/PredictiveMastering.cpp
    apps/desktop/Source/ai_client/PresetGeneticistAgent.cpp
    apps/desktop/Source/ai_client/PresetSchemaValidator.cpp
    apps/desktop/Source/ai_client/PresetSuggestionService.cpp
    apps/desktop/Source/ai_client/ProjectContext.cpp
    apps/desktop/Source/ai_client/ProjectRefactorerAgent.cpp
    apps/desktop/Source/ai_client/RealNeuralNetwork.cpp
    apps/desktop/Source/ai_client/ReferenceMatcher.cpp
    apps/desktop/Source/ai_client/SampleHunterAgent.cpp
    apps/desktop/Source/ai_client/SessionDebuggerAgent.cpp
    apps/desktop/Source/ai_client/UXDirectorAgent.cpp
    apps/desktop/Source/ai_client/VisualAnalyzer.cpp
    apps/desktop/Source/ai_client/WingmanSynthBridge.cpp
    apps/desktop/Source/ai_client/ZenithStyleApplicator.cpp
)

# Create AI library
add_library(zenith_ai STATIC
    ${ZENITH_AI_SOURCES}
)

# Target properties
target_include_directories(zenith_ai
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/apps/desktop/Source/ai_client>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/apps/desktop/Source>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_network>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zenith_ai
    PUBLIC
        cxx_std_20
)

target_link_libraries(zenith_ai
    PUBLIC
        zenith_core
        zenith_network
        juce_core
        juce_events
        juce_audio_basics
        juce_dsp
)

# Installation
install(
    TARGETS zenith_ai
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
