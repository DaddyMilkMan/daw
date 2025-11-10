import { AudioTrack } from '../audio/AudioEngine';
import { Volume2, PanelRight, Sliders, Plus, Minus, MoreHorizontal, Activity } from 'lucide-react';
import { useState, useRef, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { engineClient } from '@/lib/engineClient';

interface RightPanelProps {
  tracks: AudioTrack[];
}

export default function RightPanel({ tracks }: RightPanelProps) {
  const [tab, setTab] = useState<'mixer' | 'inspector'>('mixer');

  return (
    <div className="w-72 bg-card border-l border-border flex flex-col">
      {/* Header */}
      <div className="h-12 border-b border-border/50 flex items-center bg-card/50 backdrop-blur-xl">
        <motion.button
          whileTap={{ scale: 0.98 }}
          onClick={() => setTab('mixer')}
          className={`flex-1 h-full flex items-center justify-center gap-2 border-r border-border/50 transition-colors ${
            tab === 'mixer'
              ? 'bg-accent/50 text-foreground'
              : 'hover:bg-accent/20 text-muted-foreground'
          }`}
        >
          <Sliders className="h-4 w-4" />
          <span className="text-sm font-medium">Mixer</span>
        </motion.button>
        <motion.button
          whileTap={{ scale: 0.98 }}
          onClick={() => setTab('inspector')}
          className={`flex-1 h-full flex items-center justify-center gap-2 transition-colors ${
            tab === 'inspector'
              ? 'bg-accent/50 text-foreground'
              : 'hover:bg-accent/20 text-muted-foreground'
          }`}
        >
          <PanelRight className="h-4 w-4" />
          <span className="text-sm font-medium">Inspector</span>
        </motion.button>
      </div>

      {/* Content */}
      <div className="flex-1 overflow-y-auto">
        <AnimatePresence mode="wait">
          {tab === 'mixer' ? (
            <MixerView key="mixer" tracks={tracks} />
          ) : (
            <InspectorView key="inspector" />
          )}
        </AnimatePresence>
      </div>
    </div>
  );
}

function MixerView({ tracks }: { tracks: AudioTrack[] }) {
  if (tracks.length === 0) {
    return (
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        exit={{ opacity: 0 }}
        className="h-full flex items-center justify-center text-muted-foreground text-sm p-4 text-center"
      >
        <div>
          <motion.div
            animate={{ y: [0, -5, 0] }}
            transition={{ repeat: Infinity, duration: 2 }}
          >
            <Sliders className="h-8 w-8 mx-auto mb-2 opacity-50" />
          </motion.div>
          <p>No tracks to mix</p>
          <p className="text-xs mt-1">Add tracks to start mixing</p>
        </div>
      </motion.div>
    );
  }

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      className="p-3 space-y-4"
    >
      <AnimatePresence>
        {tracks.map((track, index) => (
          <ChannelStrip key={track.id} track={track} index={index} />
        ))}
      </AnimatePresence>

      {/* Master Channel */}
      <div className="border-t-2 border-primary/30 pt-4">
        <MasterChannel />
      </div>
    </motion.div>
  );
}

interface ChannelStripProps {
  track: AudioTrack;
  index: number;
}

function ChannelStrip({ track, index }: ChannelStripProps) {
  const [volume, setVolume] = useState(track.volume);
  const [pan, setPan] = useState(track.pan);
  const [muted, setMuted] = useState(track.muted);
  const [solo, setSolo] = useState(track.soloed);
  const [level, setLevel] = useState(0);
  const [peak, setPeak] = useState(0);
  const [showEQ, setShowEQ] = useState(false);
  const [showSends, setShowSends] = useState(false);

  // Sync local state with track props
  useEffect(() => {
    setVolume(track.volume);
    setPan(track.pan);
    setMuted(track.muted);
    setSolo(track.soloed);
  }, [track.volume, track.pan, track.muted, track.soloed]);

  // Simulated VU meter (will be replaced with real engine data in Phase 1)
  useEffect(() => {
    const interval = setInterval(() => {
      const newLevel = Math.random() * 0.7 + 0.1;
      setLevel(newLevel);
      if (newLevel > peak) {
        setPeak(newLevel);
        setTimeout(() => setPeak(0), 1500);
      }
    }, 50); // 20Hz update rate

    return () => clearInterval(interval);
  }, [peak]);

  // Calculate dB value
  const volumeDB = volume === 0 ? -Infinity : Math.round(20 * Math.log10(volume));
  const panDisplay =
    pan === 0 ? 'C' : pan > 0 ? `${Math.round(pan * 100)}R` : `${Math.round(Math.abs(pan) * 100)}L`;

  return (
    <motion.div
      initial={{ opacity: 0, x: -20 }}
      animate={{ opacity: 1, x: 0 }}
      exit={{ opacity: 0, x: -20 }}
      transition={{
        type: 'spring',
        stiffness: 300,
        damping: 25,
        delay: index * 0.05,
      }}
      className="bg-gradient-to-br from-card to-card/80 border border-border/50 rounded-lg p-3 space-y-3"
    >
      {/* Track Header */}
      <div className="flex items-center justify-between">
        <div className="flex-1 min-w-0">
          <div className="text-sm font-medium truncate">{track.name}</div>
          <div className="text-xs text-muted-foreground">{track.type.toUpperCase()}</div>
        </div>
        <motion.button
          whileHover={{ scale: 1.05 }}
          whileTap={{ scale: 0.95 }}
          className="p-1 rounded hover:bg-accent transition-colors"
        >
          <MoreHorizontal className="h-4 w-4 text-muted-foreground" />
        </motion.button>
      </div>

      {/* Insert Effects Slots */}
      <div className="space-y-1">
        <div className="flex items-center justify-between">
          <span className="text-xs text-muted-foreground font-medium">INSERTS</span>
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            className="p-0.5 rounded hover:bg-accent transition-colors"
          >
            <Plus className="h-3 w-3 text-muted-foreground" />
          </motion.button>
        </div>
        <div className="space-y-1">
          {[1, 2].map((slot) => (
            <motion.div
              key={slot}
              whileHover={{ backgroundColor: 'rgba(255,255,255,0.05)' }}
              className="px-2 py-1 bg-background/50 border border-dashed border-border/30 rounded text-[10px] text-muted-foreground text-center cursor-pointer"
            >
              Empty Slot
            </motion.div>
          ))}
        </div>
      </div>

      {/* EQ Section */}
      <div>
        <motion.button
          whileHover={{ scale: 1.01 }}
          whileTap={{ scale: 0.99 }}
          onClick={() => setShowEQ(!showEQ)}
          className="w-full px-2 py-1 bg-accent/30 hover:bg-accent/50 rounded text-xs font-medium flex items-center justify-between transition-colors"
        >
          <span>EQ</span>
          <Activity className="h-3 w-3" />
        </motion.button>
        <AnimatePresence>
          {showEQ && (
            <motion.div
              initial={{ height: 0, opacity: 0 }}
              animate={{ height: 'auto', opacity: 1 }}
              exit={{ height: 0, opacity: 0 }}
              transition={{ duration: 0.2 }}
              className="overflow-hidden"
            >
              <EQVisualizer />
            </motion.div>
          )}
        </AnimatePresence>
      </div>

      {/* Sends Section */}
      <div>
        <motion.button
          whileHover={{ scale: 1.01 }}
          whileTap={{ scale: 0.99 }}
          onClick={() => setShowSends(!showSends)}
          className="w-full px-2 py-1 bg-accent/30 hover:bg-accent/50 rounded text-xs font-medium flex items-center justify-between transition-colors"
        >
          <span>SENDS</span>
          <span className="text-[10px] opacity-70">{showSends ? '−' : '+'}</span>
        </motion.button>
        <AnimatePresence>
          {showSends && (
            <motion.div
              initial={{ height: 0, opacity: 0 }}
              animate={{ height: 'auto', opacity: 1 }}
              exit={{ height: 0, opacity: 0 }}
              transition={{ duration: 0.2 }}
              className="overflow-hidden mt-2 space-y-2"
            >
              <SendKnob label="Reverb" value={0.3} />
              <SendKnob label="Delay" value={0.15} />
            </motion.div>
          )}
        </AnimatePresence>
      </div>

      {/* Pan Knob */}
      <div>
        <div className="flex items-center justify-between mb-1">
          <span className="text-xs text-muted-foreground">PAN</span>
          <span className="text-xs font-mono font-medium">{panDisplay}</span>
        </div>
        <div className="relative">
          <input
            type="range"
            min="-1"
            max="1"
            step="0.01"
            value={pan}
            onChange={(e) => {
              const newPan = parseFloat(e.target.value);
              setPan(newPan);
              engineClient.sendCommand('track:setPan', { trackId: track.id, pan: newPan });
            }}
            className="w-full h-1 bg-accent rounded-full appearance-none cursor-pointer [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-3 [&::-webkit-slider-thumb]:h-3 [&::-webkit-slider-thumb]:rounded-full [&::-webkit-slider-thumb]:bg-primary [&::-webkit-slider-thumb]:cursor-pointer"
          />
          <div
            className="absolute top-1/2 w-0.5 h-2 bg-foreground/30 pointer-events-none"
            style={{ left: '50%', transform: 'translate(-50%, -50%)' }}
          />
        </div>
      </div>

      {/* Volume Fader & Meter */}
      <div className="flex items-center gap-3">
        {/* VU Meter */}
        <VUMeter level={level} peak={peak} />

        {/* Fader */}
        <div className="flex-1">
          <div className="flex items-center justify-between mb-1">
            <span className="text-xs text-muted-foreground">VOLUME</span>
            <span className="text-xs font-mono font-medium">
              {volumeDB === -Infinity ? '-∞' : volumeDB > 0 ? `+${volumeDB}` : volumeDB} dB
            </span>
          </div>
          <div className="relative h-24 bg-accent/30 rounded-full overflow-hidden">
            {/* Fader Track Gradient */}
            <div className="absolute inset-0 bg-gradient-to-t from-green-500/20 via-yellow-500/20 to-red-500/20" />

            {/* Fader Fill */}
            <motion.div
              className="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-primary via-primary/80 to-primary/60 rounded-full"
              style={{ height: `${volume * 100}%` }}
              initial={false}
              animate={{ height: `${volume * 100}%` }}
              transition={{ type: 'spring', stiffness: 300, damping: 30 }}
            />

            {/* Fader Input */}
            <input
              type="range"
              min="0"
              max="1"
              step="0.01"
              value={volume}
              onChange={(e) => {
                const newVolume = parseFloat(e.target.value);
                setVolume(newVolume);
                engineClient.sendCommand('track:setVolume', { trackId: track.id, volume: newVolume });
              }}
              orient="vertical"
              className="absolute inset-0 w-full h-full opacity-0 cursor-ns-resize"
            />

            {/* Fader Thumb */}
            <motion.div
              className="absolute left-1/2 w-full h-2 bg-foreground/80 border-2 border-background shadow-lg pointer-events-none rounded-sm"
              style={{
                bottom: `calc(${volume * 100}% - 4px)`,
                transform: 'translateX(-50%)',
              }}
              initial={false}
              animate={{ bottom: `calc(${volume * 100}% - 4px)` }}
              transition={{ type: 'spring', stiffness: 300, damping: 30 }}
            />
          </div>
        </div>
      </div>

      {/* Solo/Mute Buttons */}
      <div className="flex gap-2">
        <motion.button
          whileHover={{ scale: 1.05 }}
          whileTap={{ scale: 0.95 }}
          onClick={() => {
            const newMuted = !muted;
            setMuted(newMuted);
            engineClient.sendCommand('track:setMute', { trackId: track.id, muted: newMuted });
          }}
          className={`flex-1 px-3 py-1.5 text-xs font-bold rounded transition-all ${
            muted
              ? 'bg-yellow-500/80 text-yellow-950 shadow-lg shadow-yellow-500/50'
              : 'bg-secondary hover:bg-secondary/80'
          }`}
        >
          M
        </motion.button>
        <motion.button
          whileHover={{ scale: 1.05 }}
          whileTap={{ scale: 0.95 }}
          onClick={() => {
            const newSolo = !solo;
            setSolo(newSolo);
            engineClient.sendCommand('track:setSolo', { trackId: track.id, solo: newSolo });
          }}
          className={`flex-1 px-3 py-1.5 text-xs font-bold rounded transition-all ${
            solo
              ? 'bg-primary/80 text-primary-foreground shadow-lg shadow-primary/50'
              : 'bg-secondary hover:bg-secondary/80'
          }`}
        >
          S
        </motion.button>
      </div>
    </motion.div>
  );
}

function VUMeter({ level, peak }: { level: number; peak: number }) {
  return (
    <div className="w-6 h-24 bg-background/50 rounded-full overflow-hidden border border-border/50 relative">
      {/* Meter segments */}
      <div className="absolute inset-0 flex flex-col-reverse">
        {Array.from({ length: 20 }).map((_, i) => {
          const segmentLevel = i / 20;
          const isActive = segmentLevel <= level;
          const isPeak = segmentLevel <= peak && segmentLevel > level;

          let color = 'bg-green-500';
          if (segmentLevel > 0.8) color = 'bg-red-500';
          else if (segmentLevel > 0.6) color = 'bg-yellow-500';

          return (
            <motion.div
              key={i}
              className={`flex-1 mx-0.5 mb-0.5 rounded-sm transition-all ${
                isActive || isPeak ? color : 'bg-border/30'
              }`}
              animate={{
                opacity: isActive ? 1 : isPeak ? 0.8 : 0.3,
                scale: isActive ? 1 : 0.8,
              }}
              transition={{ duration: 0.1 }}
            />
          );
        })}
      </div>
    </div>
  );
}

function EQVisualizer() {
  const points = 50;
  const width = 240;
  const height = 60;

  // Generate EQ curve (simulated)
  const curve = Array.from({ length: points }, (_, i) => {
    const x = i / points;
    // Simulate a typical EQ curve with some boosts/cuts
    return (
      30 +
      10 * Math.sin(x * Math.PI * 2) +
      5 * Math.sin(x * Math.PI * 4) +
      3 * Math.cos(x * Math.PI * 6)
    );
  });

  const pathData = curve
    .map((y, i) => {
      const x = (i / points) * width;
      return `${i === 0 ? 'M' : 'L'} ${x} ${y}`;
    })
    .join(' ');

  return (
    <div className="mt-2 p-2 bg-background/50 rounded border border-border/30">
      <svg
        width="100%"
        height={height}
        viewBox={`0 0 ${width} ${height}`}
        className="overflow-visible"
      >
        {/* Grid lines */}
        <g stroke="currentColor" strokeWidth="0.5" opacity="0.1">
          {[0, height / 4, height / 2, (3 * height) / 4, height].map((y) => (
            <line key={y} x1="0" y1={y} x2={width} y2={y} />
          ))}
        </g>

        {/* EQ Curve */}
        <motion.path
          d={pathData}
          fill="none"
          stroke="url(#eqGradient)"
          strokeWidth="2"
          strokeLinecap="round"
          initial={{ pathLength: 0, opacity: 0 }}
          animate={{ pathLength: 1, opacity: 1 }}
          transition={{ duration: 0.5 }}
        />

        {/* Gradient Definition */}
        <defs>
          <linearGradient id="eqGradient" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stopColor="#3b82f6" />
            <stop offset="50%" stopColor="#8b5cf6" />
            <stop offset="100%" stopColor="#ec4899" />
          </linearGradient>
        </defs>
      </svg>

      {/* Frequency Labels */}
      <div className="flex justify-between text-[9px] text-muted-foreground mt-1">
        <span>20Hz</span>
        <span>1kHz</span>
        <span>20kHz</span>
      </div>
    </div>
  );
}

function SendKnob({ label, value }: { label: string; value: number }) {
  const [sendValue, setSendValue] = useState(value);

  return (
    <div className="flex items-center gap-2">
      <span className="text-xs text-muted-foreground w-12">{label}</span>
      <div className="flex-1 relative">
        <input
          type="range"
          min="0"
          max="1"
          step="0.01"
          value={sendValue}
          onChange={(e) => setSendValue(parseFloat(e.target.value))}
          className="w-full h-1 bg-accent rounded-full appearance-none cursor-pointer [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-2 [&::-webkit-slider-thumb]:h-2 [&::-webkit-slider-thumb]:rounded-full [&::-webkit-slider-thumb]:bg-primary [&::-webkit-slider-thumb]:cursor-pointer"
        />
      </div>
      <span className="text-xs font-mono w-8 text-right">{Math.round(sendValue * 100)}%</span>
    </div>
  );
}

function MasterChannel() {
  const [volume, setVolume] = useState(0.75);
  const [level, setLevel] = useState(0);
  const [peak, setPeak] = useState(0);

  useEffect(() => {
    const interval = setInterval(() => {
      const newLevel = Math.random() * 0.7 + 0.2;
      setLevel(newLevel);
      if (newLevel > peak) {
        setPeak(newLevel);
        setTimeout(() => setPeak(0), 1500);
      }
    }, 100);

    return () => clearInterval(interval);
  }, []);

  const volumeDB = volume === 0 ? -Infinity : Math.round(20 * Math.log10(volume));

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ delay: 0.3 }}
      className="bg-gradient-to-br from-primary/10 to-primary/5 border-2 border-primary/30 rounded-lg p-3 space-y-3"
    >
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <Volume2 className="h-4 w-4 text-primary" />
          <div className="text-sm font-bold text-primary">MASTER</div>
        </div>
      </div>

      {/* Master Fader & Meter */}
      <div className="flex items-center gap-3">
        <VUMeter level={level} peak={peak} />

        <div className="flex-1">
          <div className="flex items-center justify-between mb-1">
            <span className="text-xs text-muted-foreground font-medium">OUTPUT</span>
            <span className="text-xs font-mono font-bold text-primary">
              {volumeDB === -Infinity ? '-∞' : volumeDB > 0 ? `+${volumeDB}` : volumeDB} dB
            </span>
          </div>
          <div className="relative h-32 bg-accent/30 rounded-full overflow-hidden">
            <div className="absolute inset-0 bg-gradient-to-t from-green-500/20 via-yellow-500/20 to-red-500/20" />
            <motion.div
              className="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-primary via-primary/80 to-primary/60 rounded-full"
              style={{ height: `${volume * 100}%` }}
              initial={false}
              animate={{ height: `${volume * 100}%` }}
              transition={{ type: 'spring', stiffness: 300, damping: 30 }}
            />
            <input
              type="range"
              min="0"
              max="1"
              step="0.01"
              value={volume}
              onChange={(e) => setVolume(parseFloat(e.target.value))}
              orient="vertical"
              className="absolute inset-0 w-full h-full opacity-0 cursor-ns-resize"
            />
            <motion.div
              className="absolute left-1/2 w-full h-3 bg-foreground border-2 border-background shadow-lg pointer-events-none rounded"
              style={{
                bottom: `calc(${volume * 100}% - 6px)`,
                transform: 'translateX(-50%)',
              }}
              initial={false}
              animate={{ bottom: `calc(${volume * 100}% - 6px)` }}
              transition={{ type: 'spring', stiffness: 300, damping: 30 }}
            />
          </div>
        </div>
      </div>
    </motion.div>
  );
}

function InspectorView() {
  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      className="h-full flex items-center justify-center text-muted-foreground text-sm p-4 text-center"
    >
      <div>
        <motion.div
          animate={{ rotate: [0, 5, -5, 0] }}
          transition={{ repeat: Infinity, duration: 3 }}
        >
          <PanelRight className="h-8 w-8 mx-auto mb-2 opacity-50" />
        </motion.div>
        <p className="mb-2 font-medium">Inspector</p>
        <p className="text-xs text-muted-foreground">
          Select a track or clip to view properties
        </p>
      </div>
    </motion.div>
  );
}
