import { useEffect, useState, useCallback } from 'react';
import { AnimatePresence } from 'framer-motion';
import { useAudioStore } from './stores/audioStore';
import TransportBar from './components/TransportBar';
import LeftPanel from './components/LeftPanel';
import CenterPanel from './components/CenterPanel';
import RightPanel from './components/RightPanel';
import WingmanSidebar from './components/WingmanSidebar';
import PianoRoll from './components/PianoRoll';
import { useWingmanBridge, useWingmanTransportSync, useWingmanClipInsertion } from './hooks/useWingmanBridge';
import './App.css';

function App() {
  const [isWingmanOpen, setIsWingmanOpen] = useState(false);
  const [wingmanPosition, setWingmanPosition] = useState<'left' | 'right'>('right');
  const [pianoRollTrack, setPianoRollTrack] = useState<{ id: string; name: string } | null>(null);
  const [generatedClips, setGeneratedClips] = useState<any[]>([]);

  // Wingman AI Bridge integration
  const wingman = useWingmanBridge();

  // Sync transport state with Wingman AI
  useWingmanTransportSync(audioState.isPlaying, audioState.tempo);

  // Handle AI-generated clip insertion
  const handleClipInsertion = useCallback((data: any) => {
    console.log('🎵 Inserting AI-generated clip:', data);

    // Add to generated clips state (to be picked up by CenterPanel)
    setGeneratedClips(prev => [...prev, data]);

    // Show notification or toast
    console.log(`✅ Clip inserted: ${data.clip?.name || 'Untitled'}`);
  }, []);

  useWingmanClipInsertion(handleClipInsertion);

  // Get audio store state and actions
  const {
    tracks,
    transport,
    initEngine,
    play,
    pause,
    stop,
    dispose,
  } = useAudioStore();

  useEffect(() => {
    console.log('🎯 Vexel DAW initialized');

    // Initialize audio engine
    initEngine();

    // Global keyboard shortcuts
    const handleGlobalKeyDown = (e: KeyboardEvent) => {
      const isMac = navigator.platform.toUpperCase().includes('MAC');
      const cmdOrCtrl = isMac ? e.metaKey : e.ctrlKey;

      // Space: Play/Pause (only if not typing in an input)
      if (e.code === 'Space' && !isTypingInInput(e)) {
        e.preventDefault();
        if (transport.isPlaying) {
          pause();
        } else {
          play();
        }
        console.log('⏯️ Play/Pause toggled');
      }

      // Enter: Stop
      if (e.key === 'Enter' && !isTypingInInput(e)) {
        e.preventDefault();
        stop();
        console.log('⏹️ Transport stopped');
      }

      // Tab: Switch between Session and Arrangement views (handled by CenterPanel)
      // This is handled locally in CenterPanel but we could add it here for consistency

      // Escape: Close modals/Piano Roll
      if (e.key === 'Escape') {
        e.preventDefault();
        if (pianoRollTrack) {
          setPianoRollTrack(null);
          console.log('❌ Closed Piano Roll');
        }
        if (isWingmanOpen) {
          setIsWingmanOpen(false);
          console.log('❌ Closed Wingman');
        }
      }

      // Ctrl/Cmd+S: Save project
      if (cmdOrCtrl && e.key === 's') {
        e.preventDefault();
        console.log('💾 Save project');
        // TODO: Implement save functionality
      }

      // Ctrl/Cmd+N: New project
      if (cmdOrCtrl && e.key === 'n') {
        e.preventDefault();
        console.log('🆕 New project');
        // TODO: Implement new project functionality
      }

      // Ctrl/Cmd+O: Open project
      if (cmdOrCtrl && e.key === 'o') {
        e.preventDefault();
        console.log('📂 Open project');
        // TODO: Implement open project functionality
      }

      // Ctrl/Cmd+W: Toggle Wingman
      if (cmdOrCtrl && e.key === 'w') {
        e.preventDefault();
        setIsWingmanOpen(!isWingmanOpen);
        console.log('🤖 Toggled Wingman');
      }

      // Ctrl/Cmd+/: Show keyboard shortcuts help
      if (cmdOrCtrl && e.key === '/') {
        e.preventDefault();
        console.log('⌨️ Show keyboard shortcuts');
        // TODO: Show keyboard shortcuts modal
      }

      // Ctrl/Cmd+Z: Undo
      if (cmdOrCtrl && e.key === 'z' && !e.shiftKey) {
        e.preventDefault();
        console.log('↩️ Undo');
        // TODO: Implement undo - will be handled by history manager
      }

      // Ctrl/Cmd+Y or Ctrl/Cmd+Shift+Z: Redo
      if (cmdOrCtrl && (e.key === 'y' || (e.key === 'z' && e.shiftKey))) {
        e.preventDefault();
        console.log('↪️ Redo');
        // TODO: Implement redo - will be handled by history manager
      }
    };

    // Helper to check if user is typing in an input
    const isTypingInInput = (e: KeyboardEvent): boolean => {
      const target = e.target as HTMLElement;
      return (
        target.tagName === 'INPUT' ||
        target.tagName === 'TEXTAREA' ||
        target.contentEditable === 'true'
      );
    };

    window.addEventListener('keydown', handleGlobalKeyDown);

    return () => {
      window.removeEventListener('keydown', handleGlobalKeyDown);
      dispose(); // Cleanup audio engine on unmount
    };
  }, [pianoRollTrack, isWingmanOpen, transport.isPlaying, play, pause, stop, dispose]);

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
            onClick={() => window.electron?.windowMinimize?.()}
            className="w-8 h-6 hover:bg-white/10 rounded flex items-center justify-center"
          >
            <span className="text-xs">─</span>
          </button>
          <button
            onClick={() => window.electron?.windowMaximize?.()}
            className="w-8 h-6 hover:bg-white/10 rounded flex items-center justify-center"
          >
            <span className="text-xs">□</span>
          </button>
          <button
            onClick={() => window.electron?.windowClose?.()}
            className="w-8 h-6 hover:bg-red-500/80 rounded flex items-center justify-center"
          >
            <span className="text-xs">×</span>
          </button>
        </div>
      </div>

      {/* Transport Bar */}
      <TransportBar
        transport={transport}
        onOpenWingman={() => setIsWingmanOpen(true)}
      />

      {/* Main Content - Tri-Pane Layout */}
      <div className="flex-1 flex min-h-0">
        {/* Left Panel - Browser */}
        <LeftPanel />

        {/* Center Panel - Arrangement/Session View */}
        <CenterPanel
          tracks={tracks}
          onOpenPianoRoll={(trackId, trackName) => setPianoRollTrack({ id: trackId, name: trackName })}
        />

        {/* Right Panel - Mixer/Inspector */}
        <RightPanel tracks={tracks} />
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
