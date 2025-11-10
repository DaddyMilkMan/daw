// Hook for loading audio files and generating waveforms

import { useState, useCallback } from 'react';
import { audioEngine } from '../lib/audioEngine';
import { WaveformGenerator } from '../lib/waveformGenerator';
import { AudioClip } from '../types/clip';

export interface LoadedAudioFile {
  buffer: AudioBuffer;
  peaksData: ReturnType<typeof WaveformGenerator.generateRMSPeaks>;
  name: string;
  duration: number;
  sampleRate: number;
  numberOfChannels: number;
}

export function useAudioFileLoader() {
  const [isLoading, setIsLoading] = useState(false);
  const [progress, setProgress] = useState(0);
  const [error, setError] = useState<string | null>(null);

  const loadAudioFile = useCallback(async (file: File): Promise<LoadedAudioFile | null> => {
    setIsLoading(true);
    setProgress(0);
    setError(null);

    try {
      // Initialize audio engine if needed
      if (!audioEngine.isReady()) {
        await audioEngine.initialize();
      }

      setProgress(25);

      // Load and decode audio file
      const buffer = await audioEngine.loadAudioFile(file);

      setProgress(50);

      // Generate waveform peaks
      const cacheKey = `${file.name}-${file.size}-${file.lastModified}`;
      const peaksData = WaveformGenerator.generateRMSPeaks(buffer, 100, cacheKey);

      setProgress(100);

      const result: LoadedAudioFile = {
        buffer,
        peaksData,
        name: file.name.replace(/\.[^/.]+$/, ''), // Remove extension
        duration: buffer.duration,
        sampleRate: buffer.sampleRate,
        numberOfChannels: buffer.numberOfChannels,
      };

      setIsLoading(false);
      return result;
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to load audio file';
      setError(errorMessage);
      setIsLoading(false);
      console.error('Audio file loading error:', err);
      return null;
    }
  }, []);

  const loadMultipleFiles = useCallback(
    async (files: File[]): Promise<LoadedAudioFile[]> => {
      const results: LoadedAudioFile[] = [];

      for (let i = 0; i < files.length; i++) {
        const file = files[i];
        setProgress((i / files.length) * 100);

        const result = await loadAudioFile(file);
        if (result) {
          results.push(result);
        }
      }

      setProgress(100);
      return results;
    },
    [loadAudioFile]
  );

  return {
    loadAudioFile,
    loadMultipleFiles,
    isLoading,
    progress,
    error,
  };
}
