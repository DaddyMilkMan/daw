/*

  juce::AudioBuffer<float> tempBuffer(static_cast<int>(reader->numChannels),
                                      static_cast<int>(numSamples));
  reader->read(&tempBuffer, 0, static_cast<int>(numSamples), startSample, true,
               true);

  // Compute spectrum
  const int fftSize = 1024;
  std::vector<float> fftData(static_cast<size_t>(fftSize * 2), 0.0f);

  // Mix to mono if there are multiple channels, then use the first 1024 samples
  if (tempBuffer.getNumChannels() > 1) {
    // Simple mixdown to the first channel
    for (int ch = 1; ch < tempBuffer.getNumChannels(); ++ch) {
      tempBuffer.addFrom(0, 0, tempBuffer, ch, 0, tempBuffer.getNumSamples());
    }
    tempBuffer.applyGain(0, 0, tempBuffer.getNumSamples(),
                         1.0f /
                             static_cast<float>(tempBuffer.getNumChannels()));
  }

  const float *data = tempBuffer.getReadPointer(0);
  for (int i = 0; i < fftSize && i < tempBuffer.getNumSamples(); ++i) {
    fftData[static_cast<size_t>(i)] = data[i];
  }

  // Apply Hann window
  for (int i = 0; i < fftSize; ++i) {
    float window =
        0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi *
                                static_cast<float>(i) /
                                static_cast<float>(fftSize - 1)));
    fftData[static_cast<size_t>(i)] *= window;
  }

  fft_.performFrequencyOnlyForwardTransform(fftData.data());

  // Store target spectrum
  targetSpectrum_.clear();
  for (int i = 0; i < fftSize / 2; ++i) {
    targetSpectrum_.push_back(std::abs(fftData[static_cast<size_t>(i)]));
  }

  DBG("PresetGeneticistAgent: Loaded target audio " << file.getFileName());
}

} // namespace ai
} // namespace zenith
