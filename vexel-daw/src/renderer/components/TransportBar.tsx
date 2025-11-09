import { Play, Pause, Square, SkipBack, SkipForward, Circle, Sparkles, Repeat, Activity, Volume2 } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { Button } from './ui/button';
import { AudioState } from '@/types/audio';
import { formatTime } from '@/lib/utils';
import { useState, useEffect } from 'react';
import { metronome } from '@/lib/metronome';
import { loadMetronomeSettings, saveMetronomeSettings, loadProjectSettings, saveProjectSettings } from '@/lib/storage';

interface TransportBarProps {
  audioState: AudioState;
  onOpenWingman: () => void;
}

export default function TransportBar({ audioState, onOpenWingman }: TransportBarProps) {
  // Load saved settings
  const savedMetronome = loadMetronomeSettings();
  const savedProject = loadProjectSettings();

  const [tempo, setTempo] = useState(savedProject.tempo || audioState.tempo);
  const [isLooping, setIsLooping] = useState(false);
  const [isMetronomeOn, setIsMetronomeOn] = useState(false);
  const [tapTimes, setTapTimes] = useState<number[]>([]);
  const [cpuUsage, setCpuUsage] = useState(12);
  const [timeSignature, setTimeSignature] = useState(savedProject.timeSignature || audioState.timeSignature);
  const [showMetronomeSettings, setShowMetronomeSettings] = useState(false);
  const [metronomeVolume, setMetronomeVolume] = useState(savedMetronome.volume);
  const [showTimeSignatureEdit, setShowTimeSignatureEdit] = useState(false);
  const [preCountEnabled, setPreCountEnabled] = useState(savedMetronome.preCountEnabled);
  const [preCountBars, setPreCountBars] = useState(savedMetronome.preCountBars);

  useEffect(() => {
    setTempo(audioState.tempo);
  }, [audioState.tempo]);

  useEffect(() => {
    setTimeSignature(audioState.timeSignature);
  }, [audioState.timeSignature]);

  // Initialize metronome settings
  useEffect(() => {
    metronome.setTempo(tempo);
    metronome.setTimeSignature(timeSignature.numerator, timeSignature.denominator);
    metronome.setVolume(metronomeVolume);
    metronome.setEnabled(isMetronomeOn);
  }, [tempo, timeSignature, metronomeVolume, isMetronomeOn]);

  // Sync metronome with playback
  useEffect(() => {
    if (audioState.isPlaying) {
      if (isMetronomeOn) {
        metronome.start();
      }
    } else {
      metronome.stop();
    }
    return () => {
      metronome.stop();
    };
  }, [audioState.isPlaying, isMetronomeOn]);

  // Simulate CPU usage (in real DAW, this would come from audio engine)
  useEffect(() => {
    const interval = setInterval(() => {
      setCpuUsage(Math.floor(Math.random() * 30 + 10));
    }, 2000);
    return () => clearInterval(interval);
  }, []);

  // Save metronome settings when they change
  useEffect(() => {
    saveMetronomeSettings({
      volume: metronomeVolume,
      preCountEnabled,
      preCountBars,
    });
  }, [metronomeVolume, preCountEnabled, preCountBars]);

  // Save project settings when they change
  useEffect(() => {
    saveProjectSettings({
      tempo,
      timeSignature,
    });
  }, [tempo, timeSignature]);

  const handlePlay = () => {
    if (audioState.isPlaying) {
      window.electron.transportPause();
    } else {
      // Start with pre-count if enabled and metronome is on
      if (preCountEnabled && isMetronomeOn && !audioState.isPlaying) {
        metronome.startWithPreCount(preCountBars, () => {
          window.electron.transportPlay();
        });
      } else {
        window.electron.transportPlay();
      }
    }
  };

  const handleStop = () => {
    window.electron.transportStop();
  };

  const handleTempoChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const newTempo = parseInt(e.target.value);
    setTempo(newTempo);
  };

  const handleTempoBlur = () => {
    if (tempo !== audioState.tempo && tempo >= 20 && tempo <= 999) {
      window.electron.setTempo(tempo);
    } else {
      setTempo(audioState.tempo);
    }
  };

  const handleTapTempo = () => {
    const now = Date.now();
    const newTapTimes = [...tapTimes, now].slice(-4); // Keep last 4 taps
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
        window.electron.setTempo(newTempo);
      }
    }

    // Reset tap times after 2 seconds
    setTimeout(() => {
      setTapTimes((prev) => prev.filter((t) => Date.now() - t < 2000));
    }, 2000);
  };

  const handleTimeSignatureChange = (numerator: number, denominator: number) => {
    const newTimeSignature = { numerator, denominator };
    setTimeSignature(newTimeSignature);
    // TODO: Send to electron main process via IPC
    // window.electron.setTimeSignature(numerator, denominator);
  };

  const handleMetronomeVolumeChange = (volume: number) => {
    setMetronomeVolume(volume);
  };

  return (
    <motion.div
      initial={{ y: -20, opacity: 0 }}
      animate={{ y: 0, opacity: 1 }}
      transition={{ duration: 0.3 }}
      className="h-20 bg-gradient-to-b from-card to-card/80 border-b border-border/50 flex items-center px-6 gap-6 backdrop-blur-xl"
    >
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
            variant="ghost"
            onClick={handlePlay}
            title={audioState.isPlaying ? 'Pause (Space)' : 'Play (Space)'}
            className={`transition-all ${
              audioState.isPlaying
                ? 'bg-primary/20 text-primary hover:bg-primary/30'
                : 'hover:bg-white/5'
            }`}
          >
            <motion.div
              animate={{
                scale: audioState.isPlaying ? [1, 1.2, 1] : 1,
              }}
              transition={{
                repeat: audioState.isPlaying ? Infinity : 0,
                duration: 1,
              }}
            >
              {audioState.isPlaying ? (
                <Pause className="h-5 w-5" />
              ) : (
                <Play className="h-5 w-5 ml-0.5" />
              )}
            </motion.div>
          </Button>
        </motion.div>

        <motion.div whileHover={{ scale: 1.1 }} whileTap={{ scale: 0.95 }}>
          <Button
            size="icon"
            variant="ghost"
            title="Return to Zero"
            onClick={handleStop}
            className="hover:bg-white/5 transition-all"
          >
            <SkipBack className="h-5 w-5" />
          </Button>
        </motion.div>

        <motion.div whileHover={{ scale: 1.1 }} whileTap={{ scale: 0.95 }}>
          <Button
            size="icon"
            variant="ghost"
            title="Record (R)"
            className="hover:bg-red-500/10 transition-all"
          >
            <Circle className="h-5 w-5 fill-current text-red-500" />
          </Button>
        </motion.div>

        <motion.div whileHover={{ scale: 1.1 }} whileTap={{ scale: 0.95 }}>
          <Button
            size="icon"
            variant="ghost"
            onClick={() => setIsLooping(!isLooping)}
            title="Loop (L)"
            className={`transition-all ${
              isLooping
                ? 'bg-primary/20 text-primary hover:bg-primary/30'
                : 'hover:bg-white/5'
            }`}
          >
            <Repeat className="h-5 w-5" />
          </Button>
        </motion.div>

        <motion.div whileHover={{ scale: 1.1 }} whileTap={{ scale: 0.95 }}>
          <Button
            size="icon"
            variant="ghost"
            title="Fast Forward (End)"
            onClick={handleStop}
            className="hover:bg-white/5 transition-all"
          >
            <SkipForward className="h-5 w-5" />
          </Button>
        </motion.div>
      </div>

      {/* Divider */}
      <div className="h-12 w-px bg-gradient-to-b from-transparent via-border to-transparent" />

      {/* Time Display */}
      <motion.div
        className="flex flex-col"
        whileHover={{ scale: 1.02 }}
        transition={{ type: 'spring', stiffness: 400, damping: 10 }}
      >
        <span className="text-xs text-muted-foreground font-medium">Position</span>
        <motion.span
          className="text-lg font-mono font-semibold bg-gradient-to-r from-foreground to-foreground/70 bg-clip-text text-transparent"
          animate={{
            opacity: audioState.isPlaying ? [1, 0.7, 1] : 1,
          }}
          transition={{
            repeat: audioState.isPlaying ? Infinity : 0,
            duration: 1,
          }}
        >
          {formatTime(audioState.currentBar, audioState.timeSignature.numerator)}
        </motion.span>
      </motion.div>

      {/* Divider */}
      <div className="h-12 w-px bg-gradient-to-b from-transparent via-border to-transparent" />

      {/* Tempo */}
      <div className="flex flex-col">
        <span className="text-xs text-muted-foreground font-medium">Tempo</span>
        <div className="flex items-center gap-2">
          <motion.input
            type="number"
            value={tempo}
            onChange={handleTempoChange}
            onBlur={handleTempoBlur}
            min="20"
            max="999"
            whileFocus={{ scale: 1.02 }}
            className="w-20 px-3 py-1.5 bg-secondary/40 border border-border/50 rounded-lg text-lg font-mono font-semibold focus:outline-none focus:ring-2 focus:ring-primary/50 focus:border-primary/50 transition-all backdrop-blur-xl"
          />
          <span className="text-sm text-muted-foreground font-medium">BPM</span>
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={handleTapTempo}
            className="px-2 py-1 bg-accent hover:bg-accent/80 rounded text-xs font-medium transition-colors"
            title="Tap Tempo (T)"
          >
            TAP
          </motion.button>
        </div>
      </div>

      {/* Divider */}
      <div className="h-12 w-px bg-gradient-to-b from-transparent via-border to-transparent" />

      {/* Time Signature */}
      <div className="relative flex flex-col">
        <span className="text-xs text-muted-foreground font-medium">Time Sig</span>
        <motion.button
          onClick={() => setShowTimeSignatureEdit(!showTimeSignatureEdit)}
          whileHover={{ scale: 1.02 }}
          whileTap={{ scale: 0.98 }}
          className="text-lg font-mono font-semibold hover:bg-secondary/40 px-2 py-0.5 rounded transition-colors"
        >
          {timeSignature.numerator}/{timeSignature.denominator}
        </motion.button>

        {/* Time Signature Editor Dropdown */}
        <AnimatePresence>
          {showTimeSignatureEdit && (
            <motion.div
              initial={{ opacity: 0, y: -10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              className="absolute top-full left-0 mt-2 bg-card border border-border/50 rounded-lg p-4 shadow-xl backdrop-blur-xl z-50 min-w-[200px]"
            >
              <div className="flex flex-col gap-3">
                <div>
                  <label className="text-xs text-muted-foreground font-medium block mb-1">
                    Numerator
                  </label>
                  <select
                    value={timeSignature.numerator}
                    onChange={(e) =>
                      handleTimeSignatureChange(parseInt(e.target.value), timeSignature.denominator)
                    }
                    className="w-full px-3 py-1.5 bg-secondary/40 border border-border/50 rounded text-sm"
                  >
                    {[2, 3, 4, 5, 6, 7, 8, 9, 12, 16].map((num) => (
                      <option key={num} value={num}>
                        {num}
                      </option>
                    ))}
                  </select>
                </div>
                <div>
                  <label className="text-xs text-muted-foreground font-medium block mb-1">
                    Denominator
                  </label>
                  <select
                    value={timeSignature.denominator}
                    onChange={(e) =>
                      handleTimeSignatureChange(timeSignature.numerator, parseInt(e.target.value))
                    }
                    className="w-full px-3 py-1.5 bg-secondary/40 border border-border/50 rounded text-sm"
                  >
                    {[2, 4, 8, 16].map((den) => (
                      <option key={den} value={den}>
                        {den}
                      </option>
                    ))}
                  </select>
                </div>
                <Button
                  size="sm"
                  onClick={() => setShowTimeSignatureEdit(false)}
                  className="mt-2"
                >
                  Done
                </Button>
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </div>

      {/* Spacer */}
      <div className="flex-1" />

      {/* Wingman Button */}
      <motion.div
        whileHover={{ scale: 1.05 }}
        whileTap={{ scale: 0.95 }}
        transition={{ type: 'spring', stiffness: 400, damping: 10 }}
      >
        <Button
          onClick={onOpenWingman}
          className="bg-gradient-to-r from-primary via-primary/90 to-primary/80 hover:from-primary/90 hover:via-primary/80 hover:to-primary/70 shadow-lg shadow-primary/30 transition-all relative overflow-hidden group"
        >
          <motion.div
            className="absolute inset-0 bg-gradient-to-r from-white/0 via-white/20 to-white/0"
            animate={{
              x: ['-100%', '100%'],
            }}
            transition={{
              repeat: Infinity,
              duration: 3,
              ease: 'linear',
            }}
          />
          <Sparkles className="h-4 w-4 mr-2 relative z-10" />
          <span className="relative z-10 font-semibold">Wingman AI</span>
        </Button>
      </motion.div>

      {/* Divider */}
      <div className="h-12 w-px bg-gradient-to-b from-transparent via-border to-transparent" />

      {/* Metronome */}
      <div className="relative flex items-center gap-1">
        <motion.div whileHover={{ scale: 1.05 }} whileTap={{ scale: 0.95 }}>
          <Button
            size="icon"
            variant="ghost"
            onClick={() => setIsMetronomeOn(!isMetronomeOn)}
            title="Metronome (M)"
            className={`transition-all ${
              isMetronomeOn
                ? 'bg-primary/20 text-primary hover:bg-primary/30'
                : 'hover:bg-white/5'
            }`}
          >
            <Activity className={`h-5 w-5 ${isMetronomeOn ? 'animate-pulse' : ''}`} />
          </Button>
        </motion.div>

        <motion.div whileHover={{ scale: 1.05 }} whileTap={{ scale: 0.95 }}>
          <Button
            size="icon"
            variant="ghost"
            onClick={() => setShowMetronomeSettings(!showMetronomeSettings)}
            title="Metronome Settings"
            className="hover:bg-white/5 transition-all"
          >
            <Volume2 className="h-4 w-4" />
          </Button>
        </motion.div>

        {/* Metronome Settings Dropdown */}
        <AnimatePresence>
          {showMetronomeSettings && (
            <motion.div
              initial={{ opacity: 0, y: -10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              className="absolute top-full right-0 mt-2 bg-card border border-border/50 rounded-lg p-4 shadow-xl backdrop-blur-xl z-50 min-w-[250px]"
            >
              <div className="flex flex-col gap-4">
                <div>
                  <label className="text-xs text-muted-foreground font-medium block mb-2">
                    Metronome Volume
                  </label>
                  <div className="flex items-center gap-3">
                    <input
                      type="range"
                      min="0"
                      max="1"
                      step="0.01"
                      value={metronomeVolume}
                      onChange={(e) => handleMetronomeVolumeChange(parseFloat(e.target.value))}
                      className="flex-1"
                    />
                    <span className="text-sm font-mono w-12 text-right">
                      {Math.round(metronomeVolume * 100)}%
                    </span>
                  </div>
                </div>

                <div className="h-px bg-border/50" />

                <div>
                  <div className="flex items-center justify-between mb-2">
                    <label className="text-xs text-muted-foreground font-medium">
                      Pre-Count
                    </label>
                    <Button
                      size="sm"
                      variant={preCountEnabled ? 'default' : 'ghost'}
                      onClick={() => setPreCountEnabled(!preCountEnabled)}
                      className="h-6 px-2 text-xs"
                    >
                      {preCountEnabled ? 'On' : 'Off'}
                    </Button>
                  </div>
                  {preCountEnabled && (
                    <div className="flex items-center gap-2">
                      <span className="text-xs text-muted-foreground">Bars:</span>
                      <select
                        value={preCountBars}
                        onChange={(e) => setPreCountBars(parseInt(e.target.value))}
                        className="px-2 py-1 bg-secondary/40 border border-border/50 rounded text-sm"
                      >
                        {[1, 2, 4].map((bars) => (
                          <option key={bars} value={bars}>
                            {bars}
                          </option>
                        ))}
                      </select>
                    </div>
                  )}
                </div>

                <Button
                  size="sm"
                  onClick={() => setShowMetronomeSettings(false)}
                  className="mt-2"
                >
                  Done
                </Button>
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </div>

      {/* Divider */}
      <div className="h-12 w-px bg-gradient-to-b from-transparent via-border to-transparent" />

      {/* CPU Meter */}
      <div className="flex items-center gap-2 px-3 py-1.5 bg-secondary/40 rounded-lg backdrop-blur-xl border border-border/30">
        <span className="text-xs text-muted-foreground font-medium">CPU</span>
        <span className={`text-sm font-mono font-semibold ${cpuUsage > 70 ? 'text-red-500' : cpuUsage > 50 ? 'text-yellow-500' : 'text-green-500'}`}>
          {cpuUsage}%
        </span>
      </div>

      {/* Divider */}
      <div className="h-12 w-px bg-gradient-to-b from-transparent via-border to-transparent" />

      {/* Master Volume */}
      <div className="flex items-center gap-3">
        <span className="text-sm text-muted-foreground font-medium">Master</span>
        <div className="w-32 h-2.5 bg-secondary/40 rounded-full overflow-hidden backdrop-blur-xl border border-border/30">
          <motion.div
            className="h-full bg-gradient-to-r from-primary to-primary/70 shadow-lg shadow-primary/20"
            initial={{ width: '0%' }}
            animate={{ width: '80%' }}
            transition={{ duration: 0.5, ease: 'easeOut' }}
          />
        </div>
        <span className="text-sm font-mono font-semibold">-6 dB</span>
      </div>
    </motion.div>
  );
}
