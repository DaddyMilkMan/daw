/**
 * Waveform visualization utilities for audio buffers
 * Generates waveform peaks data for efficient canvas rendering
 */

export interface WaveformPeaks {
  min: number[];
  max: number[];
  length: number;
}

/**
 * Generate waveform peaks from audio buffer for visualization
 * @param buffer - AudioBuffer to analyze
 * @param samplesPerPixel - Number of audio samples to represent with one pixel
 * @returns Waveform peaks data
 */
export function generateWaveformPeaks(
  buffer: AudioBuffer,
  samplesPerPixel: number = 512
): WaveformPeaks {
  const channelData = buffer.getChannelData(0); // Use first channel
  const totalSamples = buffer.length;
  const peakCount = Math.ceil(totalSamples / samplesPerPixel);

  const min: number[] = [];
  const max: number[] = [];

  for (let i = 0; i < peakCount; i++) {
    const start = i * samplesPerPixel;
    const end = Math.min(start + samplesPerPixel, totalSamples);

    let minVal = 1.0;
    let maxVal = -1.0;

    for (let j = start; j < end; j++) {
      const sample = channelData[j];
      if (sample < minVal) minVal = sample;
      if (sample > maxVal) maxVal = sample;
    }

    min.push(minVal);
    max.push(maxVal);
  }

  return { min, max, length: peakCount };
}

/**
 * Draw waveform on canvas
 * @param canvas - HTML Canvas element
 * @param peaks - Waveform peaks data
 * @param color - Waveform color
 * @param backgroundColor - Background color
 */
export function drawWaveform(
  canvas: HTMLCanvasElement,
  peaks: WaveformPeaks,
  color: string = '#3b82f6',
  backgroundColor: string = '#1a1a1a'
): void {
  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const width = canvas.width;
  const height = canvas.height;
  const middle = height / 2;

  // Clear canvas
  ctx.fillStyle = backgroundColor;
  ctx.fillRect(0, 0, width, height);

  // Draw waveform
  ctx.fillStyle = color;
  ctx.strokeStyle = color;

  const pixelsPerPeak = width / peaks.length;

  // Draw using filled path for better performance
  ctx.beginPath();

  for (let i = 0; i < peaks.length; i++) {
    const x = i * pixelsPerPeak;
    const minVal = peaks.min[i];
    const maxVal = peaks.max[i];

    const minY = middle + (minVal * middle);
    const maxY = middle + (maxVal * middle);

    if (i === 0) {
      ctx.moveTo(x, minY);
    }

    // Draw vertical line from min to max
    ctx.lineTo(x, minY);
    ctx.lineTo(x, maxY);
  }

  // Complete the path
  for (let i = peaks.length - 1; i >= 0; i--) {
    const x = i * pixelsPerPeak;
    const maxVal = peaks.max[i];
    const maxY = middle + (maxVal * middle);
    ctx.lineTo(x, maxY);
  }

  ctx.closePath();
  ctx.fill();

  // Draw center line
  ctx.strokeStyle = 'rgba(255, 255, 255, 0.1)';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, middle);
  ctx.lineTo(width, middle);
  ctx.stroke();
}

/**
 * Draw waveform with gradient for better visual appeal
 */
export function drawWaveformGradient(
  canvas: HTMLCanvasElement,
  peaks: WaveformPeaks,
  colorStart: string = '#3b82f6',
  colorEnd: string = '#1d4ed8',
  backgroundColor: string = '#1a1a1a'
): void {
  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const width = canvas.width;
  const height = canvas.height;
  const middle = height / 2;

  // Clear canvas
  ctx.fillStyle = backgroundColor;
  ctx.fillRect(0, 0, width, height);

  // Create gradient
  const gradient = ctx.createLinearGradient(0, 0, 0, height);
  gradient.addColorStop(0, colorStart);
  gradient.addColorStop(0.5, colorEnd);
  gradient.addColorStop(1, colorStart);

  ctx.fillStyle = gradient;
  ctx.strokeStyle = gradient;

  const pixelsPerPeak = width / peaks.length;

  // Draw waveform
  ctx.beginPath();

  for (let i = 0; i < peaks.length; i++) {
    const x = i * pixelsPerPeak;
    const minVal = peaks.min[i];
    const maxVal = peaks.max[i];

    const minY = middle + (minVal * middle);
    const maxY = middle + (maxVal * middle);

    if (i === 0) {
      ctx.moveTo(x, minY);
    }

    ctx.lineTo(x, minY);
    ctx.lineTo(x, maxY);
  }

  for (let i = peaks.length - 1; i >= 0; i--) {
    const x = i * pixelsPerPeak;
    const maxVal = peaks.max[i];
    const maxY = middle + (maxVal * middle);
    ctx.lineTo(x, maxY);
  }

  ctx.closePath();
  ctx.fill();

  // Draw center line
  ctx.strokeStyle = 'rgba(255, 255, 255, 0.1)';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, middle);
  ctx.lineTo(width, middle);
  ctx.stroke();
}

/**
 * Calculate RMS (Root Mean Square) energy for amplitude visualization
 */
export function calculateRMS(buffer: AudioBuffer, startSample: number, length: number): number {
  const channelData = buffer.getChannelData(0);
  let sum = 0;

  const end = Math.min(startSample + length, buffer.length);

  for (let i = startSample; i < end; i++) {
    const sample = channelData[i];
    sum += sample * sample;
  }

  return Math.sqrt(sum / length);
}

/**
 * Get audio buffer duration in seconds
 */
export function getBufferDuration(buffer: AudioBuffer): number {
  return buffer.length / buffer.sampleRate;
}

/**
 * Convert seconds to sample index
 */
export function secondsToSamples(seconds: number, sampleRate: number): number {
  return Math.floor(seconds * sampleRate);
}

/**
 * Convert sample index to seconds
 */
export function samplesToSeconds(samples: number, sampleRate: number): number {
  return samples / sampleRate;
}
