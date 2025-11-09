import { useEffect, useState } from 'react';
import { AnimatePresence } from 'framer-motion';
import { AudioState } from './types/audio';
import TransportBar from './components/TransportBar';
import LeftPanel from './components/LeftPanel';
import CenterPanel from './components/CenterPanel';
import RightPanel from './components/RightPanel';
import WingmanSidebar from './components/WingmanSidebar';
import PianoRoll from './components/PianoRoll';
import './App.css';

function App() {
  const [audioState, setAudioState] = useState<AudioState>({
    tempo: 120,
    timeSignature: { numerator: 4, denominator: 4 },
    isPlaying: false,
    currentBar: 0,
    tracks: [],
  });
  const [isWingmanOpen, setIsWingmanOpen] = useState(false);
  const [wingmanPosition, setWingmanPosition] = useState<'left' | 'right'>('right');
  const [pianoRollTrack, setPianoRollTrack] = useState<{ id: string; name: string } | null>(null);

  useEffect(() => {
    console.log('🎯 Vexel DAW initialized');

    // Get initial audio state
    window.electron.getAudioState().then((state) => {
      console.log('📊 Initial audio state:', state);
      setAudioState(state);
    });

    // Subscribe to audio state updates
    const unsubscribe = window.electron.onAudioStateUpdate((state) => {
      console.log('🔄 Audio state updated:', state);
      setAudioState(state);
    });

    return () => {
      unsubscribe();
    };
  }, []);

  return (
    <div className="h-screen w-screen flex flex-col bg-background text-foreground overflow-hidden">
      {/* Title Bar */}
      <div className="h-8 bg-black/40 flex items-center justify-between px-4 select-none drag-region">
        <div className="flex items-center gap-2">
          <span className="text-sm font-semibold">Vexel DAW</span>
          <span className="text-xs text-muted-foreground">v0.1.0</span>
        </div>
        <div className="flex items-center gap-2 no-drag">
          <button
            onClick={() => window.electron.windowMinimize()}
            className="w-8 h-6 hover:bg-white/10 rounded flex items-center justify-center"
          >
            <span className="text-xs">─</span>
          </button>
          <button
            onClick={() => window.electron.windowMaximize()}
            className="w-8 h-6 hover:bg-white/10 rounded flex items-center justify-center"
          >
            <span className="text-xs">□</span>
          </button>
          <button
            onClick={() => window.electron.windowClose()}
            className="w-8 h-6 hover:bg-red-500/80 rounded flex items-center justify-center"
          >
            <span className="text-xs">×</span>
          </button>
        </div>
      </div>

      {/* Transport Bar */}
      <TransportBar
        audioState={audioState}
        onOpenWingman={() => setIsWingmanOpen(true)}
      />

      {/* Main Content - Tri-Pane Layout */}
      <div className="flex-1 flex min-h-0">
        {/* Left Panel - Browser */}
        <LeftPanel />

        {/* Center Panel - Arrangement/Session View */}
        <CenterPanel
          tracks={audioState.tracks}
          onOpenPianoRoll={(trackId, trackName) => setPianoRollTrack({ id: trackId, name: trackName })}
        />

        {/* Right Panel - Mixer/Inspector */}
        <RightPanel tracks={audioState.tracks} />
      </div>

      {/* Bottom Panel - Editor (Piano Roll / Audio Editor) */}
      <div className="h-64 bg-card border-t border-border">
        <div className="h-full flex items-center justify-center text-muted-foreground">
          <div className="text-center">
            <p className="text-lg font-semibold mb-2">Piano Roll / Audio Editor</p>
            <p className="text-sm">Double-click a clip to edit</p>
          </div>
        </div>
      </div>

      {/* Wingman AI Sidebar */}
      <WingmanSidebar
        isOpen={isWingmanOpen}
        onClose={() => setIsWingmanOpen(false)}
        position={wingmanPosition}
        onPositionChange={setWingmanPosition}
      />

      {/* Piano Roll Editor */}
      <AnimatePresence>
        {pianoRollTrack && (
          <PianoRoll
            trackId={pianoRollTrack.id}
            trackName={pianoRollTrack.name}
            onClose={() => setPianoRollTrack(null)}
          />
        )}
      </AnimatePresence>
    </div>
  );
}

export default App;
