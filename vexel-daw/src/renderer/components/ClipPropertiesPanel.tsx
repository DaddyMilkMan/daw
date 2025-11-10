// Clip Properties Panel for time-stretch, pitch shift, and other clip settings

import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { AudioClip, FadeCurve } from '../types/clip';
import { useClipStore } from '../store/clipStore';
import {
  TrendingUp,
  Music2,
  Volume2,
  Sliders,
  X,
  Lock,
  Unlock,
  Volume,
  VolumeX,
} from 'lucide-react';
import { Button } from './ui/button';

interface ClipPropertiesPanelProps {
  clip: AudioClip;
  onClose: () => void;
}

export default function ClipPropertiesPanel({ clip, onClose }: ClipPropertiesPanelProps) {
  const { updateClip, setClipPlaybackRate, setClipPitchShift, setClipGain, setClipFade } = useClipStore();

  const [localPlaybackRate, setLocalPlaybackRate] = useState(clip.playbackRate);
  const [localPitchShift, setLocalPitchShift] = useState(clip.pitchShift);
  const [localGain, setLocalGain] = useState(clip.gain);
  const [localFadeIn, setLocalFadeIn] = useState(clip.fadeIn);
  const [localFadeOut, setLocalFadeOut] = useState(clip.fadeOut);

  const handlePlaybackRateChange = (value: number) => {
    setLocalPlaybackRate(value);
    setClipPlaybackRate(clip.id, value);
  };

  const handlePitchShiftChange = (value: number) => {
    setLocalPitchShift(value);
    setClipPitchShift(clip.id, value);
  };

  const handleGainChange = (value: number) => {
    setLocalGain(value);
    setClipGain(clip.id, value);
  };

  const handleFadeInChange = (value: number) => {
    setLocalFadeIn(value);
    setClipFade(clip.id, 'fadeIn', value);
  };

  const handleFadeOutChange = (value: number) => {
    setLocalFadeOut(value);
    setClipFade(clip.id, 'fadeOut', value);
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: 20 }}
      className="fixed bottom-20 right-4 w-80 bg-card border border-border rounded-lg shadow-2xl z-50"
    >
      {/* Header */}
      <div className="h-12 border-b border-border/50 flex items-center justify-between px-4 bg-card/50 backdrop-blur-sm">
        <div className="flex items-center gap-2">
          <Sliders className="h-4 w-4" />
          <h3 className="font-semibold text-sm">Clip Properties</h3>
        </div>
        <button
          onClick={onClose}
          className="p-1 hover:bg-accent rounded transition-colors"
        >
          <X className="h-4 w-4" />
        </button>
      </div>

      {/* Content */}
      <div className="p-4 space-y-4 max-h-96 overflow-y-auto">
        {/* Clip Info */}
        <div className="pb-3 border-b border-border/30">
          <div className="text-sm font-medium truncate">{clip.name}</div>
          <div className="text-xs text-muted-foreground mt-1">
            {clip.audioFile?.duration.toFixed(2)}s • {clip.audioFile?.sampleRate}Hz
          </div>
        </div>

        {/* Playback Rate / Time-Stretch */}
        <div>
          <div className="flex items-center justify-between mb-2">
            <label className="text-sm font-medium flex items-center gap-2">
              <TrendingUp className="h-4 w-4" />
              Playback Rate
            </label>
            <span className="text-xs text-muted-foreground">
              {(localPlaybackRate * 100).toFixed(0)}%
            </span>
          </div>
          <input
            type="range"
            min="0.25"
            max="4"
            step="0.01"
            value={localPlaybackRate}
            onChange={(e) => handlePlaybackRateChange(parseFloat(e.target.value))}
            className="w-full"
          />
          <div className="flex justify-between text-xs text-muted-foreground mt-1">
            <span>25%</span>
            <span>100%</span>
            <span>400%</span>
          </div>
        </div>

        {/* Pitch Shift */}
        <div>
          <div className="flex items-center justify-between mb-2">
            <label className="text-sm font-medium flex items-center gap-2">
              <Music2 className="h-4 w-4" />
              Pitch Shift
            </label>
            <span className="text-xs text-muted-foreground">
              {localPitchShift > 0 ? '+' : ''}
              {localPitchShift} st
            </span>
          </div>
          <input
            type="range"
            min="-12"
            max="12"
            step="1"
            value={localPitchShift}
            onChange={(e) => handlePitchShiftChange(parseInt(e.target.value))}
            className="w-full"
          />
          <div className="flex justify-between text-xs text-muted-foreground mt-1">
            <span>-12</span>
            <span>0</span>
            <span>+12</span>
          </div>
        </div>

        {/* Gain */}
        <div>
          <div className="flex items-center justify-between mb-2">
            <label className="text-sm font-medium flex items-center gap-2">
              <Volume2 className="h-4 w-4" />
              Gain
            </label>
            <span className="text-xs text-muted-foreground">
              {(localGain * 100).toFixed(0)}%
            </span>
          </div>
          <input
            type="range"
            min="0"
            max="2"
            step="0.01"
            value={localGain}
            onChange={(e) => handleGainChange(parseFloat(e.target.value))}
            className="w-full"
          />
          <div className="flex justify-between text-xs text-muted-foreground mt-1">
            <span>0%</span>
            <span>100%</span>
            <span>200%</span>
          </div>
        </div>

        {/* Fade In */}
        <div>
          <div className="flex items-center justify-between mb-2">
            <label className="text-sm font-medium">Fade In</label>
            <span className="text-xs text-muted-foreground">
              {localFadeIn.toFixed(3)}s
            </span>
          </div>
          <input
            type="range"
            min="0"
            max="2"
            step="0.001"
            value={localFadeIn}
            onChange={(e) => handleFadeInChange(parseFloat(e.target.value))}
            className="w-full"
          />
        </div>

        {/* Fade Out */}
        <div>
          <div className="flex items-center justify-between mb-2">
            <label className="text-sm font-medium">Fade Out</label>
            <span className="text-xs text-muted-foreground">
              {localFadeOut.toFixed(3)}s
            </span>
          </div>
          <input
            type="range"
            min="0"
            max="2"
            step="0.001"
            value={localFadeOut}
            onChange={(e) => handleFadeOutChange(parseFloat(e.target.value))}
            className="w-full"
          />
        </div>

        {/* Quick Actions */}
        <div className="pt-3 border-t border-border/30 flex gap-2">
          <Button
            size="sm"
            variant="outline"
            onClick={() => updateClip(clip.id, { isMuted: !clip.isMuted })}
            className="flex-1"
          >
            {clip.isMuted ? <Volume className="h-4 w-4 mr-2" /> : <VolumeX className="h-4 w-4 mr-2" />}
            {clip.isMuted ? 'Unmute' : 'Mute'}
          </Button>
          <Button
            size="sm"
            variant="outline"
            onClick={() => updateClip(clip.id, { isLocked: !clip.isLocked })}
            className="flex-1"
          >
            {clip.isLocked ? <Unlock className="h-4 w-4 mr-2" /> : <Lock className="h-4 w-4 mr-2" />}
            {clip.isLocked ? 'Unlock' : 'Lock'}
          </Button>
        </div>
      </div>
    </motion.div>
  );
}
