// Audio Settings Panel for configuring crossfade preferences and other audio settings

import { motion } from 'framer-motion';
import { AudioSettings, CrossfadeCurve, FadeCurve } from '../types/clip';
import { useClipStore } from '../store/clipStore';
import { Settings, X, Check } from 'lucide-react';
import { Button } from './ui/button';

interface AudioSettingsPanelProps {
  onClose: () => void;
}

export default function AudioSettingsPanel({ onClose }: AudioSettingsPanelProps) {
  const { audioSettings, updateAudioSettings } = useClipStore();

  return (
    <motion.div
      initial={{ opacity: 0, scale: 0.95 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.95 }}
      className="fixed top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-96 bg-card border border-border rounded-lg shadow-2xl z-50"
    >
      {/* Header */}
      <div className="h-12 border-b border-border/50 flex items-center justify-between px-4 bg-card/50 backdrop-blur-sm">
        <div className="flex items-center gap-2">
          <Settings className="h-4 w-4" />
          <h3 className="font-semibold text-sm">Audio Settings</h3>
        </div>
        <button
          onClick={onClose}
          className="p-1 hover:bg-accent rounded transition-colors"
        >
          <X className="h-4 w-4" />
        </button>
      </div>

      {/* Content */}
      <div className="p-4 space-y-4 max-h-[600px] overflow-y-auto">
        {/* Auto-Crossfade */}
        <div>
          <div className="flex items-center justify-between mb-3">
            <label className="text-sm font-medium">Automatic Crossfade</label>
            <button
              onClick={() =>
                updateAudioSettings({
                  autoCrossfadeEnabled: !audioSettings.autoCrossfadeEnabled,
                })
              }
              className={`relative w-11 h-6 rounded-full transition-colors ${
                audioSettings.autoCrossfadeEnabled ? 'bg-primary' : 'bg-muted'
              }`}
            >
              <motion.div
                className="absolute top-0.5 left-0.5 w-5 h-5 bg-white rounded-full shadow-md"
                animate={{
                  x: audioSettings.autoCrossfadeEnabled ? 20 : 0,
                }}
                transition={{ type: 'spring', stiffness: 500, damping: 30 }}
              />
            </button>
          </div>
          <p className="text-xs text-muted-foreground">
            Automatically create crossfades when clips overlap
          </p>
        </div>

        {/* Crossfade Duration */}
        <div>
          <div className="flex items-center justify-between mb-2">
            <label className="text-sm font-medium">Default Crossfade Duration</label>
            <span className="text-xs text-muted-foreground">
              {(audioSettings.defaultCrossfadeDuration * 1000).toFixed(0)}ms
            </span>
          </div>
          <input
            type="range"
            min="0.001"
            max="1"
            step="0.001"
            value={audioSettings.defaultCrossfadeDuration}
            onChange={(e) =>
              updateAudioSettings({
                defaultCrossfadeDuration: parseFloat(e.target.value),
              })
            }
            className="w-full"
            disabled={!audioSettings.autoCrossfadeEnabled}
          />
          <div className="flex justify-between text-xs text-muted-foreground mt-1">
            <span>1ms</span>
            <span>500ms</span>
            <span>1000ms</span>
          </div>
        </div>

        {/* Crossfade Curve */}
        <div>
          <label className="text-sm font-medium block mb-2">
            Default Crossfade Curve
          </label>
          <div className="grid grid-cols-3 gap-2">
            {(['linear', 'equalPower', 'logarithmic'] as CrossfadeCurve[]).map((curve) => (
              <button
                key={curve}
                onClick={() => updateAudioSettings({ defaultCrossfadeCurve: curve })}
                className={`px-3 py-2 rounded text-xs font-medium transition-all ${
                  audioSettings.defaultCrossfadeCurve === curve
                    ? 'bg-primary text-primary-foreground'
                    : 'bg-muted hover:bg-muted/80 text-muted-foreground'
                }`}
                disabled={!audioSettings.autoCrossfadeEnabled}
              >
                {curve === 'equalPower' ? 'Equal Power' : curve.charAt(0).toUpperCase() + curve.slice(1)}
              </button>
            ))}
          </div>
          <p className="text-xs text-muted-foreground mt-2">
            {audioSettings.defaultCrossfadeCurve === 'equalPower' &&
              'Recommended for most cases - maintains constant perceived volume'}
            {audioSettings.defaultCrossfadeCurve === 'linear' &&
              'Best for coherent material (same source)'}
            {audioSettings.defaultCrossfadeCurve === 'logarithmic' &&
              'Smooth transition with gentle curves'}
          </p>
        </div>

        {/* Default Fade Curve */}
        <div className="pt-3 border-t border-border/30">
          <label className="text-sm font-medium block mb-2">
            Default Fade Curve
          </label>
          <div className="grid grid-cols-2 gap-2">
            {(['linear', 'exponential', 'logarithmic', 'sCurve'] as FadeCurve[]).map((curve) => (
              <button
                key={curve}
                onClick={() => updateAudioSettings({ defaultFadeCurve: curve })}
                className={`px-3 py-2 rounded text-xs font-medium transition-all ${
                  audioSettings.defaultFadeCurve === curve
                    ? 'bg-primary text-primary-foreground'
                    : 'bg-muted hover:bg-muted/80 text-muted-foreground'
                }`}
              >
                {curve === 'sCurve' ? 'S-Curve' : curve.charAt(0).toUpperCase() + curve.slice(1)}
              </button>
            ))}
          </div>
        </div>

        {/* Waveform Display */}
        <div className="pt-3 border-t border-border/30">
          <label className="text-sm font-medium block mb-2">Waveform Color</label>
          <div className="flex items-center gap-2">
            <input
              type="color"
              value={audioSettings.waveformColor}
              onChange={(e) => updateAudioSettings({ waveformColor: e.target.value })}
              className="w-12 h-8 rounded border border-border cursor-pointer"
            />
            <span className="text-xs text-muted-foreground font-mono">
              {audioSettings.waveformColor}
            </span>
          </div>
        </div>

        {/* Peaks Cache */}
        <div className="pt-3 border-t border-border/30">
          <div className="flex items-center justify-between mb-3">
            <label className="text-sm font-medium">Waveform Cache</label>
            <button
              onClick={() =>
                updateAudioSettings({
                  peaksCacheEnabled: !audioSettings.peaksCacheEnabled,
                })
              }
              className={`relative w-11 h-6 rounded-full transition-colors ${
                audioSettings.peaksCacheEnabled ? 'bg-primary' : 'bg-muted'
              }`}
            >
              <motion.div
                className="absolute top-0.5 left-0.5 w-5 h-5 bg-white rounded-full shadow-md"
                animate={{
                  x: audioSettings.peaksCacheEnabled ? 20 : 0,
                }}
                transition={{ type: 'spring', stiffness: 500, damping: 30 }}
              />
            </button>
          </div>
          <p className="text-xs text-muted-foreground mb-2">
            Cache waveform data for faster rendering
          </p>

          {audioSettings.peaksCacheEnabled && (
            <div>
              <div className="flex items-center justify-between mb-2">
                <label className="text-xs text-muted-foreground">Cache Duration</label>
                <span className="text-xs font-medium">{audioSettings.peaksCacheDays} days</span>
              </div>
              <input
                type="range"
                min="1"
                max="30"
                step="1"
                value={audioSettings.peaksCacheDays}
                onChange={(e) =>
                  updateAudioSettings({
                    peaksCacheDays: parseInt(e.target.value),
                  })
                }
                className="w-full"
              />
            </div>
          )}
        </div>
      </div>

      {/* Footer */}
      <div className="h-12 border-t border-border/30 flex items-center justify-end px-4 gap-2">
        <Button size="sm" variant="outline" onClick={onClose}>
          Close
        </Button>
      </div>
    </motion.div>
  );
}
