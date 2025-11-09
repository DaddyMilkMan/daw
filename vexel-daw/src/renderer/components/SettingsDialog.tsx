/**
 * Settings Dialog
 * Audio/MIDI device configuration and preferences
 */

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import * as Dialog from '@radix-ui/react-dialog';
import * as Select from '@radix-ui/react-select';
import { X, Check, ChevronDown, Settings } from 'lucide-react';
import { useAudioStore } from '../store/audioStore';
import { audioEngine } from '../services/audioEngine';
import { Button } from './ui/button';

interface SettingsDialogProps {
  open: boolean;
  onOpenChange: (open: boolean) => void;
}

export default function SettingsDialog({ open, onOpenChange }: SettingsDialogProps) {
  const audioSettings = useAudioStore((state) => state.audioSettings);
  const midiSettings = useAudioStore((state) => state.midiSettings);
  const updateAudioSettings = useAudioStore((state) => state.updateAudioSettings);
  const updateMIDISettings = useAudioStore((state) => state.updateMIDISettings);
  const recording = useAudioStore((state) => state.recording);

  const [audioInputDevices, setAudioInputDevices] = useState<MediaDeviceInfo[]>([]);
  const [midiInputDevices, setMIDIInputDevices] = useState<Map<string, MIDIInput>>(new Map());
  const [activeTab, setActiveTab] = useState<'audio' | 'midi' | 'recording'>('audio');

  useEffect(() => {
    if (open) {
      loadDevices();
    }
  }, [open]);

  const loadDevices = async () => {
    try {
      // Load audio input devices
      const devices = await audioEngine.getInputDevices();
      setAudioInputDevices(devices);

      // Load MIDI input devices
      const midiDevices = audioEngine.getMIDIInputDevices();
      setMIDIInputDevices(midiDevices);
    } catch (error) {
      console.error('Failed to load devices:', error);
    }
  };

  const handleAudioInputChange = async (deviceId: string) => {
    try {
      await audioEngine.setupAudioInput(deviceId);
      updateAudioSettings({ inputDevice: deviceId });
    } catch (error) {
      console.error('Failed to select audio input:', error);
    }
  };

  const handleMIDIInputChange = (deviceId: string) => {
    audioEngine.selectMIDIInput(deviceId);
    updateMIDISettings({ inputDevices: [deviceId] });
  };

  return (
    <Dialog.Root open={open} onOpenChange={onOpenChange}>
      <Dialog.Portal>
        <Dialog.Overlay className="fixed inset-0 bg-black/80 backdrop-blur-sm z-50" />
        <Dialog.Content className="fixed left-1/2 top-1/2 -translate-x-1/2 -translate-y-1/2 w-[600px] max-h-[80vh] bg-card border border-border rounded-lg shadow-2xl z-50 overflow-hidden">
          <motion.div
            initial={{ opacity: 0, scale: 0.95 }}
            animate={{ opacity: 1, scale: 1 }}
            exit={{ opacity: 0, scale: 0.95 }}
            transition={{ duration: 0.2 }}
          >
            {/* Header */}
            <div className="flex items-center justify-between p-6 border-b border-border">
              <div className="flex items-center gap-3">
                <Settings className="h-5 w-5 text-primary" />
                <Dialog.Title className="text-lg font-semibold">Settings</Dialog.Title>
              </div>
              <Dialog.Close asChild>
                <button className="p-1 rounded hover:bg-muted transition-colors">
                  <X className="h-5 w-5" />
                </button>
              </Dialog.Close>
            </div>

            {/* Tabs */}
            <div className="flex border-b border-border">
              <button
                onClick={() => setActiveTab('audio')}
                className={`flex-1 px-4 py-3 text-sm font-medium transition-colors ${
                  activeTab === 'audio'
                    ? 'text-primary border-b-2 border-primary bg-primary/5'
                    : 'text-muted-foreground hover:text-foreground hover:bg-muted/50'
                }`}
              >
                Audio
              </button>
              <button
                onClick={() => setActiveTab('midi')}
                className={`flex-1 px-4 py-3 text-sm font-medium transition-colors ${
                  activeTab === 'midi'
                    ? 'text-primary border-b-2 border-primary bg-primary/5'
                    : 'text-muted-foreground hover:text-foreground hover:bg-muted/50'
                }`}
              >
                MIDI
              </button>
              <button
                onClick={() => setActiveTab('recording')}
                className={`flex-1 px-4 py-3 text-sm font-medium transition-colors ${
                  activeTab === 'recording'
                    ? 'text-primary border-b-2 border-primary bg-primary/5'
                    : 'text-muted-foreground hover:text-foreground hover:bg-muted/50'
                }`}
              >
                Recording
              </button>
            </div>

            {/* Content */}
            <div className="p-6 space-y-6 max-h-[500px] overflow-y-auto">
              <AnimatePresence mode="wait">
                {activeTab === 'audio' && (
                  <motion.div
                    key="audio"
                    initial={{ opacity: 0, x: -20 }}
                    animate={{ opacity: 1, x: 0 }}
                    exit={{ opacity: 0, x: 20 }}
                    className="space-y-4"
                  >
                    {/* Sample Rate */}
                    <div className="space-y-2">
                      <label className="text-sm font-medium">Sample Rate</label>
                      <select
                        value={audioSettings.sampleRate}
                        onChange={(e) =>
                          updateAudioSettings({ sampleRate: parseInt(e.target.value) })
                        }
                        className="w-full px-3 py-2 bg-muted border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary"
                      >
                        <option value={44100}>44.1 kHz (CD Quality)</option>
                        <option value={48000}>48 kHz (Pro Standard)</option>
                        <option value={96000}>96 kHz (High Quality)</option>
                        <option value={192000}>192 kHz (Ultra High Quality)</option>
                      </select>
                      <p className="text-xs text-muted-foreground">
                        Higher sample rates provide better quality but require more CPU power
                      </p>
                    </div>

                    {/* Bit Depth */}
                    <div className="space-y-2">
                      <label className="text-sm font-medium">Bit Depth (Export)</label>
                      <select
                        value={audioSettings.bitDepth}
                        onChange={(e) =>
                          updateAudioSettings({
                            bitDepth: parseInt(e.target.value) as 16 | 24 | 32,
                          })
                        }
                        className="w-full px-3 py-2 bg-muted border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary"
                      >
                        <option value={16}>16-bit</option>
                        <option value={24}>24-bit</option>
                        <option value={32}>32-bit Float (Highest Quality)</option>
                      </select>
                      <p className="text-xs text-muted-foreground">
                        Internal processing is always 32-bit float
                      </p>
                    </div>

                    {/* Buffer Size */}
                    <div className="space-y-2">
                      <label className="text-sm font-medium">Buffer Size</label>
                      <select
                        value={audioSettings.bufferSize}
                        onChange={(e) =>
                          updateAudioSettings({ bufferSize: parseInt(e.target.value) })
                        }
                        className="w-full px-3 py-2 bg-muted border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary"
                      >
                        <option value={64}>64 samples (Lowest Latency - High CPU)</option>
                        <option value={128}>128 samples (Recommended for Recording)</option>
                        <option value={256}>256 samples (Balanced)</option>
                        <option value={512}>512 samples (Mixing/Mastering)</option>
                        <option value={1024}>1024 samples (Complex Projects)</option>
                      </select>
                      <p className="text-xs text-muted-foreground">
                        Latency: ~
                        {((audioSettings.bufferSize / audioSettings.sampleRate) * 1000).toFixed(2)}ms
                      </p>
                    </div>

                    {/* Audio Input Device */}
                    <div className="space-y-2">
                      <label className="text-sm font-medium">Input Device</label>
                      <select
                        value={audioSettings.inputDevice || ''}
                        onChange={(e) => handleAudioInputChange(e.target.value)}
                        className="w-full px-3 py-2 bg-muted border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary"
                      >
                        <option value="">Select Input Device...</option>
                        {audioInputDevices.map((device) => (
                          <option key={device.deviceId} value={device.deviceId}>
                            {device.label || `Device ${device.deviceId.slice(0, 8)}`}
                          </option>
                        ))}
                      </select>
                      <Button
                        size="sm"
                        variant="outline"
                        onClick={loadDevices}
                        className="w-full mt-2"
                      >
                        Refresh Devices
                      </Button>
                    </div>
                  </motion.div>
                )}

                {activeTab === 'midi' && (
                  <motion.div
                    key="midi"
                    initial={{ opacity: 0, x: -20 }}
                    animate={{ opacity: 1, x: 0 }}
                    exit={{ opacity: 0, x: 20 }}
                    className="space-y-4"
                  >
                    {/* MIDI Input Devices */}
                    <div className="space-y-2">
                      <label className="text-sm font-medium">MIDI Input Devices</label>
                      <div className="space-y-2">
                        {Array.from(midiInputDevices.entries()).map(([id, input]) => (
                          <label
                            key={id}
                            className="flex items-center gap-3 p-3 bg-muted/50 rounded-md hover:bg-muted cursor-pointer transition-colors"
                          >
                            <input
                              type="checkbox"
                              checked={midiSettings.inputDevices.includes(id)}
                              onChange={(e) => {
                                if (e.target.checked) {
                                  handleMIDIInputChange(id);
                                } else {
                                  updateMIDISettings({
                                    inputDevices: midiSettings.inputDevices.filter(
                                      (d) => d !== id
                                    ),
                                  });
                                }
                              }}
                              className="w-4 h-4 rounded border-border checked:bg-primary"
                            />
                            <div className="flex-1">
                              <div className="text-sm font-medium">{input.name}</div>
                              <div className="text-xs text-muted-foreground">
                                {input.manufacturer || 'Unknown Manufacturer'}
                              </div>
                            </div>
                          </label>
                        ))}
                        {midiInputDevices.size === 0 && (
                          <div className="text-sm text-muted-foreground text-center py-8">
                            No MIDI devices found. Connect a MIDI device and refresh.
                          </div>
                        )}
                      </div>
                      <Button
                        size="sm"
                        variant="outline"
                        onClick={loadDevices}
                        className="w-full mt-2"
                      >
                        Refresh MIDI Devices
                      </Button>
                    </div>

                    {/* MIDI Thru */}
                    <div className="space-y-2">
                      <label className="flex items-center gap-3 cursor-pointer">
                        <input
                          type="checkbox"
                          checked={midiSettings.midiThru}
                          onChange={(e) =>
                            updateMIDISettings({ midiThru: e.target.checked })
                          }
                          className="w-4 h-4 rounded border-border checked:bg-primary"
                        />
                        <div>
                          <div className="text-sm font-medium">MIDI Thru</div>
                          <div className="text-xs text-muted-foreground">
                            Pass MIDI input directly to output
                          </div>
                        </div>
                      </label>
                    </div>
                  </motion.div>
                )}

                {activeTab === 'recording' && (
                  <motion.div
                    key="recording"
                    initial={{ opacity: 0, x: -20 }}
                    animate={{ opacity: 1, x: 0 }}
                    exit={{ opacity: 0, x: 20 }}
                    className="space-y-4"
                  >
                    {/* Pre-count */}
                    <div className="space-y-2">
                      <label className="text-sm font-medium">Pre-count (Bars)</label>
                      <select
                        value={recording.preCountBars}
                        onChange={(e) =>
                          useAudioStore.setState((state) => ({
                            recording: {
                              ...state.recording,
                              preCountBars: parseInt(e.target.value),
                            },
                          }))
                        }
                        className="w-full px-3 py-2 bg-muted border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary"
                      >
                        <option value={0}>No Pre-count</option>
                        <option value={1}>1 Bar</option>
                        <option value={2}>2 Bars (Recommended)</option>
                        <option value={4}>4 Bars</option>
                      </select>
                      <p className="text-xs text-muted-foreground">
                        Count-in time before recording starts
                      </p>
                    </div>

                    {/* Input Monitoring */}
                    <div className="space-y-2">
                      <label className="flex items-center gap-3 cursor-pointer">
                        <input
                          type="checkbox"
                          checked={recording.inputMonitoring}
                          onChange={(e) =>
                            audioEngine.toggleInputMonitoring(e.target.checked)
                          }
                          className="w-4 h-4 rounded border-border checked:bg-primary"
                        />
                        <div>
                          <div className="text-sm font-medium">Input Monitoring</div>
                          <div className="text-xs text-muted-foreground">
                            Hear input signal while recording (may increase latency)
                          </div>
                        </div>
                      </label>
                    </div>

                    {/* Latency Compensation */}
                    <div className="space-y-2">
                      <label className="text-sm font-medium">
                        Latency Compensation (samples)
                      </label>
                      <input
                        type="number"
                        value={recording.latencyCompensation}
                        onChange={(e) =>
                          useAudioStore.setState((state) => ({
                            recording: {
                              ...state.recording,
                              latencyCompensation: parseInt(e.target.value) || 0,
                            },
                          }))
                        }
                        className="w-full px-3 py-2 bg-muted border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary"
                        min={0}
                        max={10000}
                        step={1}
                      />
                      <p className="text-xs text-muted-foreground">
                        Manual latency adjustment in samples (auto-detected by default)
                      </p>
                    </div>

                    {/* Auto-save */}
                    <div className="space-y-2 border-t border-border pt-4">
                      <div className="text-sm font-medium">Auto-save</div>
                      <div className="flex items-center gap-2">
                        <input
                          type="checkbox"
                          id="autosave"
                          className="w-4 h-4 rounded border-border checked:bg-primary"
                        />
                        <label htmlFor="autosave" className="text-sm text-muted-foreground">
                          Automatically save project every 5 minutes
                        </label>
                      </div>
                    </div>
                  </motion.div>
                )}
              </AnimatePresence>
            </div>

            {/* Footer */}
            <div className="flex justify-end gap-2 p-6 border-t border-border">
              <Dialog.Close asChild>
                <Button variant="outline">Cancel</Button>
              </Dialog.Close>
              <Dialog.Close asChild>
                <Button>Save Settings</Button>
              </Dialog.Close>
            </div>
          </motion.div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
