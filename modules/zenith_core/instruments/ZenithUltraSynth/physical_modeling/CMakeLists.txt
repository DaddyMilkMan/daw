cmake_minimum_required(VERSION 3.10)

# String Model Voice
add_library(StringModelVoice
    StringModel.h
    StringModelVoice.cpp
)

target_include_directories(StringModelVoice PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../../..
)

target_compile_definitions(StringModelVoice PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# Wind Model Voice
add_library(WindModelVoice
    WindModel.h
    WindModelVoice.cpp
)

target_include_directories(WindModelVoice PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../../..
)

target_compile_definitions(WindModelVoice PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# Percussion Model Voice
add_library(PercussionModelVoice
    PercussionModel.h
    PercussionModelVoice.cpp
)

target_include_directories(PercussionModelVoice PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../../..
)

target_compile_definitions(PercussionModelVoice PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# Physical Modeling Library
add_library(ZenithPhysicalModeling
    StringModel.h
    StringModelVoice.cpp
    WindModel.h
    WindModelVoice.cpp
    PercussionModel.h
    PercussionModelVoice.cpp
)

target_include_directories(ZenithPhysicalModeling PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../../..
)

target_link_libraries(ZenithPhysicalModeling
    ZenithCore
    ZenithDSP
)

# Set C++ standard
target_compile_features(ZenithPhysicalModeling PRIVATE cxx_std_17)
