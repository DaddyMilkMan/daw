/**
 * Time Stretching and Pitch Shifting Controls
 * UI for adjusting tempo and pitch of audio clips
 */

import { useState } from 'react';
import { motion } from 'framer-motion';
import { Clock, Music, Wand2, RotateCcw } from 'lucide-react';
import { Button } from './ui/button';
import * as Slider from '@radix-ui/react-slider';
import { timeStretchService } from '../services/timeStretch';
import { AudioClip } from '../types/recording';
import { useAudioStore } from '../store/audioStore';

interface TimePitchControlsProps {
  clip: AudioClip;
  onClose: () => void;
}

export default function TimePitchControls({ clip, onClose }: TimePitchControlsProps) {
  const [timeStretchRate, setTimeStretchRate] = useState(1.0);
  const [pitchShift, setPitchShift] = useState(0);
  const [preservePitch, setPreservePitch] = useState(true);
  const [preserveTempo, setPreserveTempo] = useState(true);
  const [isProcessing, setIsProcessing] = useState(false);

  const handleApply = async () => {
    if (!clip.audioBuffer) {
      alert('No audio buffer available');
      return;
    }

    setIsProcessing(true);

    try {
      let processedBuffer = clip.audioBuffer;

      // Apply time stretching if rate changed
      if (Math.abs(timeStretchRate - 1.0) > 0.01) {
        processedBuffer = await timeStretchService.timeStretch(processedBuffer, {
          rate: timeStretchRate,
          preservePitch,
        });
      }

      // Apply pitch shifting if changed
      if (Math.abs(pitchShift) > 0.01) {
        processedBuffer = await timeStretchService.pitchShift(processedBuffer, {
          semitones: pitchShift,
          preserveTempo,
        });
      }

      // Update clip with processed buffer
      const store = useAudioStore.getState();
      const updatedClips = store.audioClips.map((c) =>
        c.id === clip.id ? { ...c, audioBuffer: processedBuffer } : c
      );
      useAudioStore.setState({ audioClips: updatedClips });

      alert('Processing complete!');
      onClose();
    } catch (error) {
      console.error('Failed to process audio:', error);
      alert('Failed to process audio');
    } finally {
      setIsProcessing(false);
    }
  };

  const handleReset = () => {
    setTimeStretchRate(1.0);
    setPitchShift(0);
    setPreservePitch(true);
    setPreserveTempo(true);
  };

  // Calculate resulting tempo and pitch
  const audioState = useAudioStore((state) => state.audioState);
  const originalTempo = audioState.tempo || 120;
  const resultingTempo = preserveTempo ? originalTempo : originalTempo / timeStretchRate;
  const resultingPitch = preservePitch ? pitchShift : pitchShift + (12 * Math.log2(timeStretchRate));

  return (
    <motion.div
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.95 }}
      className="bg-card border border-border rounded-lg shadow-2xl w-full max-w-2xl p-6"
    >
      {/* Header */}
      <div className="flex items-center justify-between mb-6">
        <div>
          <h2 className="text-2xl font-bold text-foreground flex items-center gap-2">
            <Wand2 className="h-6 w-6 text-primary" />
            Time & Pitch
          </h2>
          <p className="text-sm text-muted-foreground mt-1">
            Adjust tempo and pitch of: {clip.name}
          </p>
        </div>
      </div>

      {/* Time Stretch Section */}
      <div className="mb-8">
        <div className="flex items-center gap-2 mb-3">
          <Clock className="h-5 w-5 text-primary" />
          <h3 className="text-lg font-semibold text-foreground">Time Stretch</h3>
        </div>

        <div className="space-y-4">
          <div>
            <div className="flex items-center justify-between mb-2">
              <label className="text-sm text-muted-foreground">Playback Rate</label>
              <div className="flex items-center gap-2">
                <span className="text-sm font-mono text-foreground">
                  {timeStretchRate.toFixed(2)}x
                </span>
                <span className="text-xs text-muted-foreground">
                  ({timeStretchRate < 1 ? 'slower' : timeStretchRate > 1 ? 'faster' : 'normal'})
                </span>
              </div>
            </div>
            <Slider.Root
              className="relative flex items-center select-none touch-none w-full h-5"
              value={[timeStretchRate]}
              onValueChange={([value]) => setTimeStretchRate(value)}
              min={0.5}
              max={2.0}
              step={0.01}
            >
              <Slider.Track className="bg-muted relative grow rounded-full h-2">
                <Slider.Range className="absolute bg-primary rounded-full h-full" />
              </Slider.Track>
              <Slider.Thumb
                className="block w-5 h-5 bg-primary border-2 border-background rounded-full hover:bg-primary/90 focus:outline-none focus:ring-2 focus:ring-primary"
                aria-label="Time stretch rate"
              />
            </Slider.Root>
            <div className="flex justify-between text-xs text-muted-foreground mt-1">
              <span>0.5x (Half speed)</span>
              <span>1.0x (Normal)</span>
              <span>2.0x (Double speed)</span>
            </div>
          </div>

          <div className="flex items-center gap-2">
            <input
              type="checkbox"
              id="preservePitch"
              checked={preservePitch}
              onChange={(e) => setPreservePitch(e.target.checked)}
              className="w-4 h-4 text-primary bg-background border-border rounded focus:ring-2 focus:ring-primary"
            />
            <label htmlFor="preservePitch" className="text-sm text-foreground cursor-pointer">
              Preserve pitch (recommended)
            </label>
          </div>
        </div>
      </div>

      {/* Pitch Shift Section */}
      <div className="mb-8">
        <div className="flex items-center gap-2 mb-3">
          <Music className="h-5 w-5 text-primary" />
          <h3 className="text-lg font-semibold text-foreground">Pitch Shift</h3>
        </div>

        <div className="space-y-4">
          <div>
            <div className="flex items-center justify-between mb-2">
              <label className="text-sm text-muted-foreground">Semitones</label>
              <div className="flex items-center gap-2">
                <span className="text-sm font-mono text-foreground">
                  {pitchShift > 0 ? '+' : ''}{pitchShift.toFixed(1)}
                </span>
                <span className="text-xs text-muted-foreground">semitones</span>
              </div>
            </div>
            <Slider.Root
              className="relative flex items-center select-none touch-none w-full h-5"
              value={[pitchShift]}
              onValueChange={([value]) => setPitchShift(value)}
              min={-12}
              max={12}
              step={0.1}
            >
              <Slider.Track className="bg-muted relative grow rounded-full h-2">
                <Slider.Range className="absolute bg-primary rounded-full h-full" />
              </Slider.Track>
              <Slider.Thumb
                className="block w-5 h-5 bg-primary border-2 border-background rounded-full hover:bg-primary/90 focus:outline-none focus:ring-2 focus:ring-primary"
                aria-label="Pitch shift"
              />
            </Slider.Root>
            <div className="flex justify-between text-xs text-muted-foreground mt-1">
              <span>-12 (1 octave down)</span>
              <span>0 (Original)</span>
              <span>+12 (1 octave up)</span>
            </div>
          </div>

          <div className="flex items-center gap-2">
            <input
              type="checkbox"
              id="preserveTempo"
              checked={preserveTempo}
              onChange={(e) => setPreserveTempo(e.target.checked)}
              className="w-4 h-4 text-primary bg-background border-border rounded focus:ring-2 focus:ring-primary"
            />
            <label htmlFor="preserveTempo" className="text-sm text-foreground cursor-pointer">
              Preserve tempo (recommended)
            </label>
          </div>
        </div>
      </div>

      {/* Preview Info */}
      <div className="bg-muted/50 rounded-lg p-4 mb-6">
        <h4 className="text-sm font-semibold text-foreground mb-2">Preview</h4>
        <div className="space-y-1 text-sm">
          <div className="flex justify-between">
            <span className="text-muted-foreground">Original Duration:</span>
            <span className="font-mono text-foreground">
              {(clip.length * 60 / 120).toFixed(2)}s
            </span>
          </div>
          <div className="flex justify-between">
            <span className="text-muted-foreground">New Duration:</span>
            <span className="font-mono text-foreground">
              {((clip.length * 60 / 120) / timeStretchRate).toFixed(2)}s
            </span>
          </div>
          <div className="flex justify-between">
            <span className="text-muted-foreground">Pitch Change:</span>
            <span className="font-mono text-foreground">
              {resultingPitch > 0 ? '+' : ''}{resultingPitch.toFixed(1)} semitones
            </span>
          </div>
        </div>
      </div>

      {/* Actions */}
      <div className="flex items-center justify-between gap-3">
        <Button variant="outline" onClick={handleReset} disabled={isProcessing}>
          <RotateCcw className="h-4 w-4 mr-2" />
          Reset
        </Button>

        <div className="flex gap-3">
          <Button variant="ghost" onClick={onClose} disabled={isProcessing}>
            Cancel
          </Button>
          <Button
            onClick={handleApply}
            disabled={isProcessing}
            className="bg-primary hover:bg-primary/90"
          >
            {isProcessing ? (
              <>
                <motion.div
                  animate={{ rotate: 360 }}
                  transition={{ duration: 1, repeat: Infinity, ease: 'linear' }}
                  className="mr-2"
                >
                  <Wand2 className="h-4 w-4" />
                </motion.div>
                Processing...
              </>
            ) : (
              <>
                <Wand2 className="h-4 w-4 mr-2" />
                Apply
              </>
            )}
          </Button>
        </div>
      </div>

      {/* Note */}
      <div className="mt-4 p-3 bg-yellow-500/10 border border-yellow-500/30 rounded-md">
        <p className="text-xs text-yellow-600 dark:text-yellow-400">
          <strong>Note:</strong> Time stretching and pitch shifting may take several seconds for
          large audio files. The quality depends on the WSOLA algorithm parameters.
        </p>
      </div>
    </motion.div>
  );
}
