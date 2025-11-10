import { useEffect, useState, useCallback, useRef } from 'react';
import { AnimatePresence } from 'framer-motion';
import TransportBar from './components/TransportBar';
import LeftPanel from './components/LeftPanel';
import CenterPanel from './components/CenterPanel';
import RightPanel from './components/RightPanel';
import WingmanSidebar from './components/WingmanSidebar';
import PianoRoll from './components/PianoRoll';
import { engineClient } from './lib/engineClient';
import { useStore, useProjectState, usePreferences } from './lib/store';
import './App.css';

function App() {
  // Use Zustand store instead of local state
  const projectState = useProjectState();
  const { wingmanOpen, wingmanPosition } = usePreferences();
  const setWingmanOpen = useStore((state) => state.setWingmanOpen);
  const setWingmanPosition = useStore((state) => state.setWingmanPosition);
  const updateProjectState = useStore((state) => state.updateProjectState);
  const setEngineConnected = useStore((state) => state.setEngineConnected);
  const setEngineMode = useStore((state) => state.setEngineMode);

  // Keep piano roll as local state for now (UI-only)
  const [pianoRollTrack, setPianoRollTrack] = useState<{ id: string; name: string } | null>(null);

  // Store latest state in refs for stable access (Pattern from web search)
  const pianoRollTrackRef = useRef(pianoRollTrack);
  const wingmanOpenRef = useRef(wingmanOpen);
  const isPlayingRef = useRef(projectState.isPlaying);

  // Update refs when state changes (no side effects)
  useEffect(() => {
    pianoRollTrackRef.current = pianoRollTrack;
  }, [pianoRollTrack]);

  useEffect(() => {
    wingmanOpenRef.current = wingmanOpen;
  }, [wingmanOpen]);

  useEffect(() => {
    isPlayingRef.current = projectState.isPlaying;
  }, [projectState.isPlaying]);

  // Effect 1: Connect to engine ONCE on mount (never disconnects except on unmount)
  useEffect(() => {
    console.log('🎯 Vexel DAW initialized');

    // Connect to engine and wire up event listeners
    const initializeEngine = async () => {
      try {
        await engineClient.connect();
        setEngineConnected(true);
        setEngineMode(engineClient.getMode());
        console.log('✅ Engine connected:', engineClient.getMode());

        // Subscribe to engine events
        const unsubscribe = engineClient.onEvent((event) => {
          console.log('🔄 Engine event:', event.type, event.data);

          if (event.type === 'state:updated' || event.type === 'state:initial') {
            updateProjectState(event.data);
          }
        });

        return unsubscribe;
      } catch (error) {
        console.error('❌ Failed to connect to engine:', error);
        setEngineConnected(false);
      }
    };

    const unsubscribePromise = initializeEngine();

    // Cleanup: Only disconnect on unmount
    return () => {
      console.log('🔌 Disconnecting engine (component unmount)');
      unsubscribePromise?.then((unsubscribe) => {
        if (unsubscribe) {
          unsubscribe();
        }
      });
      engineClient.disconnect();
    };
  }, []); // Empty deps - runs ONCE on mount

  // Effect 2: Setup keyboard handlers (rebinds when needed, but no engine disconnect)
  useEffect(() => {
    // Global keyboard shortcuts
    const handleGlobalKeyDown = (e: KeyboardEvent) => {
      const isMac = navigator.platform.toUpperCase().includes('MAC');
      const cmdOrCtrl = isMac ? e.metaKey : e.ctrlKey;

      // Helper to check if user is typing in an input
      const isTypingInInput = (e: KeyboardEvent): boolean => {
        const target = e.target as HTMLElement;
        return (
          target.tagName === 'INPUT' ||
          target.tagName === 'TEXTAREA' ||
          target.contentEditable === 'true'
        );
      };

      // Space: Play/Pause (only if not typing in an input)
      if (e.code === 'Space' && !isTypingInInput(e)) {
        e.preventDefault();
        // Read from ref - always has latest value
        if (isPlayingRef.current) {
          engineClient.sendCommand('transport:pause');
        } else {
          engineClient.sendCommand('transport:play');
        }
        console.log('⏯️ Play/Pause toggled');
      }

      // Enter: Stop
      if (e.key === 'Enter' && !isTypingInInput(e)) {
        e.preventDefault();
        engineClient.sendCommand('transport:stop');
        console.log('⏹️ Transport stopped');
      }

      // Escape: Close modals/Piano Roll
      if (e.key === 'Escape') {
        e.preventDefault();
        // Read from refs - always has latest value
        if (pianoRollTrackRef.current) {
          setPianoRollTrack(null);
          console.log('❌ Closed Piano Roll');
        }
        if (wingmanOpenRef.current) {
          setWingmanOpen(false);
          console.log('❌ Closed Wingman');
        }
      }

      // Ctrl/Cmd+S: Save project
      if (cmdOrCtrl && e.key === 's') {
        e.preventDefault();
        console.log('💾 Save project');
        engineClient.sendCommand('project:save');
      }

      // Ctrl/Cmd+N: New project
      if (cmdOrCtrl && e.key === 'n') {
        e.preventDefault();
        console.log('🆕 New project');
        engineClient.sendCommand('project:new');
      }

      // Ctrl/Cmd+O: Open project
      if (cmdOrCtrl && e.key === 'o') {
        e.preventDefault();
        console.log('📂 Open project');
        engineClient.sendCommand('project:open');
      }

      // Ctrl/Cmd+W: Toggle Wingman
      if (cmdOrCtrl && e.key === 'w') {
        e.preventDefault();
        setWingmanOpen(!wingmanOpenRef.current);
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
        engineClient.sendCommand('project:undo');
      }

      // Ctrl/Cmd+Y or Ctrl/Cmd+Shift+Z: Redo
      if (cmdOrCtrl && (e.key === 'y' || (e.key === 'z' && e.shiftKey))) {
        e.preventDefault();
        console.log('↪️ Redo');
        engineClient.sendCommand('project:redo');
      }
    };

    window.addEventListener('keydown', handleGlobalKeyDown);

    // Cleanup: Only remove listener (no engine disconnect)
    return () => {
      window.removeEventListener('keydown', handleGlobalKeyDown);
    };
  }, []); // Empty deps - keyboard handler can read refs for latest values

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
        audioState={projectState}
        onOpenWingman={() => setWingmanOpen(true)}
      />

      {/* Main Content - Tri-Pane Layout */}
      <div className="flex-1 flex min-h-0">
        {/* Left Panel - Browser */}
        <LeftPanel />

        {/* Center Panel - Arrangement/Session View */}
        <CenterPanel
          tracks={projectState.tracks}
          bpm={projectState.tempo}
          onOpenPianoRoll={(trackId, trackName) => setPianoRollTrack({ id: trackId, name: trackName })}
        />

        {/* Right Panel - Mixer/Inspector */}
        <RightPanel tracks={projectState.tracks} />
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
        isOpen={wingmanOpen}
        onClose={() => setWingmanOpen(false)}
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
