import { useEffect, useState, useCallback } from 'react';
import { AnimatePresence } from 'framer-motion';
import { AudioState } from './types/audio';
import TransportBar from './components/TransportBar';
import LeftPanel from './components/LeftPanel';
import CenterPanel from './components/CenterPanel';
import RightPanel from './components/RightPanel';
import WingmanSidebar from './components/WingmanSidebar';
import PianoRoll from './components/PianoRoll';
import FileMenu from './components/FileMenu';
import { NotificationContainer, NotificationProps } from './components/Notification';
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
  const [notifications, setNotifications] = useState<NotificationProps[]>([]);

  // Notification helpers
  const showNotification = (type: NotificationProps['type'], message: string) => {
    const id = Date.now().toString();
    const notification: NotificationProps = {
      id,
      type,
      message,
      onClose: removeNotification,
    };
    setNotifications((prev) => [...prev, notification]);
  };

  const removeNotification = (id: string) => {
    setNotifications((prev) => prev.filter((n) => n.id !== id));
  };

  // Project handlers
  const handleNewProject = useCallback(async () => {
    try {
      const result = await window.electron.newProject();
      if (result.success) {
        showNotification('success', 'New project created');
        console.log('🆕 New project created');
      } else {
        showNotification('error', `Failed to create new project: ${result.error}`);
      }
    } catch (error) {
      showNotification('error', 'Failed to create new project');
      console.error('Error creating new project:', error);
    }
  }, []);

  const handleSaveProject = useCallback(async () => {
    try {
      const result = await window.electron.saveProject();
      if (result.success && !result.canceled) {
        showNotification('success', `Project saved: ${result.fileName}`);
        console.log(`💾 Project saved: ${result.filePath}`);
      } else if (result.canceled) {
        console.log('💾 Save canceled');
      } else {
        showNotification('error', `Failed to save project: ${result.error}`);
      }
    } catch (error) {
      showNotification('error', 'Failed to save project');
      console.error('Error saving project:', error);
    }
  }, []);

  const handleLoadProject = useCallback(async () => {
    try {
      const result = await window.electron.loadProject();
      if (result.success && !result.canceled) {
        showNotification('success', `Project loaded: ${result.fileName}`);
        console.log(`📂 Project loaded: ${result.filePath}`);
      } else if (result.canceled) {
        console.log('📂 Load canceled');
      } else {
        showNotification('error', `Failed to load project: ${result.error}`);
      }
    } catch (error) {
      showNotification('error', 'Failed to load project');
      console.error('Error loading project:', error);
    }
  }, []);

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

    // Global keyboard shortcuts
    const handleGlobalKeyDown = (e: KeyboardEvent) => {
      const isMac = navigator.platform.toUpperCase().includes('MAC');
      const cmdOrCtrl = isMac ? e.metaKey : e.ctrlKey;

      // Space: Play/Pause (only if not typing in an input)
      if (e.code === 'Space' && !isTypingInInput(e)) {
        e.preventDefault();
        window.electron.playPause();
        console.log('⏯️ Play/Pause toggled');
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
        handleSaveProject();
      }

      // Ctrl/Cmd+N: New project
      if (cmdOrCtrl && e.key === 'n') {
        e.preventDefault();
        handleNewProject();
      }

      // Ctrl/Cmd+O: Open project
      if (cmdOrCtrl && e.key === 'o') {
        e.preventDefault();
        handleLoadProject();
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
      unsubscribe();
      window.removeEventListener('keydown', handleGlobalKeyDown);
    };
  }, [pianoRollTrack, isWingmanOpen, handleNewProject, handleSaveProject, handleLoadProject]);

  return (
    <div className="h-screen w-screen flex flex-col bg-background text-foreground overflow-hidden">
      {/* Title Bar */}
      <div className="h-8 bg-black/40 flex items-center justify-between px-4 select-none drag-region">
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2">
            <span className="text-sm font-semibold">Vexel DAW</span>
            <span className="text-xs text-muted-foreground">v0.1.0</span>
          </div>
          <div className="no-drag">
            <FileMenu
              onNewProject={handleNewProject}
              onSaveProject={handleSaveProject}
              onLoadProject={handleLoadProject}
            />
          </div>
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

      {/* Notifications */}
      <NotificationContainer notifications={notifications} />
    </div>
  );
}

export default App;
