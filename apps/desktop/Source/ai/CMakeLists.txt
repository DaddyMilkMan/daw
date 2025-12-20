# AI Agents module
target_sources(ZenithDAW PRIVATE
    # Core Infrastructure
    AIStatusManager.h
    AIStatusManager.cpp
    AIEventBus.h
    AIEventBus.cpp
    AIResponseCache.h
    AIResponseCache.cpp
    GrokAPIClient.h

    # Style & UI
    ZenithStyleApplicator.h
    ZenithStyleApplicator.cpp

    # Fitness Evaluation
    AudioFitnessEvaluator.h
    AudioFitnessEvaluator.cpp

    # Agents
    AIMasteringAgent.h
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
)
