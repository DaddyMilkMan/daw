/*
  ==============================================================================
    agents/SecurityAgent/SecurityAgentBridge.h
    C++ bridge for Python SecurityAgent integration.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <string>
#include <vector>

namespace zenith {
namespace agents {

//==============================================================================
/**
    SecurityAgentBridge provides C++ interface to Python SecurityAgent.
    
    This bridge allows C++ code to invoke security validation functions
    implemented in Python.
*/
class SecurityAgentBridge {
public:
  //==============================================================================
  enum class ValidationResult {
    Valid,
    Invalid,
    Suspicious,
    Error
  };

  //==============================================================================
  SecurityAgentBridge();
  ~SecurityAgentBridge();

  //==============================================================================
  // Input Validation
  
  /// Validate MIDI input data
  ValidationResult validateMIDIData(const juce::MidiBuffer& midiData);
  
  /// Validate audio file before loading
  ValidationResult validateAudioFile(const juce::File& file);
  
  /// Validate project file before loading
  ValidationResult validateProjectFile(const juce::File& file);
  
  //==============================================================================
  // Plugin Security
  
  /// Verify plugin signature and trust status
  bool verifyPluginSignature(const juce::File& pluginFile);
  
  /// Check if plugin is in trusted list
  bool isPluginTrusted(const juce::File& pluginFile);
  
  //==============================================================================
  // Configuration
  
  /// Enable/disable security checks
  void setEnabled(bool enabled);
  
  /// Get security status
  bool isEnabled() const { return enabled_; }

private:
  //==============================================================================
  bool enabled_{true};
  
  // TODO: Add Python interpreter integration
  // TODO: Add IPC for Python agent communication
  // TODO: Add caching for validation results
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SecurityAgentBridge)
};

} // namespace agents
} // namespace zenith
