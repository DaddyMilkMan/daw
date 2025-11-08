import { Play, Pause, Square, SkipBack, Circle, Sparkles } from 'lucide-react';
import { Button } from './ui/button';
import { AudioState } from '@/types/audio';
import { formatTime } from '@/lib/utils';
import { useState } from 'react';

interface TransportBarProps {
  audioState: AudioState;
  onOpenWingman: () => void;
}

export default function TransportBar({ audioState, onOpenWingman }: TransportBarProps) {
  const [tempo, setTempo] = useState(audioState.tempo);

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
    <div className="h-20 bg-card border-b border-border flex items-center px-4 gap-6">
      {/* Transport Controls */}
      <div className="flex items-center gap-2">
        <Button
          size="icon"
          variant="ghost"
          onClick={handleStop}
          title="Stop"
        >
          <Square className="h-5 w-5" />
        </Button>
        <Button
          size="icon"
          variant="ghost"
          onClick={handlePlay}
          title={audioState.isPlaying ? 'Pause' : 'Play'}
          className={audioState.isPlaying ? 'text-primary' : ''}
        >
          {audioState.isPlaying ? (
            <Pause className="h-5 w-5" />
          ) : (
            <Play className="h-5 w-5" />
          )}
        </Button>
        <Button
          size="icon"
          variant="ghost"
          title="Return to Zero"
          onClick={handleStop}
        >
          <SkipBack className="h-5 w-5" />
        </Button>
        <Button
          size="icon"
          variant={false ? 'default' : 'ghost'}
          title="Record"
        >
          <Circle className="h-5 w-5 fill-current text-red-500" />
        </Button>
      </div>

      {/* Divider */}
      <div className="h-12 w-px bg-border" />

      {/* Time Display */}
      <div className="flex flex-col">
        <span className="text-xs text-muted-foreground">Position</span>
        <span className="text-lg font-mono font-semibold">
          {formatTime(audioState.currentBar, audioState.timeSignature.numerator)}
        </span>
      </div>

      {/* Divider */}
      <div className="h-12 w-px bg-border" />

      {/* Tempo */}
      <div className="flex flex-col">
        <span className="text-xs text-muted-foreground">Tempo</span>
        <div className="flex items-center gap-2">
          <input
            type="number"
            value={tempo}
            onChange={handleTempoChange}
            onBlur={handleTempoBlur}
            min="20"
            max="999"
            className="w-20 px-2 py-1 bg-background border border-input rounded text-lg font-mono font-semibold focus:outline-none focus:ring-2 focus:ring-primary"
          />
          <span className="text-sm text-muted-foreground">BPM</span>
        </div>
      </div>

      {/* Divider */}
      <div className="h-12 w-px bg-border" />

      {/* Time Signature */}
      <div className="flex flex-col">
        <span className="text-xs text-muted-foreground">Time Sig</span>
        <span className="text-lg font-mono font-semibold">
          {audioState.timeSignature.numerator}/{audioState.timeSignature.denominator}
        </span>
      </div>

      {/* Spacer */}
      <div className="flex-1" />

      {/* Wingman Button */}
      <Button
        onClick={onOpenWingman}
        className="bg-gradient-to-r from-primary to-primary/80 hover:from-primary/90 hover:to-primary/70"
      >
        <Sparkles className="h-4 w-4 mr-2" />
        Wingman AI
      </Button>

      {/* Divider */}
      <div className="h-12 w-px bg-border" />

      {/* Master Volume */}
      <div className="flex items-center gap-3">
        <span className="text-sm text-muted-foreground">Master</span>
        <div className="w-32 h-2 bg-background rounded-full overflow-hidden">
          <div className="h-full bg-primary" style={{ width: '80%' }} />
        </div>
        <span className="text-sm font-mono">-6 dB</span>
      </div>
    </div>
  );
}
