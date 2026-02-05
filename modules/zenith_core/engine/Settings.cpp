#include "Settings.h"

Settings::Settings() {
    // Initialize settings
    loadDefaults();
}

Settings::~Settings() {
    // Clean up settings
}

void Settings::loadDefaults() {
    // Load default settings
    audioSampleRate = 44100;
    audioBufferSize = 512;
    audioChannels = 2;
}

int Settings::getAudioSampleRate() const {
    return audioSampleRate;
}

void Settings::setAudioSampleRate(int rate) {
    audioSampleRate = rate;
}

int Settings::getAudioBufferSize() const {
    return audioBufferSize;
}

void Settings::setAudioBufferSize(int size) {
    audioBufferSize = size;
}

int Settings::getAudioChannels() const {
    return audioChannels;
}

void Settings::setAudioChannels(int channels) {
    audioChannels = channels;
}