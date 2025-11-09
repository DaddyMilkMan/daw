/**
 * Metronome Settings Panel
 * Comprehensive settings for metronome and MIDI clock
 */

import { motion } from 'framer-motion';
import { X, Upload, Volume2, Clock, Disc, Settings as SettingsIcon } from 'lucide-react';
import { Button } from './ui/button';
import { MetronomeSettings as MetronomeSettingsType, MIDIClockSettings, MetronomeSoundType } from '../types/metronome';
import { useState } from 'react';

interface MetronomeSettingsProps {
  settings: MetronomeSettingsType;
  midiSettings: MIDIClockSettings;
  onSettingsChange: (settings: Partial<MetronomeSettingsType>) => void;
  onMIDISettingsChange: (settings: Partial<MIDIClockSettings>) => void;
  onClose: () => void;
  onLoadCustomSound?: (file: File) => void;
}

export default function MetronomeSettings({
  settings,
  midiSettings,
  onSettingsChange,
  onMIDISettingsChange,
  onClose,
  onLoadCustomSound,
}: MetronomeSettingsProps) {
  const [activeTab, setActiveTab] = useState<'sound' | 'precount' | 'midi'>('sound');

  const handleSoundTypeChange = (soundType: MetronomeSoundType) => {
    onSettingsChange({ soundType });
  };

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file && onLoadCustomSound) {
      onLoadCustomSound(file);
      onSettingsChange({ soundType: 'custom' });
    }
  };

  const handleCustomTimeSignature = (value: string) => {
    // Parse custom time signature grouping like "2+2+3" for 7/8
    const groups = value.split('+').map(n => parseInt(n.trim())).filter(n => !isNaN(n));
    onSettingsChange({ accentGrouping: groups });
  };

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      className="fixed inset-0 bg-black/60 backdrop-blur-sm z-50 flex items-center justify-center"
      onClick={onClose}
    >
      <motion.div
        initial={{ scale: 0.9, opacity: 0 }}
        animate={{ scale: 1, opacity: 1 }}
        exit={{ scale: 0.9, opacity: 0 }}
        onClick={(e) => e.stopPropagation()}
        className="bg-card border border-border rounded-xl shadow-2xl w-full max-w-2xl max-h-[80vh] overflow-hidden flex flex-col"
      >
        {/* Header */}
        <div className="flex items-center justify-between p-6 border-b border-border">
          <div className="flex items-center gap-3">
            <SettingsIcon className="h-6 w-6 text-primary" />
            <h2 className="text-2xl font-bold">Metronome Settings</h2>
          </div>
          <Button size="icon" variant="ghost" onClick={onClose}>
            <X className="h-5 w-5" />
          </Button>
        </div>

        {/* Tabs */}
        <div className="flex gap-1 p-4 border-b border-border bg-secondary/20">
          <button
            onClick={() => setActiveTab('sound')}
            className={`px-4 py-2 rounded-lg transition-all ${
              activeTab === 'sound'
                ? 'bg-primary text-primary-foreground'
                : 'hover:bg-secondary/50'
            }`}
          >
            <Volume2 className="h-4 w-4 inline mr-2" />
            Sound
          </button>
          <button
            onClick={() => setActiveTab('precount')}
            className={`px-4 py-2 rounded-lg transition-all ${
              activeTab === 'precount'
                ? 'bg-primary text-primary-foreground'
                : 'hover:bg-secondary/50'
            }`}
          >
            <Clock className="h-4 w-4 inline mr-2" />
            Pre-Count
          </button>
          <button
            onClick={() => setActiveTab('midi')}
            className={`px-4 py-2 rounded-lg transition-all ${
              activeTab === 'midi'
                ? 'bg-primary text-primary-foreground'
                : 'hover:bg-secondary/50'
            }`}
          >
            <Disc className="h-4 w-4 inline mr-2" />
            MIDI Clock
          </button>
        </div>

        {/* Content */}
        <div className="flex-1 overflow-y-auto p-6">
          {/* Sound Tab */}
          {activeTab === 'sound' && (
            <div className="space-y-6">
              {/* Volume */}
              <div>
                <label className="block text-sm font-medium mb-2">Volume</label>
                <div className="flex items-center gap-4">
                  <input
                    type="range"
                    min="0"
                    max="100"
                    value={settings.volume * 100}
                    onChange={(e) => onSettingsChange({ volume: parseInt(e.target.value) / 100 })}
                    className="flex-1"
                  />
                  <span className="text-sm font-mono w-12">{Math.round(settings.volume * 100)}%</span>
                </div>
              </div>

              {/* Sound Type */}
              <div>
                <label className="block text-sm font-medium mb-2">Sound Type</label>
                <div className="grid grid-cols-2 gap-3">
                  {(['wood', 'beep', 'click', 'custom'] as MetronomeSoundType[]).map((type) => (
                    <button
                      key={type}
                      onClick={() => handleSoundTypeChange(type)}
                      className={`p-4 rounded-lg border-2 transition-all ${
                        settings.soundType === type
                          ? 'border-primary bg-primary/10'
                          : 'border-border hover:border-primary/50'
                      }`}
                    >
                      <div className="font-semibold capitalize">{type}</div>
                      <div className="text-xs text-muted-foreground mt-1">
                        {type === 'wood' && 'Warm, organic woodblock'}
                        {type === 'beep' && 'Classic digital beep'}
                        {type === 'click' && 'Sharp, percussive click'}
                        {type === 'custom' && 'Your own sound'}
                      </div>
                    </button>
                  ))}
                </div>
              </div>

              {/* Custom Sound Upload */}
              {settings.soundType === 'custom' && (
                <div>
                  <label className="block text-sm font-medium mb-2">Upload Custom Sound</label>
                  <label className="flex items-center justify-center gap-2 p-4 border-2 border-dashed border-border rounded-lg hover:border-primary/50 cursor-pointer transition-all">
                    <Upload className="h-5 w-5" />
                    <span>Choose audio file (WAV, MP3, OGG)</span>
                    <input
                      type="file"
                      accept="audio/*"
                      onChange={handleFileUpload}
                      className="hidden"
                    />
                  </label>
                  {settings.customSoundUrl && (
                    <p className="text-sm text-green-500 mt-2">✓ Custom sound loaded</p>
                  )}
                </div>
              )}

              {/* Accent Settings */}
              <div>
                <label className="block text-sm font-medium mb-2">Accent Pattern</label>
                <div className="space-y-3">
                  <label className="flex items-center gap-2">
                    <input
                      type="checkbox"
                      checked={settings.accentDownbeat}
                      onChange={(e) => onSettingsChange({ accentDownbeat: e.target.checked })}
                      className="rounded"
                    />
                    <span>Accent downbeat (beat 1)</span>
                  </label>

                  <label className="flex items-center gap-2">
                    <input
                      type="checkbox"
                      checked={settings.useTimeSignatureAccents}
                      onChange={(e) => onSettingsChange({ useTimeSignatureAccents: e.target.checked })}
                      className="rounded"
                    />
                    <span>Time signature aware accents</span>
                  </label>

                  <div>
                    <label className="block text-xs text-muted-foreground mb-1">
                      Custom grouping (e.g., "2+2+3" for 7/8)
                    </label>
                    <input
                      type="text"
                      placeholder="2+2+3"
                      defaultValue={settings.accentGrouping.join('+')}
                      onBlur={(e) => handleCustomTimeSignature(e.target.value)}
                      className="w-full px-3 py-2 bg-secondary/40 border border-border rounded-lg"
                    />
                  </div>
                </div>
              </div>

              {/* Click Behavior */}
              <div>
                <label className="block text-sm font-medium mb-2">Click During</label>
                <div className="space-y-2">
                  <label className="flex items-center gap-2">
                    <input
                      type="checkbox"
                      checked={settings.clickDuringPlayback}
                      onChange={(e) => onSettingsChange({ clickDuringPlayback: e.target.checked })}
                      className="rounded"
                    />
                    <span>Playback</span>
                  </label>
                  <label className="flex items-center gap-2">
                    <input
                      type="checkbox"
                      checked={settings.clickDuringRecording}
                      onChange={(e) => onSettingsChange({ clickDuringRecording: e.target.checked })}
                      className="rounded"
                    />
                    <span>Recording</span>
                  </label>
                  <label className="flex items-center gap-2">
                    <input
                      type="checkbox"
                      checked={settings.clickDuringCountIn}
                      onChange={(e) => onSettingsChange({ clickDuringCountIn: e.target.checked })}
                      className="rounded"
                    />
                    <span>Count-in</span>
                  </label>
                </div>
              </div>
            </div>
          )}

          {/* Pre-Count Tab */}
          {activeTab === 'precount' && (
            <div className="space-y-6">
              <div>
                <label className="flex items-center gap-2 mb-4">
                  <input
                    type="checkbox"
                    checked={settings.preCount.enabled}
                    onChange={(e) => onSettingsChange({
                      preCount: { ...settings.preCount, enabled: e.target.checked }
                    })}
                    className="rounded"
                  />
                  <span className="font-medium">Enable Pre-Count</span>
                </label>
              </div>

              {settings.preCount.enabled && (
                <>
                  <div>
                    <label className="block text-sm font-medium mb-2">
                      Number of Bars ({settings.preCount.bars})
                    </label>
                    <input
                      type="range"
                      min="1"
                      max="8"
                      value={settings.preCount.bars}
                      onChange={(e) => onSettingsChange({
                        preCount: { ...settings.preCount, bars: parseInt(e.target.value) }
                      })}
                      className="w-full"
                    />
                    <div className="flex justify-between text-xs text-muted-foreground mt-1">
                      <span>1 bar</span>
                      <span>8 bars</span>
                    </div>
                  </div>

                  <div>
                    <label className="flex items-center gap-2">
                      <input
                        type="checkbox"
                        checked={settings.preCount.onlyWhenRecording}
                        onChange={(e) => onSettingsChange({
                          preCount: { ...settings.preCount, onlyWhenRecording: e.target.checked }
                        })}
                        className="rounded"
                      />
                      <span>Only when recording</span>
                    </label>
                  </div>

                  <div>
                    <label className="flex items-center gap-2">
                      <input
                        type="checkbox"
                        checked={settings.preCount.showVisualCountdown}
                        onChange={(e) => onSettingsChange({
                          preCount: { ...settings.preCount, showVisualCountdown: e.target.checked }
                        })}
                        className="rounded"
                      />
                      <span>Show visual countdown</span>
                    </label>
                    <p className="text-xs text-muted-foreground mt-1 ml-6">
                      Display big numbers "1 2 3 GO" in the center of the screen
                    </p>
                  </div>
                </>
              )}
            </div>
          )}

          {/* MIDI Clock Tab */}
          {activeTab === 'midi' && (
            <div className="space-y-6">
              <div>
                <label className="flex items-center gap-2 mb-4">
                  <input
                    type="checkbox"
                    checked={midiSettings.enabled}
                    onChange={(e) => onMIDISettingsChange({ enabled: e.target.checked })}
                    className="rounded"
                  />
                  <span className="font-medium">Enable MIDI Clock Output</span>
                </label>
                <p className="text-sm text-muted-foreground">
                  Send MIDI clock messages (24 PPQN) to external devices for synchronization
                </p>
              </div>

              {midiSettings.enabled && (
                <>
                  <div>
                    <label className="block text-sm font-medium mb-2">Send Messages</label>
                    <div className="space-y-2">
                      <label className="flex items-center gap-2">
                        <input
                          type="checkbox"
                          checked={midiSettings.sendStart}
                          onChange={(e) => onMIDISettingsChange({ sendStart: e.target.checked })}
                          className="rounded"
                        />
                        <span>Start</span>
                      </label>
                      <label className="flex items-center gap-2">
                        <input
                          type="checkbox"
                          checked={midiSettings.sendStop}
                          onChange={(e) => onMIDISettingsChange({ sendStop: e.target.checked })}
                          className="rounded"
                        />
                        <span>Stop</span>
                      </label>
                      <label className="flex items-center gap-2">
                        <input
                          type="checkbox"
                          checked={midiSettings.sendContinue}
                          onChange={(e) => onMIDISettingsChange({ sendContinue: e.target.checked })}
                          className="rounded"
                        />
                        <span>Continue</span>
                      </label>
                      <label className="flex items-center gap-2">
                        <input
                          type="checkbox"
                          checked={midiSettings.sendSongPosition}
                          onChange={(e) => onMIDISettingsChange({ sendSongPosition: e.target.checked })}
                          className="rounded"
                        />
                        <span>Song Position Pointer</span>
                      </label>
                    </div>
                  </div>

                  <div>
                    <label className="flex items-center gap-2">
                      <input
                        type="checkbox"
                        checked={midiSettings.timeSignatureAware}
                        onChange={(e) => onMIDISettingsChange({ timeSignatureAware: e.target.checked })}
                        className="rounded"
                      />
                      <span>Time signature aware</span>
                    </label>
                    <p className="text-xs text-muted-foreground mt-1 ml-6">
                      Adjust MIDI clock pattern based on time signature changes
                    </p>
                  </div>
                </>
              )}
            </div>
          )}
        </div>

        {/* Footer */}
        <div className="p-6 border-t border-border flex justify-end">
          <Button onClick={onClose}>Close</Button>
        </div>
      </motion.div>
    </motion.div>
  );
}
