/**
 * Advanced Quantization Panel
 * Combines Ableton Live and Logic Pro quantization features
 */

import { useState, useEffect } from 'react';
import { motion } from 'framer-motion';
import * as Slider from '@radix-ui/react-slider';
import * as Select from '@radix-ui/react-dropdown-menu';
import { Check, ChevronDown } from 'lucide-react';
import { useAudioStore } from '../store/audioStore';
import { Button } from './ui/button';

export default function QuantizationPanel() {
  const quantizationSettings = useAudioStore((state) => state.quantizationSettings);
  const updateQuantizationSettings = useAudioStore((state) => state.updateQuantizationSettings);

  const [gridValue, setGridValue] = useState(quantizationSettings.gridValue);
  const [strength, setStrength] = useState(quantizationSettings.strength);
  const [swing, setSwing] = useState(quantizationSettings.swing);

  // Grid value options
  const gridOptions = [
    { label: '1/4', value: 1 / 4 },
    { label: '1/8', value: 1 / 8 },
    { label: '1/16', value: 1 / 16 },
    { label: '1/32', value: 1 / 32 },
    { label: '1/64', value: 1 / 64 },
  ];

  // Scale options
  const scaleOptions = [
    { label: 'Major', value: 'major' },
    { label: 'Minor', value: 'minor' },
    { label: 'Dorian', value: 'dorian' },
    { label: 'Phrygian', value: 'phrygian' },
    { label: 'Lydian', value: 'lydian' },
    { label: 'Mixolydian', value: 'mixolydian' },
    { label: 'Locrian', value: 'locrian' },
    { label: 'Harmonic Minor', value: 'harmonic-minor' },
    { label: 'Melodic Minor', value: 'melodic-minor' },
  ] as const;

  // Root note options
  const rootOptions = [
    { label: 'C', value: 0 },
    { label: 'C#', value: 1 },
    { label: 'D', value: 2 },
    { label: 'D#', value: 3 },
    { label: 'E', value: 4 },
    { label: 'F', value: 5 },
    { label: 'F#', value: 6 },
    { label: 'G', value: 7 },
    { label: 'G#', value: 8 },
    { label: 'A', value: 9 },
    { label: 'A#', value: 10 },
    { label: 'B', value: 11 },
  ];

  const handleStrengthChange = (value: number[]) => {
    setStrength(value[0]);
    updateQuantizationSettings({ strength: value[0] });
  };

  const handleSwingChange = (value: number[]) => {
    setSwing(value[0]);
    updateQuantizationSettings({ swing: value[0] });
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: -10 }}
      animate={{ opacity: 1, y: 0 }}
      className="bg-card/95 backdrop-blur-xl border border-border/50 rounded-lg p-4 space-y-4"
    >
      {/* Header */}
      <div className="flex items-center justify-between">
        <h3 className="text-sm font-semibold text-foreground">Quantization</h3>
        <Button
          size="sm"
          variant={quantizationSettings.enabled ? 'default' : 'outline'}
          onClick={() =>
            updateQuantizationSettings({ enabled: !quantizationSettings.enabled })
          }
          className="h-7 text-xs"
        >
          {quantizationSettings.enabled ? 'On' : 'Off'}
        </Button>
      </div>

      {/* Grid Value Selection */}
      <div className="space-y-2">
        <label className="text-xs text-muted-foreground">Grid</label>
        <div className="flex gap-1">
          {gridOptions.map((option) => (
            <button
              key={option.value}
              onClick={() => {
                setGridValue(option.value);
                updateQuantizationSettings({ gridValue: option.value });
              }}
              className={`flex-1 px-2 py-1.5 text-xs rounded transition-all ${
                gridValue === option.value
                  ? 'bg-primary text-primary-foreground'
                  : 'bg-muted/50 hover:bg-muted text-muted-foreground'
              }`}
            >
              {option.label}
            </button>
          ))}
        </div>

        {/* Triplets Toggle */}
        <label className="flex items-center gap-2 text-xs cursor-pointer">
          <input
            type="checkbox"
            checked={quantizationSettings.triplets}
            onChange={(e) =>
              updateQuantizationSettings({ triplets: e.target.checked })
            }
            className="w-3.5 h-3.5 rounded border-border bg-background checked:bg-primary"
          />
          <span className="text-muted-foreground">Triplets</span>
        </label>
      </div>

      {/* Ableton-style: Quantize Start/End */}
      <div className="space-y-2">
        <label className="text-xs text-muted-foreground">Quantize (Ableton Style)</label>
        <div className="flex gap-2">
          <label className="flex items-center gap-2 text-xs cursor-pointer flex-1 p-2 rounded bg-muted/30 hover:bg-muted/50 transition-colors">
            <input
              type="checkbox"
              checked={quantizationSettings.quantizeNoteStart}
              onChange={(e) =>
                updateQuantizationSettings({ quantizeNoteStart: e.target.checked })
              }
              className="w-3.5 h-3.5 rounded border-border bg-background checked:bg-primary"
            />
            <span>Note Start</span>
          </label>

          <label className="flex items-center gap-2 text-xs cursor-pointer flex-1 p-2 rounded bg-muted/30 hover:bg-muted/50 transition-colors">
            <input
              type="checkbox"
              checked={quantizationSettings.quantizeNoteEnd}
              onChange={(e) =>
                updateQuantizationSettings({ quantizeNoteEnd: e.target.checked })
              }
              className="w-3.5 h-3.5 rounded border-border bg-background checked:bg-primary"
            />
            <span>Note End</span>
          </label>
        </div>
      </div>

      {/* Logic Pro-style: Q-Strength Slider */}
      <div className="space-y-2">
        <div className="flex justify-between items-center">
          <label className="text-xs text-muted-foreground">Q-Strength (Logic Style)</label>
          <span className="text-xs font-mono text-foreground">{strength}%</span>
        </div>
        <Slider.Root
          className="relative flex items-center select-none touch-none w-full h-5"
          value={[strength]}
          onValueChange={handleStrengthChange}
          max={100}
          step={1}
        >
          <Slider.Track className="bg-muted relative grow rounded-full h-1">
            <Slider.Range className="absolute bg-primary rounded-full h-full" />
          </Slider.Track>
          <Slider.Thumb
            className="block w-4 h-4 bg-primary rounded-full hover:bg-primary/90 focus:outline-none focus:ring-2 focus:ring-primary focus:ring-offset-2 focus:ring-offset-background shadow-lg"
            aria-label="Q-Strength"
          />
        </Slider.Root>
        <div className="flex justify-between text-xs text-muted-foreground">
          <span>0% (None)</span>
          <span>100% (Full)</span>
        </div>
      </div>

      {/* Logic Pro-style: Q-Swing Slider */}
      <div className="space-y-2">
        <div className="flex justify-between items-center">
          <label className="text-xs text-muted-foreground">Q-Swing (Logic Style)</label>
          <span className="text-xs font-mono text-foreground">{swing}%</span>
        </div>
        <Slider.Root
          className="relative flex items-center select-none touch-none w-full h-5"
          value={[swing]}
          onValueChange={handleSwingChange}
          max={99}
          step={1}
        >
          <Slider.Track className="bg-muted relative grow rounded-full h-1">
            <Slider.Range
              className="absolute rounded-full h-full"
              style={{
                backgroundColor: swing === 50 ? '#6b7280' : swing > 50 ? '#3b82f6' : '#ef4444',
              }}
            />
          </Slider.Track>
          <Slider.Thumb
            className="block w-4 h-4 bg-primary rounded-full hover:bg-primary/90 focus:outline-none focus:ring-2 focus:ring-primary focus:ring-offset-2 focus:ring-offset-background shadow-lg"
            aria-label="Q-Swing"
          />
        </Slider.Root>
        <div className="flex justify-between text-xs text-muted-foreground">
          <span>0% (Early)</span>
          <span className="font-semibold">50% (None)</span>
          <span>99% (Late)</span>
        </div>
      </div>

      {/* Scale Quantization */}
      <div className="space-y-2 border-t border-border/50 pt-3">
        <div className="flex items-center justify-between">
          <label className="text-xs text-muted-foreground">Scale Quantization</label>
          <Button
            size="sm"
            variant={quantizationSettings.scaleQuantization.enabled ? 'default' : 'outline'}
            onClick={() =>
              updateQuantizationSettings({
                scaleQuantization: {
                  ...quantizationSettings.scaleQuantization,
                  enabled: !quantizationSettings.scaleQuantization.enabled,
                },
              })
            }
            className="h-6 text-xs"
          >
            {quantizationSettings.scaleQuantization.enabled ? 'On' : 'Off'}
          </Button>
        </div>

        {quantizationSettings.scaleQuantization.enabled && (
          <div className="space-y-2">
            {/* Root Selection */}
            <div>
              <label className="text-xs text-muted-foreground mb-1 block">Root Note</label>
              <div className="grid grid-cols-6 gap-1">
                {rootOptions.map((option) => (
                  <button
                    key={option.value}
                    onClick={() =>
                      updateQuantizationSettings({
                        scaleQuantization: {
                          ...quantizationSettings.scaleQuantization,
                          root: option.value,
                        },
                      })
                    }
                    className={`px-2 py-1 text-xs rounded transition-all ${
                      quantizationSettings.scaleQuantization.root === option.value
                        ? 'bg-primary text-primary-foreground'
                        : 'bg-muted/50 hover:bg-muted text-muted-foreground'
                    }`}
                  >
                    {option.label}
                  </button>
                ))}
              </div>
            </div>

            {/* Scale Selection */}
            <div>
              <label className="text-xs text-muted-foreground mb-1 block">Scale Type</label>
              <select
                value={quantizationSettings.scaleQuantization.scale}
                onChange={(e) =>
                  updateQuantizationSettings({
                    scaleQuantization: {
                      ...quantizationSettings.scaleQuantization,
                      scale: e.target.value as any,
                    },
                  })
                }
                className="w-full px-3 py-1.5 text-xs bg-muted border border-border rounded focus:outline-none focus:ring-2 focus:ring-primary"
              >
                {scaleOptions.map((option) => (
                  <option key={option.value} value={option.value}>
                    {option.label}
                  </option>
                ))}
              </select>
            </div>
          </div>
        )}
      </div>

      {/* Quick Actions */}
      <div className="flex gap-2 border-t border-border/50 pt-3">
        <Button
          size="sm"
          variant="outline"
          className="flex-1 text-xs"
          onClick={() => {
            // Reset to defaults
            updateQuantizationSettings({
              gridValue: 1 / 16,
              strength: 100,
              swing: 50,
              quantizeNoteStart: true,
              quantizeNoteEnd: false,
              scaleQuantization: {
                enabled: false,
                root: 0,
                scale: 'major',
              },
              triplets: false,
            });
            setGridValue(1 / 16);
            setStrength(100);
            setSwing(50);
          }}
        >
          Reset
        </Button>
      </div>
    </motion.div>
  );
}
