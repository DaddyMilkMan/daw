# =============================================================================
# ZENITH AI CLIENT MODULE
# =============================================================================

include_guard(GLOBAL)

# AI Client sources
set(ZENITH_AI_CLIENT_SOURCES
    modules/zenith_ai_client/AIEventBus.cpp
    modules/zenith_ai_client/AIMasteringAgent.cpp
    modules/zenith_ai_client/AIResponseCache.cpp
    modules/zenith_ai_client/AIStatusManager.cpp
    modules/zenith_ai_client/APILimits.cpp
    modules/zenith_ai_client/AudioFitnessEvaluator.cpp
    modules/zenith_ai_client/AudioThreadSafeProcessor.cpp
    modules/zenith_ai_client/CreativeNeuralNetwork.cpp
    modules/zenith_ai_client/CreativePartner.cpp
    modules/zenith_ai_client/GenreDetector.cpp
    modules/zenith_ai_client/GrokAPIClient.cpp
    modules/zenith_ai_client/GrokAPIReal.cpp
    modules/zenith_ai_client/GrokJourney.cpp
    modules/zenith_ai_client/MIDIPatternGenerator.cpp
    modules/zenith_ai_client/MixingAssistant.cpp
    modules/zenith_ai_client/MLGenreClassifier.cpp
    modules/zenith_ai_client/ModelTrainer.cpp
    modules/zenith_ai_client/NeuralInferenceBridge.cpp
    modules/zenith_ai_client/NeuralPresetGenerator.cpp
    modules/zenith_ai_client/PredictiveMastering.cpp
    modules/zenith_ai_client/PresetGeneticistAgent.cpp
    modules/zenith_ai_client/PresetSchemaValidator.cpp
    modules/zenith_ai_client/PresetSuggestionService.cpp
    modules/zenith_ai_client/ProjectContext.cpp
    modules/zenith_ai_client/ProjectRefactorerAgent.cpp
    modules/zenith_ai_client/RealNeuralNetwork.cpp
    modules/zenith_ai_client/ReferenceMatcher.cpp
    modules/zenith_ai_client/SampleHunterAgent.cpp
    modules/zenith_ai_client/SessionDebuggerAgent.cpp
    modules/zenith_ai_client/UXDirectorAgent.cpp
    modules/zenith_ai_client/VisualAnalyzer.cpp
    modules/zenith_ai_client/WingmanSynthBridge.cpp
    modules/zenith_ai_client/ZenithStyleApplicator.cpp
)

# Create AI library
add_library(zenith_ai_client STATIC
    ${ZENITH_AI_CLIENT_SOURCES}
)

# Target properties
target_include_directories(zenith_ai_client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_ai_client>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zenith_ai_client
    PUBLIC
        cxx_std_20
)

target_link_libraries(zenith_ai_client
    PUBLIC
        zenith_core
        zenith_network
        juce_core
        juce_events
        juce_audio_basics
        juce_dsp
)

# Alias for backward compatibility if needed, though we should update consumers
add_library(zenith_ai ALIAS zenith_ai_client)

# Installation
install(
    TARGETS zenith_ai_client
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
