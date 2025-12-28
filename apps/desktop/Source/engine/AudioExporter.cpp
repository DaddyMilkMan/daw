/*

  juce::int64 startSample = static_cast<juce::int64>(startTime * sampleRate);
  juce::int64 totalSamples = static_cast<juce::int64>(sampleRate * duration);
  juce::int64 samplesProcessed = 0;
  outMaxPeak = 0.0f;

  while (samplesProcessed < totalSamples && !shouldCancel_.load()) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(kExportBlockSize),
                                                  totalSamples - samplesProcessed));
    engine_.renderOfflineBlock(context, buffer, numSamples, startSample + samplesProcessed);


    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      float channelPeak = buffer.getMagnitude(ch, 0, numSamples);
      if (channelPeak > outMaxPeak) outMaxPeak = channelPeak;
    }
    samplesProcessed += numSamples;
  }
  return !shouldCancel_.load();
}

bool AudioExporter::renderToTempFile(const juce::File &tempFile,
                                     double duration, double sampleRate,
                                     double startTime, float &outMaxPeak) {
  tempFile.deleteFile();

  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::FileOutputStream> stream(tempFile.createOutputStream());
  if (!stream)
    return false;

  auto writerOptions = juce::AudioFormatWriterOptions()
      .withSampleRate(sampleRate)
      .withNumChannels(2)
      .withBitsPerSample(32);

  std::unique_ptr<juce::OutputStream> streamPtr(std::move(stream));
  std::unique_ptr<juce::AudioFormatWriter> writer(
      wavFormat.createWriterFor(streamPtr, writerOptions));

  if (!writer)
    return false;

  juce::AudioBuffer<float> buffer(2, kExportBlockSize);

  // Create local render context
  AudioRenderContext context;
  context.prepare(sampleRate, blockSize, engine_.getNumTracks(), engine_.getNumAuxBuses());


  // Note: Engine playback should already be suspended here by wrapper

  juce::int64 startSample = static_cast<juce::int64>(startTime * sampleRate);
  juce::int64 totalSamples = static_cast<juce::int64>(sampleRate * duration);
  juce::int64 samplesProcessed = 0;
  outMaxPeak = 0.0f;

  while (samplesProcessed < totalSamples && !shouldCancel_.load()) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(kExportBlockSize),
                                                  totalSamples - samplesProcessed));

    engine_.renderOfflineBlock(context, buffer, numSamples, startSample + samplesProcessed);


    // Find peak across both channels
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      float channelPeak = buffer.getMagnitude(ch, 0, numSamples);
      if (channelPeak > outMaxPeak)
        outMaxPeak = channelPeak;
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
      return false;

    samplesProcessed += numSamples;
  }

  return !shouldCancel_.load();
}

bool AudioExporter::writeFinalFile(const juce::File &tempFile,
                                   const ExportOptions &options,
                                   float maxPeak) {
  // Calculate normalization gain
  float targetLinear = juce::Decibels::decibelsToGain(static_cast<float>(options.normalizeDb));
  float gain = 1.0f;

  if (maxPeak > 0.0001f) {
    gain = targetLinear / maxPeak;
  }

  DBG("AudioExporter: Applying Gain: " + juce::String(gain) + 
      " (" + juce::String(juce::Decibels::gainToDecibels(gain)) + " dB)");

  // Read temporary file
  juce::AudioFormatManager tempMgr;
  tempMgr.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(tempMgr.createReaderFor(tempFile));
  if (!reader)
    return false;

  // Prepare output format
  juce::AudioFormat *targetFormat = getFormatForType(options.format);
  if (!targetFormat)
    return false;

  juce::File outputFile = options.outputFile;
  outputFile.deleteFile();

  std::unique_ptr<juce::FileOutputStream> outStream(outputFile.createOutputStream());
  if (!outStream)
    return false;

  auto writerOptions = juce::AudioFormatWriterOptions()
      .withSampleRate(options.sampleRate)
      .withNumChannels(2)
      .withBitsPerSample(options.bitDepth);

  std::unique_ptr<juce::OutputStream> streamPtr(std::move(outStream));
  std::unique_ptr<juce::AudioFormatWriter> writer(
      targetFormat->createWriterFor(streamPtr, writerOptions));
  if (!writer)
    return false;

  // Process with gain and dithering
  juce::AudioBuffer<float> buffer(2, kExportBlockSize);
  
  zenith::dsp::Dither dither;
  if (options.enableDither) {
    dither.prepare(2);
    // Note: Dither type is configured via noise shaping internally
  }

  juce::int64 totalSamples = reader->lengthInSamples;
  juce::int64 samplesWritten = 0;

  while (samplesWritten < totalSamples && !shouldCancel_.load()) {
    int numSamples = static_cast<int>(juce::jmin(static_cast<juce::int64>(kExportBlockSize), 
                                                  totalSamples - samplesWritten));

    reader->read(&buffer, 0, numSamples, samplesWritten, true, true);

    // Apply normalization gain
    buffer.applyGain(gain);

    // Apply dithering
    if (options.enableDither && options.bitDepth < 32) {
      dither.process(buffer, options.bitDepth);
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, numSamples))
      return false;

    samplesWritten += numSamples;

    if (options.progressCallback) {
      float progress = 0.5f + (0.5f * static_cast<float>(samplesWritten) / 
                               static_cast<float>(totalSamples));
      options.progressCallback(progress, "Writing normalized audio...");
    }
  }

  return !shouldCancel_.load();
}

