// Waveform Canvas Component for Audio Clip Visualization

import { useEffect, useRef } from 'react';
import { AudioClip } from '../types/clip';
import { WaveformGenerator } from '../lib/waveformGenerator';

interface WaveformCanvasProps {
  clip: AudioClip;
  width: number;
  height: number;
  className?: string;
}

export default function WaveformCanvas({ clip, width, height, className }: WaveformCanvasProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !clip.audioFile?.peaksData) return;

    try {
      WaveformGenerator.renderToCanvas(canvas, clip.audioFile.peaksData, {
        width,
        height,
        waveformColor: clip.color,
        backgroundColor: 'transparent',
        trimStart: clip.trimStart,
        trimEnd: clip.trimEnd,
        fadeIn: clip.fadeIn,
        fadeOut: clip.fadeOut,
      });
    } catch (error) {
      console.error('Failed to render waveform:', error);
    }
  }, [clip, width, height]);

  return (
    <canvas
      ref={canvasRef}
      width={width}
      height={height}
      className={className}
      style={{ width: `${width}px`, height: `${height}px` }}
    />
  );
}
