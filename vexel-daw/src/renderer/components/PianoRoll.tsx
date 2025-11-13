import { useState, useRef, useCallback, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Music2,
  Grid3x3,
  Scissors,
  Eraser,
  MousePointer2,
  Volume2,
  Sparkles,
  Settings,
  Shuffle,
  Guitar,
  Sliders,
  ZoomIn,
  ZoomOut,
} from 'lucide-react';
import { Button } from './ui/button';
import {
  Note,
  PianoRollState,
  isNoteInScale,
  getNoteName,
  snapToGrid,
  humanizeNotes,
  strumNotes,
} from '@/types/piano-roll';

interface PianoRollProps {
  trackId: string;
  trackName: string;
  onClose: () => void;
}

const KEYS_PER_OCTAVE = 12;
const OCTAVES = 10; // C-1 to C9
const TOTAL_KEYS = KEYS_PER_OCTAVE * OCTAVES + 1;
const BEATS_PER_BAR = 4;
const TOTAL_BARS = 16;

export default function PianoRoll({ trackId, trackName, onClose }: PianoRollProps) {
  const [state, setState] = useState<PianoRollState>({
    notes: [],
    selectedNotes: [],
    scale: { root: 0, type: 'minor' }, // C minor
    snapEnabled: true,
    snapValue: 0.25, // 1/16 note
    ghostNotesEnabled: false,
    scaleHighlightEnabled: true,
    tool: 'draw',
    zoom: { horizontal: 80, vertical: 16 },
    viewRange: { startBeat: 0, endBeat: 64, lowestNote: 36, highestNote: 84 }, // C2 to C6
  });

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [isDrawing, setIsDrawing] = useState(false);
  const [hoveredNote, setHoveredNote] = useState<number | null>(null);

  // Add a note
  const addNote = useCallback((pitch: number, start: number) => {
    const snappedStart = state.snapEnabled ? snapToGrid(start, state.snapValue) : start;
    const length = state.snapEnabled ? state.snapValue : 1;

    const newNote: Note = {
      id: `${Date.now()}-${Math.random()}`,
      pitch,
      start: snappedStart,
      length,
      velocity: 100,
      selected: false,
      muted: false,
    };

    setState(prev => ({ ...prev, notes: [...prev.notes, newNote] }));
  }, [state.snapEnabled, state.snapValue]);

  // Humanize selected notes
  const handleHumanize = () => {
    setState(prev => ({
      ...prev,
      notes: prev.notes.map(note =>
        prev.selectedNotes.includes(note.id)
          ? humanizeNotes([note], 0.3)[0]
          : note
      ),
    }));
  };

  // Strum selected notes
  const handleStrum = () => {
    const selectedNotes = state.notes.filter(n => state.selectedNotes.includes(n.id));
    if (selectedNotes.length > 1) {
      const strummed = strumNotes(selectedNotes, 'down', 1);
      setState(prev => ({
        ...prev,
        notes: prev.notes.map(note => {
          const strummedNote = strummed.find(s => s.id === note.id);
          return strummedNote || note;
        }),
      }));
    }
  };

  // Select all notes
  const handleSelectAll = () => {
    setState(prev => ({
      ...prev,
      selectedNotes: prev.notes.map(n => n.id),
    }));
  };

  // Delete selected notes
  const handleDelete = useCallback(() => {
    setState(prev => ({
      ...prev,
      notes: prev.notes.filter(n => !prev.selectedNotes.includes(n.id)),
      selectedNotes: [],
    }));
  }, []);

  // Quantize selected notes
  const handleQuantize = useCallback(() => {
    setState(prev => ({
      ...prev,
      notes: prev.notes.map(note =>
        prev.selectedNotes.includes(note.id)
          ? { ...note, start: snapToGrid(note.start, prev.snapValue) }
          : note
      ),
    }));
  }, []);

  // Keyboard shortcuts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Select all (Ctrl+A)
      if ((e.ctrlKey || e.metaKey) && e.key === 'a') {
        e.preventDefault();
        handleSelectAll();
      }
      // Delete (Delete/Backspace)
      if (e.key === 'Delete' || e.key === 'Backspace') {
        e.preventDefault();
        handleDelete();
      }
      // Quantize (Ctrl+Q)
      if ((e.ctrlKey || e.metaKey) && e.key === 'q') {
        e.preventDefault();
        handleQuantize();
      }
      // Tool shortcuts
      if (e.key === 'v') setState(prev => ({ ...prev, tool: 'select' }));
      if (e.key === 'b') setState(prev => ({ ...prev, tool: 'draw' }));
      if (e.key === 'e') setState(prev => ({ ...prev, tool: 'erase' }));
      if (e.key === 's') setState(prev => ({ ...prev, tool: 'slice' }));
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [handleDelete, handleQuantize]);

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: 20 }}
      transition={{ type: 'spring', stiffness: 300, damping: 30 }}
      className="fixed inset-0 z-50 bg-background flex flex-col"
    >
      {/* Header */}
      <div className="h-14 bg-gradient-to-r from-card to-card/80 border-b border-border/50 flex items-center justify-between px-6 backdrop-blur-xl">
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-primary to-primary/70 flex items-center justify-center">
            <Music2 className="h-5 w-5 text-white" />
          </div>
          <div>
            <h2 className="font-semibold text-base">Piano Roll</h2>
            <p className="text-xs text-muted-foreground">{trackName}</p>
          </div>
        </div>

        <div className="flex items-center gap-2">
          <Button size="sm" variant="ghost" onClick={onClose}>
            Close
          </Button>
        </div>
      </div>

      {/* Toolbar */}
      <div className="h-16 bg-card/50 border-b border-border/30 flex items-center px-4 gap-4 backdrop-blur-xl">
        {/* Tools */}
        <div className="flex items-center gap-1">
          <ToolButton
            icon={<MousePointer2 className="h-4 w-4" />}
            active={state.tool === 'select'}
            onClick={() => setState(prev => ({ ...prev, tool: 'select' }))}
            tooltip="Select (V)"
          />
          <ToolButton
            icon={<Grid3x3 className="h-4 w-4" />}
            active={state.tool === 'draw'}
            onClick={() => setState(prev => ({ ...prev, tool: 'draw' }))}
            tooltip="Draw (B)"
          />
          <ToolButton
            icon={<Eraser className="h-4 w-4" />}
            active={state.tool === 'erase'}
            onClick={() => setState(prev => ({ ...prev, tool: 'erase' }))}
            tooltip="Erase (E)"
          />
          <ToolButton
            icon={<Scissors className="h-4 w-4" />}
            active={state.tool === 'slice'}
            onClick={() => setState(prev => ({ ...prev, tool: 'slice' }))}
            tooltip="Slice (S)"
          />
        </div>

        <div className="h-8 w-px bg-border/50" />

        {/* Snap */}
        <div className="flex items-center gap-2">
          <span className="text-xs text-muted-foreground font-medium">Snap:</span>
          <select
            value={state.snapValue}
            onChange={(e) => setState(prev => ({ ...prev, snapValue: parseFloat(e.target.value) }))}
            className="px-2 py-1 bg-secondary/40 border border-border/50 rounded text-sm"
          >
            <option value={1}>1/4</option>
            <option value={0.5}>1/8</option>
            <option value={0.25}>1/16</option>
            <option value={0.125}>1/32</option>
            <option value={0.0625}>1/64</option>
          </select>
          <Button
            size="sm"
            variant={state.snapEnabled ? 'default' : 'ghost'}
            onClick={() => setState(prev => ({ ...prev, snapEnabled: !prev.snapEnabled }))}
          >
            {state.snapEnabled ? 'On' : 'Off'}
          </Button>
        </div>

        <div className="h-8 w-px bg-border/50" />

        {/* Scale */}
        <div className="flex items-center gap-2">
          <span className="text-xs text-muted-foreground font-medium">Scale:</span>
          <select
            value={state.scale.root}
            onChange={(e) => setState(prev => ({
              ...prev,
              scale: { ...prev.scale, root: parseInt(e.target.value) }
            }))}
            className="px-2 py-1 bg-secondary/40 border border-border/50 rounded text-sm"
          >
            {['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'].map((note, i) => (
              <option key={i} value={i}>{note}</option>
            ))}
          </select>
          <select
            value={state.scale.type}
            onChange={(e) => setState(prev => ({
              ...prev,
              scale: { ...prev.scale, type: e.target.value as any }
            }))}
            className="px-2 py-1 bg-secondary/40 border border-border/50 rounded text-sm"
          >
            <option value="major">Major</option>
            <option value="minor">Minor</option>
            <option value="dorian">Dorian</option>
            <option value="phrygian">Phrygian</option>
            <option value="lydian">Lydian</option>
            <option value="mixolydian">Mixolydian</option>
          </select>
          <Button
            size="sm"
            variant={state.scaleHighlightEnabled ? 'default' : 'ghost'}
            onClick={() => setState(prev => ({ ...prev, scaleHighlightEnabled: !prev.scaleHighlightEnabled }))}
            title="Highlight Scale"
          >
            <Music2 className="h-4 w-4" />
          </Button>
        </div>

        <div className="h-8 w-px bg-border/50" />

        {/* Actions */}
        <Button size="sm" onClick={handleHumanize} title="Humanize (H)">
          <Shuffle className="h-4 w-4 mr-1" />
          Humanize
        </Button>
        <Button size="sm" onClick={handleStrum} title="Strum (T)">
          <Guitar className="h-4 w-4 mr-1" />
          Strum
        </Button>
        <Button size="sm" onClick={handleQuantize} title="Quantize (Ctrl+Q)">
          <Grid3x3 className="h-4 w-4 mr-1" />
          Quantize
        </Button>
        <Button size="sm" onClick={handleSelectAll} title="Select All (Ctrl+A)">
          Select All
        </Button>

        <div className="flex-1" />

        {/* Zoom */}
        <div className="flex items-center gap-2">
          <Button
            size="icon"
            variant="ghost"
            onClick={() => setState(prev => ({ ...prev, zoom: { ...prev.zoom, horizontal: Math.max(20, prev.zoom.horizontal - 20) } }))}
          >
            <ZoomOut className="h-4 w-4" />
          </Button>
          <Button
            size="icon"
            variant="ghost"
            onClick={() => setState(prev => ({ ...prev, zoom: { ...prev.zoom, horizontal: Math.min(200, prev.zoom.horizontal + 20) } }))}
          >
            <ZoomIn className="h-4 w-4" />
          </Button>
        </div>
      </div>

      {/* Piano Roll Grid */}
      <div className="flex-1 flex overflow-hidden">
        {/* Piano Keyboard */}
        <div className="w-16 bg-card/30 border-r border-border/50 overflow-y-auto">
          <div className="relative" style={{ height: TOTAL_KEYS * state.zoom.vertical }}>
            {Array.from({ length: TOTAL_KEYS }).map((_, index) => {
              const pitch = TOTAL_KEYS - 1 - index;
              const isBlackKey = [1, 3, 6, 8, 10].includes(pitch % 12);
              const isInScale = state.scaleHighlightEnabled && isNoteInScale(pitch, state.scale);
              const isC = pitch % 12 === 0;

              return (
                <motion.div
                  key={pitch}
                  className={`absolute w-full border-b border-border/20 flex items-center justify-end px-2 text-xs font-mono cursor-pointer transition-colors ${
                    isBlackKey
                      ? 'bg-zinc-800 text-zinc-400'
                      : isInScale
                      ? 'bg-primary/5 text-foreground'
                      : 'bg-card text-muted-foreground'
                  } ${hoveredNote === pitch ? 'bg-primary/20' : ''} ${isC ? 'border-primary/30' : ''}`}
                  style={{
                    top: index * state.zoom.vertical,
                    height: state.zoom.vertical,
                  }}
                  onMouseEnter={() => setHoveredNote(pitch)}
                  onMouseLeave={() => setHoveredNote(null)}
                  whileHover={{ x: 2, backgroundColor: 'rgba(59, 130, 246, 0.15)' }}
                >
                  {isC && getNoteName(pitch)}
                </motion.div>
              );
            })}
          </div>
        </div>

        {/* Grid Area */}
        <div className="flex-1 relative overflow-auto bg-background">
          <PianoRollGrid
            state={state}
            onAddNote={addNote}
            hoveredNote={hoveredNote}
          />
        </div>
      </div>

      {/* Velocity Lane */}
      <div className="h-32 bg-card/20 border-t border-border/50">
        <div className="h-full flex items-center justify-center text-muted-foreground text-sm">
          <div className="text-center">
            <Sliders className="h-6 w-6 mx-auto mb-1 opacity-50" />
            <p>Velocity Lane</p>
          </div>
        </div>
      </div>
    </motion.div>
  );
}

// Tool button component
interface ToolButtonProps {
  icon: React.ReactNode;
  active: boolean;
  onClick: () => void;
  tooltip: string;
}

function ToolButton({ icon, active, onClick, tooltip }: ToolButtonProps) {
  return (
    <motion.button
      onClick={onClick}
      title={tooltip}
      className={`p-2 rounded transition-all ${
        active
          ? 'bg-primary text-primary-foreground'
          : 'bg-secondary/40 hover:bg-secondary/60'
      }`}
      whileHover={{ scale: 1.05 }}
      whileTap={{ scale: 0.95 }}
    >
      {icon}
    </motion.button>
  );
}

// Piano Roll Grid Component
interface PianoRollGridProps {
  state: PianoRollState;
  onAddNote: (pitch: number, start: number) => void;
  hoveredNote: number | null;
}

function PianoRollGrid({ state, onAddNote, hoveredNote }: PianoRollGridProps) {
  const gridRef = useRef<HTMLDivElement>(null);
  const [isAltPressed, setIsAltPressed] = useState(false);

  // Track Alt key for snap override
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.altKey) setIsAltPressed(true);
    };
    const handleKeyUp = (e: KeyboardEvent) => {
      if (!e.altKey) setIsAltPressed(false);
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, []);

  const handleClick = (e: React.MouseEvent<HTMLDivElement>) => {
    if (state.tool !== 'draw') return;

    const rect = gridRef.current?.getBoundingClientRect();
    if (!rect) return;

    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    let beat = x / state.zoom.horizontal;
    const pitch = TOTAL_KEYS - 1 - Math.floor(y / state.zoom.vertical);

    // Override snap if Alt is pressed (FL Studio behavior)
    if (!isAltPressed && state.snapEnabled) {
      beat = snapToGrid(beat, state.snapValue);
    }

    onAddNote(pitch, beat);
  };

  return (
    <div
      ref={gridRef}
      className="relative"
      style={{
        width: TOTAL_BARS * BEATS_PER_BAR * state.zoom.horizontal,
        height: TOTAL_KEYS * state.zoom.vertical,
      }}
      onClick={handleClick}
    >
      {/* Horizontal lines (keys) */}
      {Array.from({ length: TOTAL_KEYS }).map((_, index) => {
        const pitch = TOTAL_KEYS - 1 - index;
        const isBlackKey = [1, 3, 6, 8, 10].includes(pitch % 12);
        const isInScale = state.scaleHighlightEnabled && isNoteInScale(pitch, state.scale);
        const isHighlighted = pitch === hoveredNote;

        return (
          <div
            key={pitch}
            className={`absolute w-full border-b transition-colors ${
              isBlackKey
                ? 'bg-zinc-900/20 border-border/10'
                : isInScale
                ? 'bg-primary/3 border-border/20'
                : 'bg-transparent border-border/10'
            } ${isHighlighted ? 'bg-primary/10' : ''}`}
            style={{
              top: index * state.zoom.vertical,
              height: state.zoom.vertical,
            }}
          />
        );
      })}

      {/* Vertical lines (beats) */}
      {Array.from({ length: TOTAL_BARS * BEATS_PER_BAR + 1 }).map((_, beatIndex) => {
        const isBar = beatIndex % BEATS_PER_BAR === 0;
        return (
          <div
            key={beatIndex}
            className={`absolute h-full ${
              isBar ? 'border-l-2 border-border/40' : 'border-l border-border/10'
            }`}
            style={{
              left: beatIndex * state.zoom.horizontal,
            }}
          />
        );
      })}

      {/* Notes */}
      <AnimatePresence>
        {state.notes.map((note) => (
          <motion.div
            key={note.id}
            initial={{ scale: 0.8, opacity: 0 }}
            animate={{ scale: 1, opacity: note.muted ? 0.3 : 1 }}
            exit={{ scale: 0.8, opacity: 0 }}
            transition={{ type: 'spring', stiffness: 500, damping: 30 }}
            className={`absolute rounded cursor-pointer ${
              state.selectedNotes.includes(note.id)
                ? 'bg-primary border-2 border-primary-foreground'
                : 'bg-primary/80 border border-primary-foreground/20'
            }`}
            style={{
              left: note.start * state.zoom.horizontal,
              top: (TOTAL_KEYS - 1 - note.pitch) * state.zoom.vertical + 1,
              width: note.length * state.zoom.horizontal - 2,
              height: state.zoom.vertical - 2,
            }}
            whileHover={{ scale: 1.05, backgroundColor: 'rgba(59, 130, 246, 1)' }}
          />
        ))}
      </AnimatePresence>
    </div>
  );
}
