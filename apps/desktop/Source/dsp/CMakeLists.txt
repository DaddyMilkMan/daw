target_sources(ZenithDAW PRIVATE
    AudioFifo.h
    DSPStemSeparator.cpp
    DSPVoiceChanger.cpp
    SpectralProcessor.cpp
    TimeStretcher.cpp
    ONNXStemSeparator.cpp
    ModelManager.cpp
    ONNXStemSeparatorAutoLoader.cpp
)

# Platform-specific sources are handled via Source/platform/[os]/master

