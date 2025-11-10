/**
 * Input Level Meter
 * Real-time input level visualization with peak detection
 */

import { useEffect, useRef } from 'react';
import { motion } from 'framer-motion';
import { useAudioStore } from '../store/audioStore';

interface InputLevelMeterProps {
  trackId: string;
  height?: number;
  width?: number;
  orientation?: 'vertical' | 'horizontal';
}

export default function InputLevelMeter({
  trackId,
  height = 100,
  width = 20,
  orientation = 'vertical',
}: InputLevelMeterProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const inputLevels = useAudioStore((state) => state.inputLevels);

  const level = inputLevels.get(trackId);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d')!;
    const dpr = window.devicePixelRatio || 1;

    // Set canvas size accounting for device pixel ratio
    if (orientation === 'vertical') {
      canvas.width = width * dpr;
      canvas.height = height * dpr;
      canvas.style.width = `${width}px`;
      canvas.style.height = `${height}px`;
    } else {
      canvas.width = height * dpr;
      canvas.height = width * dpr;
      canvas.style.width = `${height}px`;
      canvas.style.height = `${width}px`;
    }

    ctx.scale(dpr, dpr);

    // Clear canvas
    ctx.fillStyle = '#1a1a1a';
    if (orientation === 'vertical') {
      ctx.fillRect(0, 0, width, height);
    } else {
      ctx.fillRect(0, 0, height, width);
    }

    if (!level) return;

    // Convert RMS to dB
    const rmsDB = level.rms > 0 ? 20 * Math.log10(level.rms) : -100;
    const peakDB = level.peak > 0 ? 20 * Math.log10(level.peak) : -100;

    // Map dB to pixel position (-60dB to 0dB)
    const dbToPosition = (db: number) => {
      const normalized = Math.max(0, Math.min(1, (db + 60) / 60));
      return normalized;
    };

    const rmsPosition = dbToPosition(rmsDB);
    const peakPosition = dbToPosition(peakDB);

    if (orientation === 'vertical') {
      // Draw RMS level (bottom-up)
      const rmsHeight = rmsPosition * height;

      // Gradient for level meter
      const gradient = ctx.createLinearGradient(0, height, 0, 0);
      gradient.addColorStop(0, '#10b981'); // Green at bottom
      gradient.addColorStop(0.6, '#f59e0b'); // Yellow
      gradient.addColorStop(0.9, '#ef4444'); // Red at top

      ctx.fillStyle = gradient;
      ctx.fillRect(0, height - rmsHeight, width, rmsHeight);

      // Draw peak indicator
      if (peakPosition > rmsPosition) {
        const peakY = height - (peakPosition * height);
        ctx.fillStyle = level.clipping ? '#ef4444' : '#3b82f6';
        ctx.fillRect(0, peakY - 2, width, 2);
      }

      // Draw clipping indicator
      if (level.clipping) {
        ctx.fillStyle = '#ef4444';
        ctx.fillRect(0, 0, width, 4);
      }

      // Draw scale marks
      ctx.fillStyle = '#4b5563';
      ctx.fillRect(0, height * 0.2, width, 1); // -48dB
      ctx.fillRect(0, height * 0.5, width, 1); // -30dB
      ctx.fillRect(0, height * 0.8, width, 1); // -12dB
      ctx.fillRect(0, height * 0.95, width, 1); // -3dB
    } else {
      // Horizontal orientation (left-to-right)
      const rmsWidth = rmsPosition * height;

      const gradient = ctx.createLinearGradient(0, 0, height, 0);
      gradient.addColorStop(0, '#10b981');
      gradient.addColorStop(0.6, '#f59e0b');
      gradient.addColorStop(0.9, '#ef4444');

      ctx.fillStyle = gradient;
      ctx.fillRect(0, 0, rmsWidth, width);

      // Peak indicator
      if (peakPosition > rmsPosition) {
        const peakX = peakPosition * height;
        ctx.fillStyle = level.clipping ? '#ef4444' : '#3b82f6';
        ctx.fillRect(peakX - 2, 0, 2, width);
      }

      // Clipping indicator
      if (level.clipping) {
        ctx.fillStyle = '#ef4444';
        ctx.fillRect(height - 4, 0, 4, width);
      }
    }
  }, [level, height, width, orientation]);

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      className="relative"
    >
      <canvas
        ref={canvasRef}
        className="rounded-sm border border-border/50"
      />
      {level?.clipping && (
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          className="absolute top-0 left-0 w-full text-xs text-red-500 font-bold text-center bg-red-500/20 px-1"
        >
          CLIP
        </motion.div>
      )}
    </motion.div>
  );
}
