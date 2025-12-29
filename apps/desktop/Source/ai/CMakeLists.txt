# AI Agents module
target_sources(ZenithDAW PRIVATE
    # Core Infrastructure
    AIStatusManager.h
    AIStatusManager.cpp
    AIEventBus.h
    AIEventBus.cpp
    AIResponseCache.h
    AIResponseCache.cpp
    # Core API
    GrokAPIClient.h
    GrokAPIClient.cpp
    GrokJourney.h
    GrokJourney.cpp
    ReferenceMatcher.h
    ReferenceMatcher.cpp

    # Phase 2: Context-Aware AI (FIXED - JUCE 8 compatible)
    VisualAnalyzer.h
    VisualAnalyzer.cpp
    ProjectContext.h
    ProjectContext.cpp
    GenreDetector.h
    GenreDetector.cpp
    CreativePartner.h
    CreativePartner.cpp
    
    # Phase 3: Autonomous Intelligence (some files pending implementation)
    PredictiveMastering.h
    # PredictiveMastering.cpp  # Not yet implemented
    # RealTimeCollaboration.h  # Not yet implemented
    # RealTimeCollaboration.cpp  # Not yet implemented
    # AudioThreadSafeProcessor.h  # Not yet implemented
    # AudioThreadSafeProcessor.cpp  # Not yet implemented
    # RealNeuralNetwork.h  # Not yet implemented
    # RealNeuralNetwork.cpp  # Not yet implemented
    # AutonomousDecisionEngine.h  # Not yet implemented
    # AutonomousDecisionEngine.cpp  # Not yet implemented

    # Style & UI
    ZenithStyleApplicator.h
    ZenithStyleApplicator.cpp

    # Fitness Evaluation
    AudioFitnessEvaluator.h
    AudioFitnessEvaluator.cpp

    # Agents (FIXED for JUCE 8 compatibility)
    AIMasteringAgent.h
    AIMasteringAgent.cpp
    SessionDebuggerAgent.h
    SessionDebuggerAgent.cpp
    ProjectRefactorerAgent.h
    ProjectRefactorerAgent.cpp
    UXDirectorAgent.h
    UXDirectorAgent.cpp
    SampleHunterAgent.h
    SampleHunterAgent.cpp
    PresetGeneticistAgent.h
    PresetGeneticistAgent.cpp
    PresetSuggestionService.h
    PresetSuggestionService.cpp
)

# AI Dependencies (DISABLED - using JSON storage fallback)
# SQLite3 integration commented out until proper vcpkg integration
# find_package(SQLite3 QUIET)
# if(SQLite3_FOUND)
#     target_link_libraries(ZenithDAW PRIVATE SQLite3::SQLite3)
#     target_compile_definitions(ZenithDAW PRIVATE USE_SQLITE=1)
# else()
#     message(STATUS "SQLite3 not found - using JSON storage fallback for AI learning")
# endif()
message(STATUS "AI learning using JSON storage fallback")

# Add AI include directories
target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# AI Compiler Definitions
target_compile_definitions(ZenithDAW PRIVATE
    ZENITH_AI_ENABLED=1
    ZENITH_PHASE3_ENABLED=1
)
