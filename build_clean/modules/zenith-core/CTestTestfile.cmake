# CMake generated Testfile for 
# Source directory: C:/zenith/daw/modules/zenith-core
# Build directory: C:/zenith/daw/build_clean/modules/zenith-core
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[ProjectStateTests]=] "C:/zenith/daw/build_clean/modules/zenith-core/ProjectStateTests_artefacts/Release/ProjectStateTests.exe")
set_tests_properties([=[ProjectStateTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/zenith/daw/modules/zenith-core/CMakeLists.txt;344;add_test;C:/zenith/daw/modules/zenith-core/CMakeLists.txt;0;")
add_test([=[TempoTests]=] "C:/zenith/daw/build_clean/modules/zenith-core/TempoTests_artefacts/Release/TempoTests.exe")
set_tests_properties([=[TempoTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/zenith/daw/modules/zenith-core/CMakeLists.txt;373;add_test;C:/zenith/daw/modules/zenith-core/CMakeLists.txt;0;")
add_test([=[RecordingAutomationTests]=] "C:/zenith/daw/build_clean/modules/zenith-core/RecordingAutomationTests_artefacts/Release/RecordingAutomationTests.exe")
set_tests_properties([=[RecordingAutomationTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/zenith/daw/modules/zenith-core/CMakeLists.txt;409;add_test;C:/zenith/daw/modules/zenith-core/CMakeLists.txt;0;")
add_test([=[InstrumentValidationTests]=] "C:/zenith/daw/build_clean/modules/zenith-core/InstrumentValidationTests_artefacts/Release/InstrumentValidationTests.exe")
set_tests_properties([=[InstrumentValidationTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/zenith/daw/modules/zenith-core/CMakeLists.txt;459;add_test;C:/zenith/daw/modules/zenith-core/CMakeLists.txt;0;")
add_test([=[PresetRegressionTests]=] "C:/zenith/daw/build_clean/modules/zenith-core/PresetRegressionTests_artefacts/Release/PresetRegressionTests.exe")
set_tests_properties([=[PresetRegressionTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/zenith/daw/modules/zenith-core/CMakeLists.txt;509;add_test;C:/zenith/daw/modules/zenith-core/CMakeLists.txt;0;")
add_test([=[TrackInstrumentIntegrationTests]=] "C:/zenith/daw/build_clean/modules/zenith-core/TrackInstrumentIntegrationTests_artefacts/Release/TrackInstrumentIntegrationTests.exe")
set_tests_properties([=[TrackInstrumentIntegrationTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/zenith/daw/modules/zenith-core/CMakeLists.txt;595;add_test;C:/zenith/daw/modules/zenith-core/CMakeLists.txt;0;")
add_test([=[SkiaSimpleTest]=] "C:/zenith/daw/build_clean/modules/zenith-core/SkiaSimpleTest_artefacts/Release/SkiaSimpleTest.exe")
set_tests_properties([=[SkiaSimpleTest]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/zenith/daw/modules/zenith-core/CMakeLists.txt;628;add_test;C:/zenith/daw/modules/zenith-core/CMakeLists.txt;0;")
subdirs("../../_deps/juce-build")
