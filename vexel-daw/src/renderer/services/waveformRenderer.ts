/**
 * Waveform Renderer Service
 * Real-time waveform visualization for audio recording and playback
 */

export interface WaveformRenderOptions {
  width: number;
  height: number;
  backgroundColor: string;
  waveformColor: string;
  centerLineColor: string;
  gridColor?: string;
  showGrid?: boolean;
  amplitude?: number; // 0-1, default 1
  style?: 'filled' | 'outline' | 'bars';
}

export class WaveformRenderer {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private options: WaveformRenderOptions;
  private streamingData: Float32Array[];
  private maxDataPoints: number;

  constructor(canvas: HTMLCanvasElement, options: WaveformRenderOptions) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d')!;
    this.options = options;
    this.streamingData = [];
    this.maxDataPoints = 10000; // Limit stored data points

    this.canvas.width = options.width;
    this.canvas.height = options.height;
  }

  /**
   * Render a complete waveform from AudioBuffer
   */
  renderWaveform(audioBuffer: AudioBuffer): void {
    const data = audioBuffer.getChannelData(0); // Use first channel
    this.renderFromData(data);
  }

  /**
   * Render waveform from Float32Array data
   */
  renderFromData(data: Float32Array): void {
    this.clear();

    const { width, height, waveformColor, amplitude = 1, style = 'filled' } = this.options;

    const step = Math.ceil(data.length / width);
    const halfHeight = height / 2;

    this.ctx.strokeStyle = waveformColor;
    this.ctx.fillStyle = waveformColor;
    this.ctx.lineWidth = 1;

    if (style === 'filled') {
      this.renderFilledWaveform(data, step, halfHeight, amplitude);
    } else if (style === 'outline') {
      this.renderOutlineWaveform(data, step, halfHeight, amplitude);
    } else if (style === 'bars') {
      this.renderBarsWaveform(data, step, halfHeight, amplitude);
    }

    this.drawCenterLine();
    if (this.options.showGrid) {
      this.drawGrid();
    }
  }

  /**
   * Render filled waveform (classic DAW style)
   */
  private renderFilledWaveform(
    data: Float32Array,
    step: number,
    halfHeight: number,
    amplitude: number
  ): void {
    const { width } = this.options;

    this.ctx.beginPath();
    this.ctx.moveTo(0, halfHeight);

    // Draw top half
    for (let i = 0; i < width; i++) {
      let min = 1.0;
      let max = -1.0;

      for (let j = 0; j < step && i * step + j < data.length; j++) {
        const datum = data[i * step + j];
        if (datum < min) min = datum;
        if (datum > max) max = datum;
      }

      const y = (1 - max * amplitude) * halfHeight;
      this.ctx.lineTo(i, y);
    }

    // Draw bottom half (mirrored)
    for (let i = width - 1; i >= 0; i--) {
      let min = 1.0;
      let max = -1.0;

      for (let j = 0; j < step && i * step + j < data.length; j++) {
        const datum = data[i * step + j];
        if (datum < min) min = datum;
        if (datum > max) max = datum;
      }

      const y = (1 - min * amplitude) * halfHeight;
      this.ctx.lineTo(i, y);
    }

    this.ctx.closePath();
    this.ctx.fill();
  }

  /**
   * Render outline waveform
   */
  private renderOutlineWaveform(
    data: Float32Array,
    step: number,
    halfHeight: number,
    amplitude: number
  ): void {
    const { width } = this.options;

    this.ctx.beginPath();

    for (let i = 0; i < width; i++) {
      let min = 1.0;
      let max = -1.0;

      for (let j = 0; j < step && i * step + j < data.length; j++) {
        const datum = data[i * step + j];
        if (datum < min) min = datum;
        if (datum > max) max = datum;
      }

      const yMax = (1 - max * amplitude) * halfHeight;
      const yMin = (1 - min * amplitude) * halfHeight;

      if (i === 0) {
        this.ctx.moveTo(i, yMax);
      } else {
        this.ctx.lineTo(i, yMax);
      }

      this.ctx.lineTo(i, yMin);
    }

    this.ctx.stroke();
  }

  /**
   * Render bars waveform (for RMS/peak meters)
   */
  private renderBarsWaveform(
    data: Float32Array,
    step: number,
    halfHeight: number,
    amplitude: number
  ): void {
    const { width } = this.options;
    const barWidth = Math.max(1, Math.floor(width / 100));

    for (let i = 0; i < width; i += barWidth + 1) {
      let rms = 0;

      for (let j = 0; j < step && i * step + j < data.length; j++) {
        const datum = data[i * step + j];
        rms += datum * datum;
      }

      rms = Math.sqrt(rms / step);
      const barHeight = rms * amplitude * halfHeight;

      this.ctx.fillRect(i, halfHeight - barHeight, barWidth, barHeight * 2);
    }
  }

  /**
   * Add streaming data point (for real-time recording visualization)
   */
  addStreamingData(chunk: Float32Array): void {
    this.streamingData.push(new Float32Array(chunk));

    // Limit stored data to prevent memory issues
    const totalLength = this.streamingData.reduce((sum, arr) => sum + arr.length, 0);
    while (totalLength > this.maxDataPoints && this.streamingData.length > 0) {
      this.streamingData.shift();
    }

    this.renderStreamingWaveform();
  }

  /**
   * Render streaming waveform (real-time during recording)
   */
  private renderStreamingWaveform(): void {
    // Combine all streaming data
    const totalLength = this.streamingData.reduce((sum, arr) => sum + arr.length, 0);
    const combinedData = new Float32Array(totalLength);

    let offset = 0;
    for (const chunk of this.streamingData) {
      combinedData.set(chunk, offset);
      offset += chunk.length;
    }

    this.renderFromData(combinedData);
  }

  /**
   * Clear streaming data
   */
  clearStreamingData(): void {
    this.streamingData = [];
    this.clear();
  }

  /**
   * Clear canvas
   */
  clear(): void {
    const { width, height, backgroundColor } = this.options;

    this.ctx.fillStyle = backgroundColor;
    this.ctx.fillRect(0, 0, width, height);
  }

  /**
   * Draw center line
   */
  private drawCenterLine(): void {
    const { width, height, centerLineColor } = this.options;

    this.ctx.strokeStyle = centerLineColor;
    this.ctx.lineWidth = 1;
    this.ctx.setLineDash([5, 5]);

    this.ctx.beginPath();
    this.ctx.moveTo(0, height / 2);
    this.ctx.lineTo(width, height / 2);
    this.ctx.stroke();

    this.ctx.setLineDash([]);
  }

  /**
   * Draw grid lines
   */
  private drawGrid(): void {
    const { width, height, gridColor = 'rgba(255, 255, 255, 0.1)' } = this.options;

    this.ctx.strokeStyle = gridColor;
    this.ctx.lineWidth = 1;

    // Horizontal grid lines
    const gridSpacing = height / 4;
    for (let y = gridSpacing; y < height; y += gridSpacing) {
      this.ctx.beginPath();
      this.ctx.moveTo(0, y);
      this.ctx.lineTo(width, y);
      this.ctx.stroke();
    }

    // Vertical grid lines
    const verticalSpacing = width / 10;
    for (let x = verticalSpacing; x < width; x += verticalSpacing) {
      this.ctx.beginPath();
      this.ctx.moveTo(x, 0);
      this.ctx.lineTo(x, height);
      this.ctx.stroke();
    }
  }

  /**
   * Update render options
   */
  updateOptions(newOptions: Partial<WaveformRenderOptions>): void {
    this.options = { ...this.options, ...newOptions };

    if (newOptions.width || newOptions.height) {
      this.canvas.width = this.options.width;
      this.canvas.height = this.options.height;
    }
  }

  /**
   * Generate waveform data for thumbnail (downsampled)
   */
  static generateThumbnailData(
    audioBuffer: AudioBuffer,
    width: number
  ): Float32Array {
    const data = audioBuffer.getChannelData(0);
    const step = Math.ceil(data.length / width);
    const thumbnail = new Float32Array(width * 2); // min/max pairs

    for (let i = 0; i < width; i++) {
      let min = 1.0;
      let max = -1.0;

      for (let j = 0; j < step && i * step + j < data.length; j++) {
        const datum = data[i * step + j];
        if (datum < min) min = datum;
        if (datum > max) max = datum;
      }

      thumbnail[i * 2] = max;
      thumbnail[i * 2 + 1] = min;
    }

    return thumbnail;
  }

  /**
   * Create peaks data for efficient waveform rendering at multiple zoom levels
   */
  static generatePeaksData(
    audioBuffer: AudioBuffer,
    peaksPerSecond: number = 20
  ): Float32Array[] {
    const sampleRate = audioBuffer.sampleRate;
    const channelData = audioBuffer.getChannelData(0);
    const samplesPerPeak = Math.floor(sampleRate / peaksPerSecond);

    const peakCount = Math.ceil(channelData.length / samplesPerPeak);
    const peaks = new Float32Array(peakCount * 2); // min/max pairs

    for (let i = 0; i < peakCount; i++) {
      const start = i * samplesPerPeak;
      const end = Math.min(start + samplesPerPeak, channelData.length);

      let min = 1.0;
      let max = -1.0;

      for (let j = start; j < end; j++) {
        const sample = channelData[j];
        if (sample < min) min = sample;
        if (sample > max) max = sample;
      }

      peaks[i * 2] = max;
      peaks[i * 2 + 1] = min;
    }

    return [peaks];
  }
}
