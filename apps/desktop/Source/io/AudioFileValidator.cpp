/*
  ==============================================================================

    AudioFileValidator.cpp
    Implementation of audio file validation

  ==============================================================================
*/

#include "AudioFileValidator.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// AudioFileValidator Implementation
//==============================================================================

AudioFileValidator::AudioFileValidator() {
    // Register basic formats
    formatManager_.registerBasicFormats();

    std::cout << "AudioFileValidator: Initialized" << std::endl;
}

AudioFileValidator::~AudioFileValidator() {
    std::cout << "AudioFileValidator: Shut down" << std::endl;
}

//==============================================================================
AudioFileValidationResult AudioFileValidator::validateFile(const juce::File& file) {
    AudioFileValidationResult result;

    // Check if file exists
    if (!file.existsAsFile()) {
        result.issues.push_back(AudioFileValidationIssue::CorruptHeader);
        return result;
    }

    // Check file size
    juce::int64 fileSize = file.getSize();
    result.fileSize = fileSize;

    if (!isValidFileSize(fileSize)) {
        result.issues.push_back(AudioFileValidationIssue::FileTooLarge);
    }

    // Check for path traversal
    if (checkForPathTraversal(file)) {
        result.issues.push_back(AudioFileValidationIssue::SecurityIssue);
    }

    // Try to create audio format reader
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager_.createReaderFor(file));

    if (reader == nullptr) {
        result.issues.push_back(AudioFileValidationIssue::InvalidFormat);
        return result;
    }

    // Extract file info
    // Get format from file extension
    juce::String fileExt = file.getFileExtension().toLowerCase();

    // Basic format detection
    if (fileExt == "wav") result.format = "WAV";
    else if (fileExt == "aiff" || fileExt == "aif") result.format = "AIFF";
    else if (fileExt == "flac") result.format = "FLAC";
    else if (fileExt == "ogg") result.format = "OGG";
    else if (fileExt == "mp3") result.format = "MP3";
    else result.format = fileExt.toUpperCase();

    result.sampleRate = reader->sampleRate;
    result.channels = reader->numChannels;
    result.lengthSeconds = reader->lengthInSamples / reader->sampleRate;

    // Get bits per sample
    result.bitDepth = reader->bitsPerSample;

    // Validate sample rate
    if (!isValidSampleRate(result.sampleRate)) {
        result.issues.push_back(AudioFileValidationIssue::InvalidSampleRate);
    }

    // Validate channels
    if (result.channels < 1 || result.channels > 64) {
        result.issues.push_back(AudioFileValidationIssue::InvalidChannels);
    }

    // Validate bit depth
    if (result.bitDepth != 0 &&
        (result.bitDepth < 8 || result.bitDepth > 64)) {
        result.issues.push_back(AudioFileValidationIssue::InvalidBitDepth);
    }

    // Check for truncation
    juce::int64 expectedBytes = reader->lengthInSamples * result.channels * (result.bitDepth / 8);
    if (fileSize < expectedBytes * 0.9) {  // Allow 10% margin
        result.issues.push_back(AudioFileValidationIssue::FileTruncated);
    }

    result.isValid = result.issues.empty();

    return result;
}

//==============================================================================
AudioFileValidationResult AudioFileValidator::validateHeader(const juce::File& file) {
    AudioFileValidationResult result;

    // Quick header validation
    if (!file.existsAsFile()) {
        result.issues.push_back(AudioFileValidationIssue::CorruptHeader);
        return result;
    }

    // Check file signature (magic bytes)
    if (!checkFileSignature(file)) {
        result.issues.push_back(AudioFileValidationIssue::CorruptHeader);
        return result;
    }

    // Quick format check
    juce::String ext = file.getFileExtension().toLowerCase();

    // Known magic bytes
    bool validMagic = false;
    if (ext == "wav" || ext == "wave") {
        validMagic = true;  // RIFF checked in checkFileSignature
    } else if (ext == "aiff" || ext == "aif") {
        validMagic = true;
    } else if (ext == "flac") {
        validMagic = true;
    }

    if (!validMagic) {
        result.issues.push_back(AudioFileValidationIssue::InvalidFormat);
    }

    result.isValid = result.issues.empty();

    return result;
}

//==============================================================================
bool AudioFileValidator::isFormatSupported(const juce::File& file) {
    juce::String ext = file.getFileExtension().toLowerCase();

    // Check if we have a reader for this format
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    return reader != nullptr;
}

//==============================================================================
std::vector<juce::String> AudioFileValidator::getSupportedFormats() {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    // Get common formats
    std::vector<juce::String> result;
    result.push_back("WAV");
    result.push_back("AIFF");
    result.push_back("FLAC");
    result.push_back("OGG");

    return result;
}

//==============================================================================
// Private Methods
//==============================================================================

bool AudioFileValidator::checkForPathTraversal(const juce::File& file) const {
    // Check for path traversal attempts
    juce::String path = file.getFullPathName();

    // Look for ../ patterns
    if (path.contains("../")) {
        return true;
    }

    // Check for absolute paths escaping root
    juce::File parent = file.getParentDirectory();

    // If file is not under expected root, suspicious
    // (Simplified check - production would be more thorough)

    return false;
}

bool AudioFileValidator::checkFileSignature(const juce::File& file) const {
    // Read first 16 bytes for magic number check
    juce::FileInputStream inputStream(file);
    if (!inputStream.openedOk()) {
        return false;
    }

    juce::uint8 header[16];
    int bytesRead = inputStream.read(header, 16);

    if (bytesRead < 16) {
        return false;
    }

    // Check for RIFF (WAV)
    if (header[0] == 'R' && header[1] == 'I' &&
        header[2] == 'F' && header[3] == 'F') {
        return true;
    }

    // Check for FORM (AIFF)
    if (header[0] == 'F' && header[1] == 'O' &&
        header[2] == 'R' && header[3] == 'M') {
        return true;
    }

    // Check for fLaC (FLAC)
    if (header[0] == 'f' && header[1] == 'L' &&
        header[2] == 'a' && header[3] == 'C') {
        return true;
    }

    // Check for ID3 (MP3)
    if (header[0] == 'I' && header[1] == 'D' &&
        header[2] == '3') {
        return true;
    }

    // File signature not recognized
    return false;
}

} // namespace zenith
