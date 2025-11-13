/**
 * Enhanced TransportBar with Recording Functionality
 * Includes pre-count, recording controls, and integration with audio engine
 */

import { Play, Pause, Square, Circle, Repeat, Activity, Settings as SettingsIcon } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { Button } from './ui/button';
import { useState, useEffect } from 'react';
import { useAudioStore } from '../store/audioStore';
import { audioEngine } from '../services/audioEngine';
import SettingsDialog from './SettingsDialog';

interface TransportBarRecordingProps {
  onOpenWingman: () => void;
}

export default function TransportBarRecording({ onOpenWingman }: TransportBarRecordingProps) {
  const isPlaying = useAudioStore((state) => state.isPlaying);
  const tempo = useAudioStore((state) => state.tempo);
  const currentBeat = useAudioStore((state) => state.currentBeat);
  const timeSignature = useAudioStore((state) => state.timeSignature);
  const recording = useAudioStore((state) => state.recording);
  const looping = useAudioStore((state) => state.looping);

  const setTempo = useAudioStore((state) => state.setTempo);
  const play = useAudioStore((state) => state.play);
  const pause = useAudioStore((state) => state.pause);
  const stop = useAudioStore((state) => state.stop);

  const [tempTempo, setTempTempo] = useState(tempo);
  const [tapTimes, setTapTimes] = useState<number[]>([]);
  const [cpuUsage, setCpuUsage] = useState(12);
  const [showSettings, setShowSettings] = useState(false);
  const [isInitialized, setIsInitialized] = useState(false);

  // Initialize audio engine
  useEffect(() => {
    const init = async () => {
      try {
        await audioEngine.initialize();
        audioEngine.start();
        setIsInitialized(true);
      } catch (error) {
        console.error('Failed to initialize audio engine:', error);
      }
    };
    init();

    return () => {
      audioEngine.stop();
    };
  }, []);

  useEffect(() => {
    setTempTempo(tempo);
  }, [tempo]);

  // Simulate CPU usage
  useEffect(() => {
    const interval = setInterval(() => {
      setCpuUsage(Math.floor(Math.random() * 30 + 10));
    }, 2000);
    return () => clearInterval(interval);
  }, []);

  const handlePlay = () => {
    if (isPlaying) {
      audioEngine.pausePlayback();
    } else {
      audioEngine.startPlayback();
    }
  };

  const handleStop = () => {
    if (recording.isRecording) {
      audioEngine.stopRecording();
    }
    audioEngine.stopPlayback();
  };

  const handleRecord = async () => {
    if (!isInitialized) {
      console.error('Audio engine not initialized');
      return;
    }

    if (recording.isRecording) {
      // Stop recording
      await audioEngine.stopRecording();
    } else {
      // Start recording
      if (recording.armedTracks.size === 0) {
        alert('Please arm at least one track for recording (click the record button on a track)');
        return;
      }

      await audioEngine.startRecording();

      // Auto-play if not already playing
      if (!isPlaying) {
        play();
      }
    }
  };

  const handleTempoChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const newTempo = parseInt(e.target.value);
    if (!isNaN(newTempo)) {
      setTempTempo(newTempo);
    }
  };

  const handleTempoBlur = () => {
    if (tempTempo >= 20 && tempTempo <= 999) {
      setTempo(tempTempo);
    } else {
      setTempTempo(tempo);
    }
  };

  const handleTapTempo = () => {
    const now = Date.now();
    const newTapTimes = [...tapTimes, now].slice(-4);
    setTapTimes(newTapTimes);

    if (newTapTimes.length >= 2) {
      const intervals: number[] = [];
      for (let i = 1; i < newTapTimes.length; i++) {
        intervals.push(newTapTimes[i] - newTapTimes[i - 1]);
      }
      const avgInterval = intervals.reduce((a, b) => a + b, 0) / intervals.length;
      const newTempo = Math.round(60000 / avgInterval);

      if (newTempo >= 20 && newTempo <= 999) {
        setTempo(newTempo);
        setTempTempo(newTempo);
      }
    }

    setTimeout(() => {
      setTapTimes((prev) => prev.filter((t) => Date.now() - t < 2000));
    }, 2000);
  };

  const toggleLoop = () => {
    useAudioStore.setState((state) => ({
      looping: !state.looping,
    }));
  };

  // Format beat position as bars:beats
  const formatBeatPosition = (beat: number) => {
    const bar = Math.floor(beat / timeSignature.numerator) + 1;
    const beatInBar = Math.floor(beat % timeSignature.numerator) + 1;
    return `${bar}.${beatInBar}`;
  };

  return (
    <>
      <motion.div
        initial={{ y: -20, opacity: 0 }}
        animate={{ y: 0, opacity: 1 }}
        transition={{ duration: 0.3 }}
        className="h-20 bg-gradient-to-b from-card to-card/80 border-b border-border/50 flex items-center px-6 gap-6 backdrop-blur-xl relative"
      >
        {/* Pre-count Overlay */}
        <AnimatePresence>
          {recording.preCountRemaining > 0 && (
            <motion.div
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              exit={{ opacity: 0 }}
              className="absolute inset-0 bg-primary/20 backdrop-blur-sm flex items-center justify-center z-10"
            >
              <motion.div
                key={recording.preCountRemaining}
                initial={{ scale: 0.5, opacity: 0 }}
                animate={{ scale: 1, opacity: 1 }}
                exit={{ scale: 1.5, opacity: 0 }}
                className="text-6xl font-bold text-primary"
              >
                {recording.preCountRemaining}
              </motion.div>
            </motion.div>
          )}
        </AnimatePresence>

        {/* Transport Controls */}
        <div className="flex items-center gap-2">
          <motion.div whileHover={{ scale: 1.1 }} whileTap={{ scale: 0.95 }}>
            <Button
              size="icon"
              variant="ghost"
              onClick={handleStop}
              title="Stop (Enter)"
              className="hover:bg-white/5 transition-all"
            >
              <Square className="h-5 w-5" />
            </Button>
          </motion.div>

          <motion.div whileHover={{ scale: 1.1 }} whileTap={{ scale: 0.95 }}>
            <Button
              size="icon"
              variant={isPlaying ? 'default' : 'ghost'}
              onClick={handlePlay}
              title="Play/Pause (Space)"
              className={`${
                isPlaying ? 'bg-primary hover:bg-primary/90' : 'hover:bg-white/5'
              } transition-all`}
            >
              {isPlaying ? (
                <Pause className="h-5 w-5" />
              ) : (
                <Play className="h-5 w-5 ml-0.5" />
              )}
            </Button>
          </motion.div>

          {/* Record Button */}
          <motion.div
            whileHover={{ scale: 1.1 }}
            whileTap={{ scale: 0.95 }}
            animate={recording.isRecording ? { scale: [1, 1.1, 1] } : {}}
            transition={{ repeat: recording.isRecording ? Infinity : 0, duration: 1 }}
          >
            <Button
              size="icon"
              variant={recording.isRecording ? 'default' : recording.isArmed ? 'outline' : 'ghost'}
              onClick={handleRecord}
              title="Record (R)"
              disabled={!isInitialized}
              className={`${
                recording.isRecording
                  ? 'bg-red-500 hover:bg-red-600 text-white'
                  : recording.isArmed
                  ? 'border-red-500 text-red-500 hover:bg-red-500/10'
                  : 'hover:bg-white/5'
              } transition-all`}
            >
              <Circle
                className={`h-5 w-5 ${recording.isRecording ? 'fill-current' : ''}`}
              />
            </Button>
          </motion.div>

          {/* Loop Toggle */}
          <motion.div whileHover={{ scale: 1.1 }} whileTap={{ scale: 0.95 }}>
            <Button
              size="icon"
              variant={looping ? 'default' : 'ghost'}
              onClick={toggleLoop}
              title="Loop (L)"
              className={`${
                looping ? 'bg-primary/20 text-primary' : 'hover:bg-white/5'
              } transition-all`}
            >
              <Repeat className="h-5 w-5" />
            </Button>
          </motion.div>
        </div>

        {/* Position Display */}
        <div className="flex items-center gap-2 px-4 py-2 bg-muted/50 rounded-md">
          <span className="text-sm font-mono text-muted-foreground">Position:</span>
          <span className="text-lg font-mono font-bold text-foreground">
            {formatBeatPosition(currentBeat)}
          </span>
        </div>

        {/* Tempo Control */}
        <div className="flex items-center gap-3">
          <span className="text-sm text-muted-foreground">BPM:</span>
          <input
            type="number"
            value={tempTempo}
            onChange={handleTempoChange}
            onBlur={handleTempoBlur}
            onKeyDown={(e) => {
              if (e.key === 'Enter') {
                handleTempoBlur();
                e.currentTarget.blur();
              }
            }}
            className="w-16 px-2 py-1 text-center bg-muted border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary font-mono text-lg font-bold"
            min={20}
            max={999}
          />
          <Button
            size="sm"
            variant="outline"
            onClick={handleTapTempo}
            className="text-xs"
            title="Tap Tempo (T)"
          >
            TAP
          </Button>
        </div>

        {/* Time Signature */}
        <div className="flex items-center gap-2 px-3 py-1.5 bg-muted/50 rounded-md">
          <span className="text-lg font-mono font-bold text-foreground">
            {timeSignature.numerator}/{timeSignature.denominator}
          </span>
        </div>

        {/* Recording Status Indicator */}
        {recording.isArmed && (
          <motion.div
            initial={{ opacity: 0, x: -10 }}
            animate={{ opacity: 1, x: 0 }}
            className="flex items-center gap-2 px-3 py-1.5 bg-red-500/10 border border-red-500/30 rounded-md"
          >
            <motion.div
              animate={{ opacity: [1, 0.3, 1] }}
              transition={{ repeat: Infinity, duration: 1.5 }}
              className="w-2 h-2 bg-red-500 rounded-full"
            />
            <span className="text-xs text-red-500 font-medium">
              {recording.armedTracks.size} track{recording.armedTracks.size !== 1 ? 's' : ''} armed
            </span>
          </motion.div>
        )}

        {/* Spacer */}
        <div className="flex-1" />

        {/* CPU Meter */}
        <div className="flex items-center gap-2">
          <Activity className="h-4 w-4 text-muted-foreground" />
          <div className="flex flex-col items-end">
            <span className="text-xs text-muted-foreground">CPU</span>
            <span
              className={`text-sm font-mono font-bold ${
                cpuUsage > 80
                  ? 'text-red-500'
                  : cpuUsage > 60
                  ? 'text-yellow-500'
                  : 'text-green-500'
              }`}
            >
              {cpuUsage}%
            </span>
          </div>
        </div>

        {/* Settings Button */}
        <Button
          size="icon"
          variant="ghost"
          onClick={() => setShowSettings(true)}
          title="Settings"
          className="hover:bg-white/5"
        >
          <SettingsIcon className="h-5 w-5" />
        </Button>
      </motion.div>

      {/* Settings Dialog */}
      <SettingsDialog open={showSettings} onOpenChange={setShowSettings} />
    </>
  );
}
