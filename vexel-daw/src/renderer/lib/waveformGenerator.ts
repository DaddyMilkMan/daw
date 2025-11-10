// Waveform generation and caching system using Canvas API
// Provides efficient waveform visualization with RMS peak calculation

import { PeaksData } from '../types/clip';

export interface WaveformRenderOptions {
  width: number;
  height: number;
  waveformColor?: string;
  backgroundColor?: string;
  showRMS?: boolean; // Show RMS instead of peak
  trimStart?: number; // Trim offset in seconds
  trimEnd?: number; // Trim end offset in seconds
  fadeIn?: number; // Fade in duration in seconds
  fadeOut?: number; // Fade out duration in seconds
}

export class WaveformGenerator {
  private static peaksCache = new Map<string, PeaksData>();
  private static readonly CACHE_CHECK_INTERVAL = 1000 * 60 * 60; // 1 hour
  private static cacheCheckTimer: number | null = null;

  // Initialize cache cleanup timer
  static startCacheCleanup(): void {
    if (this.cacheCheckTimer) return;

    this.cacheCheckTimer = window.setInterval(() => {
      this.cleanExpiredCache();
    }, this.CACHE_CHECK_INTERVAL);
  }

  // Stop cache cleanup timer
  static stopCacheCleanup(): void {
    if (this.cacheCheckTimer) {
      clearInterval(this.cacheCheckTimer);
      this.cacheCheckTimer = null;
    }
  }

  // Clean up expired cache entries (older than 7 days)
  static cleanExpiredCache(): void {
    const now = new Date();
    let cleanedCount = 0;

    this.peaksCache.forEach((peaks, key) => {
      if (now > peaks.cacheExpiry) {
        this.peaksCache.delete(key);
        cleanedCount++;
      }
    });

    if (cleanedCount > 0) {
      console.log(`🗑️ Cleaned ${cleanedCount} expired waveform cache entries`);
    }
  }

  // Generate peaks data from AudioBuffer
  static generatePeaks(
    audioBuffer: AudioBuffer,
    peaksPerSecond: number = 100,
    cacheKey?: string
  ): PeaksData {
    // Check cache first
    if (cacheKey && this.peaksCache.has(cacheKey)) {
      const cached = this.peaksCache.get(cacheKey)!;
      console.log('📊 Using cached peaks for:', cacheKey);
      return cached;
    }

    const startTime = performance.now();

    const numberOfChannels = audioBuffer.numberOfChannels;
    const sampleRate = audioBuffer.sampleRate;
    const duration = audioBuffer.duration;
    const totalPeaks = Math.ceil(duration * peaksPerSecond);
    const samplesPerPeak = Math.floor(audioBuffer.length / totalPeaks);

    // Generate peaks for each channel
    const peaksData: number[][] = [];

    for (let channel = 0; channel < numberOfChannels; channel++) {
      const channelData = audioBuffer.getChannelData(channel);
      const channelPeaks: number[] = [];

      for (let i = 0; i < totalPeaks; i++) {
        const start = i * samplesPerPeak;
        const end = Math.min(start + samplesPerPeak, channelData.length);

        // Calculate both min and max for this peak
        let min = 1;
        let max = -1;

        for (let j = start; j < end; j++) {
          const sample = channelData[j];
          if (sample < min) min = sample;
          if (sample > max) max = sample;
        }

        // Store min and max as consecutive values
        channelPeaks.push(min, max);
      }

      peaksData.push(channelPeaks);
    }

    const now = new Date();
    const cacheExpiry = new Date(now.getTime() + 7 * 24 * 60 * 60 * 1000); // 7 days

    const peaks: PeaksData = {
      version: 1,
      length: totalPeaks,
      data: peaksData,
      samplesPerPeak,
      generatedAt: now,
      cacheExpiry,
    };

    // Cache the result
    if (cacheKey) {
      this.peaksCache.set(cacheKey, peaks);
    }

    const elapsedTime = performance.now() - startTime;
    console.log(`📊 Generated ${totalPeaks} peaks in ${elapsedTime.toFixed(2)}ms for ${duration.toFixed(2)}s audio`);

    return peaks;
  }

  // Generate RMS peaks (smoother, better for visual representation)
  static generateRMSPeaks(
    audioBuffer: AudioBuffer,
    peaksPerSecond: number = 100,
    cacheKey?: string
  ): PeaksData {
    if (cacheKey && this.peaksCache.has(cacheKey)) {
      return this.peaksCache.get(cacheKey)!;
    }

    const numberOfChannels = audioBuffer.numberOfChannels;
    const duration = audioBuffer.duration;
    const totalPeaks = Math.ceil(duration * peaksPerSecond);
    const samplesPerPeak = Math.floor(audioBuffer.length / totalPeaks);

    const peaksData: number[][] = [];

    for (let channel = 0; channel < numberOfChannels; channel++) {
      const channelData = audioBuffer.getChannelData(channel);
      const channelPeaks: number[] = [];

      for (let i = 0; i < totalPeaks; i++) {
        const start = i * samplesPerPeak;
        const end = Math.min(start + samplesPerPeak, channelData.length);

        // Calculate RMS
        let sumSquares = 0;
        const count = end - start;

        for (let j = start; j < end; j++) {
          const sample = channelData[j];
          sumSquares += sample * sample;
        }

        const rms = Math.sqrt(sumSquares / count);

        // Store as positive and negative RMS
        channelPeaks.push(-rms, rms);
      }

      peaksData.push(channelPeaks);
    }

    const now = new Date();
    const cacheExpiry = new Date(now.getTime() + 7 * 24 * 60 * 60 * 1000);

    const peaks: PeaksData = {
      version: 1,
      length: totalPeaks,
      data: peaksData,
      samplesPerPeak,
      generatedAt: now,
      cacheExpiry,
    };

    if (cacheKey) {
      this.peaksCache.set(cacheKey, peaks);
    }

    return peaks;
  }

  // Render waveform to canvas
  static renderToCanvas(
    canvas: HTMLCanvasElement,
    peaks: PeaksData,
    options: WaveformRenderOptions
  ): void {
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const { width, height } = options;
    const waveformColor = options.waveformColor || '#3b82f6';
    const backgroundColor = options.backgroundColor || '#1e293b';

    // Set canvas size
    canvas.width = width;
    canvas.height = height;

    // Clear canvas
    ctx.fillStyle = backgroundColor;
    ctx.fillRect(0, 0, width, height);

    // Calculate which peaks to display based on trim
    const totalPeaks = peaks.length;
    let startPeak = 0;
    let endPeak = totalPeaks;

    if (options.trimStart || options.trimEnd) {
      const duration = totalPeaks / 100; // Assuming 100 peaks per second
      if (options.trimStart) {
        startPeak = Math.floor((options.trimStart / duration) * totalPeaks);
      }
      if (options.trimEnd) {
        endPeak = totalPeaks - Math.floor((options.trimEnd / duration) * totalPeaks);
      }
    }

    const visiblePeaks = endPeak - startPeak;
    const pixelsPerPeak = width / visiblePeaks;

    // Draw waveform for each channel (stacked)
    const channelHeight = height / peaks.data.length;

    peaks.data.forEach((channelPeaks, channelIndex) => {
      const yOffset = channelIndex * channelHeight;
      const centerY = yOffset + channelHeight / 2;

      // Set style
      ctx.fillStyle = waveformColor;
      ctx.strokeStyle = waveformColor;

      // Draw waveform
      for (let i = 0; i < visiblePeaks; i++) {
        const peakIndex = (startPeak + i) * 2; // * 2 because we store min/max pairs
        const min = channelPeaks[peakIndex];
        const max = channelPeaks[peakIndex + 1];

        const x = i * pixelsPerPeak;
        const yMin = centerY + (min * channelHeight) / 2;
        const yMax = centerY + (max * channelHeight) / 2;

        // Calculate fade opacity
        let opacity = 1;
        if (options.fadeIn || options.fadeOut) {
          const duration = visiblePeaks / 100;
          const currentTime = (i / visiblePeaks) * duration;

          if (options.fadeIn && currentTime < options.fadeIn) {
            opacity = Math.min(opacity, currentTime / options.fadeIn);
          }

          if (options.fadeOut) {
            const fadeOutStart = duration - options.fadeOut;
            if (currentTime > fadeOutStart) {
              opacity = Math.min(opacity, (duration - currentTime) / options.fadeOut);
            }
          }
        }

        // Draw peak bar
        ctx.globalAlpha = opacity;
        ctx.fillRect(x, yMax, Math.max(1, pixelsPerPeak), yMin - yMax);
      }

      // Reset alpha
      ctx.globalAlpha = 1;

      // Draw center line
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.1)';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(0, centerY);
      ctx.lineTo(width, centerY);
      ctx.stroke();
    });
  }

  // Render waveform and return as data URL (for caching)
  static async renderToDataURL(
    peaks: PeaksData,
    options: WaveformRenderOptions
  ): Promise<string> {
    const canvas = document.createElement('canvas');
    this.renderToCanvas(canvas, peaks, options);
    return canvas.toDataURL('image/png');
  }

  // Get cache statistics
  static getCacheStats(): {
    entries: number;
    oldestEntry: Date | null;
    newestEntry: Date | null;
  } {
    let oldest: Date | null = null;
    let newest: Date | null = null;

    this.peaksCache.forEach((peaks) => {
      if (!oldest || peaks.generatedAt < oldest) {
        oldest = peaks.generatedAt;
      }
      if (!newest || peaks.generatedAt > newest) {
        newest = peaks.generatedAt;
      }
    });

    return {
      entries: this.peaksCache.size,
      oldestEntry: oldest,
      newestEntry: newest,
    };
  }

  // Clear all cache
  static clearCache(): void {
    this.peaksCache.clear();
    console.log('🗑️ Waveform cache cleared');
  }

  // Get cache entry
  static getCachedPeaks(key: string): PeaksData | null {
    return this.peaksCache.get(key) || null;
  }

  // Manually cache peaks
  static cachePeaks(key: string, peaks: PeaksData): void {
    this.peaksCache.set(key, peaks);
  }
}

// Start cache cleanup on module load
if (typeof window !== 'undefined') {
  WaveformGenerator.startCacheCleanup();
}
