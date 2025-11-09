import { Play, Pause, Square, SkipBack, Circle, Sparkles } from 'lucide-react';
import { motion } from 'framer-motion';
import { Button } from './ui/button';
import { AudioState } from '@/types/audio';
import { formatTime } from '@/lib/utils';
import { useState, useEffect } from 'react';

interface TransportBarProps {
  audioState: AudioState;
  onOpenWingman: () => void;
}

export default function TransportBar({ audioState, onOpenWingman }: TransportBarProps) {
  const [tempo, setTempo] = useState(audioState.tempo);

  useEffect(() => {
    setTempo(audioState.tempo);
  }, [audioState.tempo]);

  const handlePlay = () => {
    if (audioState.isPlaying) {
      window.electron.transportPause();
    } else {
      window.electron.transportPlay();
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
            title="Record"
            className="hover:bg-red-500/10 transition-all"
          >
            <Circle className="h-5 w-5 fill-current text-red-500" />
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
        </div>
      </div>

      {/* Divider */}
      <div className="h-12 w-px bg-gradient-to-b from-transparent via-border to-transparent" />

      {/* Time Signature */}
      <motion.div
        className="flex flex-col"
        whileHover={{ scale: 1.02 }}
        transition={{ type: 'spring', stiffness: 400, damping: 10 }}
      >
        <span className="text-xs text-muted-foreground font-medium">Time Sig</span>
        <span className="text-lg font-mono font-semibold">
          {audioState.timeSignature.numerator}/{audioState.timeSignature.denominator}
        </span>
      </motion.div>

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
