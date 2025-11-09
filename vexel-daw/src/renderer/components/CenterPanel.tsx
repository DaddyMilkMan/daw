import { Track, AudioState } from '@/types/audio';
import { Plus, Grid3X3, List, Copy, Trash2, Edit3, Palette, FolderTree, Circle, ChevronDown, ChevronRight } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { Button } from './ui/button';
import { useState } from 'react';
import ContextMenu, { ContextMenuItem } from './ContextMenu';
import TimelineRuler from './TimelineRuler';
import AutomationLaneComponent, { AutomationLane, AutomationMode } from './AutomationLane';

interface CenterPanelProps {
  tracks: Track[];
  onOpenPianoRoll: (trackId: string, trackName: string) => void;
  audioState: AudioState;
}

export default function CenterPanel({ tracks, onOpenPianoRoll, audioState }: CenterPanelProps) {
  const [view, setView] = useState<'session' | 'arrangement'>('arrangement');

  const handleCreateTrack = () => {
    const name = `Track ${tracks.length + 1}`;
    window.electron.createTrack(name, 'midi');
  };

  return (
    <div className="flex-1 bg-gradient-to-br from-background to-background/95 flex flex-col min-w-0">
      {/* Header */}
      <div className="h-12 border-b border-border/50 flex items-center px-4 justify-between bg-card/50 backdrop-blur-xl">
        <div className="flex items-center gap-2">
          <motion.div whileTap={{ scale: 0.95 }}>
            <Button
              size="sm"
              variant={view === 'session' ? 'default' : 'ghost'}
              onClick={() => setView('session')}
              className="transition-all"
            >
              <Grid3X3 className="h-4 w-4 mr-2" />
              Session
            </Button>
          </motion.div>
          <motion.div whileTap={{ scale: 0.95 }}>
            <Button
              size="sm"
              variant={view === 'arrangement' ? 'default' : 'ghost'}
              onClick={() => setView('arrangement')}
              className="transition-all"
            >
              <List className="h-4 w-4 mr-2" />
              Arrangement
            </Button>
          </motion.div>
        </div>

        <motion.div whileHover={{ scale: 1.05 }} whileTap={{ scale: 0.95 }}>
          <Button
            size="sm"
            onClick={handleCreateTrack}
            className="bg-gradient-to-r from-secondary to-secondary/80 hover:from-secondary/90 hover:to-secondary/70"
          >
            <Plus className="h-4 w-4 mr-2" />
            Add Track
          </Button>
        </motion.div>
      </div>

      {/* View Content */}
      <div className="flex-1 overflow-auto">
        <AnimatePresence mode="wait">
          {view === 'arrangement' ? (
            <ArrangementView key="arrangement" tracks={tracks} onOpenPianoRoll={onOpenPianoRoll} audioState={audioState} />
          ) : (
            <SessionView key="session" tracks={tracks} />
          )}
        </AnimatePresence>
      </div>
    </div>
  );
}

function ArrangementView({ tracks, onOpenPianoRoll, audioState }: { tracks: Track[]; onOpenPianoRoll: (trackId: string, trackName: string) => void; audioState: AudioState }) {
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; trackId: string } | null>(null);
  const [recordArmed, setRecordArmed] = useState<Set<string>>(new Set());
  const [expandedAutomation, setExpandedAutomation] = useState<Set<string>>(new Set());
  const [automationLanes, setAutomationLanes] = useState<Map<string, AutomationLane[]>>(new Map());

  const handleTrackContextMenu = (e: React.MouseEvent, trackId: string) => {
    e.preventDefault();
    setContextMenu({ x: e.clientX, y: e.clientY, trackId });
  };

  const toggleRecordArm = (trackId: string, e: React.MouseEvent) => {
    e.stopPropagation();
    setRecordArmed((prev) => {
      const newSet = new Set(prev);
      if (newSet.has(trackId)) {
        newSet.delete(trackId);
      } else {
        newSet.add(trackId);
      }
      return newSet;
    });
  };

  const toggleAutomation = (trackId: string, e: React.MouseEvent) => {
    e.stopPropagation();
    setExpandedAutomation((prev) => {
      const newSet = new Set(prev);
      if (newSet.has(trackId)) {
        newSet.delete(trackId);
      } else {
        newSet.add(trackId);
        // Initialize default automation lanes if not exists
        if (!automationLanes.has(trackId)) {
          const defaultLanes: AutomationLane[] = [
            {
              id: `${trackId}-volume`,
              trackId,
              parameter: 'volume',
              parameterName: 'Volume',
              points: [
                { time: 0, value: 0.8 },
                { time: 16, value: 0.5 },
                { time: 32, value: 0.8 },
              ],
              mode: 'read',
              color: '#3b82f6',
            },
          ];
          setAutomationLanes((prev) => new Map(prev).set(trackId, defaultLanes));
        }
      }
      return newSet;
    });
  };

  const getTrackContextMenuItems = (trackId: string): ContextMenuItem[] => {
    const track = tracks.find(t => t.id === trackId);
    if (!track) return [];

    return [
      {
        label: 'Rename Track',
        icon: <Edit3 className="h-4 w-4" />,
        shortcut: 'F2',
        onClick: () => {
          console.log('Rename track:', trackId);
          // TODO: Implement rename
        },
      },
      {
        label: 'Duplicate Track',
        icon: <Copy className="h-4 w-4" />,
        shortcut: 'Ctrl+D',
        onClick: () => {
          console.log('Duplicate track:', trackId);
          // TODO: Implement duplicate
        },
      },
      {
        label: 'Change Color',
        icon: <Palette className="h-4 w-4" />,
        onClick: () => {
          console.log('Change color:', trackId);
          // TODO: Implement color picker
        },
      },
      {
        label: 'Group Tracks',
        icon: <FolderTree className="h-4 w-4" />,
        onClick: () => {
          console.log('Group tracks:', trackId);
          // TODO: Implement grouping
        },
      },
      { divider: true, label: '', onClick: () => {} },
      {
        label: 'Delete Track',
        icon: <Trash2 className="h-4 w-4" />,
        shortcut: 'Del',
        danger: true,
        onClick: () => {
          console.log('Delete track:', trackId);
          // TODO: Implement delete
        },
      },
    ];
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 10 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: -10 }}
      transition={{ duration: 0.2 }}
      className="h-full flex flex-col"
    >
      {/* Timeline Ruler */}
      <div className="flex">
        <div className="w-48" /> {/* Spacer for track list */}
        <div className="flex-1">
          <TimelineRuler
            bars={32}
            beatsPerBar={audioState.timeSignature.numerator}
            tempo={audioState.tempo}
            pixelsPerBeat={50}
            timeSignature={audioState.timeSignature}
          />
        </div>
      </div>

      {/* Main Content */}
      <div className="flex-1 flex min-h-0">
        {/* Track List */}
        <div className="w-48 border-r border-border/50 bg-card/30 backdrop-blur-xl">
        {tracks.length === 0 ? (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            className="h-full flex items-center justify-center text-muted-foreground text-sm p-4 text-center"
          >
            <div>
              <motion.div
                animate={{ y: [0, -5, 0] }}
                transition={{ repeat: Infinity, duration: 2 }}
              >
                <Plus className="h-8 w-8 mx-auto mb-2 opacity-50" />
              </motion.div>
              <p>No tracks yet.</p>
              <p className="text-xs mt-1">Click "Add Track" to start</p>
            </div>
          </motion.div>
        ) : (
          <div>
            <AnimatePresence>
              {tracks.map((track, index) => (
                <motion.div
                  key={track.id}
                  initial={{ opacity: 0, x: -20 }}
                  animate={{ opacity: 1, x: 0 }}
                  exit={{ opacity: 0, x: -20 }}
                  transition={{
                    type: 'spring',
                    stiffness: 300,
                    damping: 25,
                    delay: index * 0.05,
                  }}
                  whileHover={{ x: 4, backgroundColor: 'rgba(255,255,255,0.05)' }}
                  className="border-b border-border/30"
                >
                  {/* Main Track Row */}
                  <div
                    className="h-16 px-3 py-2 cursor-pointer"
                    onDoubleClick={() => onOpenPianoRoll(track.id, track.name)}
                    onContextMenu={(e) => handleTrackContextMenu(e, track.id)}
                    title="Double-click to open Piano Roll • Right-click for options"
                  >
                    <div className="flex items-center justify-between gap-2 h-full">
                      <div className="flex items-center gap-2 flex-1 min-w-0">
                        {/* Automation toggle */}
                        <motion.button
                          whileHover={{ scale: 1.1 }}
                          whileTap={{ scale: 0.9 }}
                          onClick={(e) => toggleAutomation(track.id, e)}
                          className="text-muted-foreground hover:text-foreground transition-colors"
                          title="Toggle automation lanes (A)"
                        >
                          {expandedAutomation.has(track.id) ? (
                            <ChevronDown className="h-3 w-3" />
                          ) : (
                            <ChevronRight className="h-3 w-3" />
                          )}
                        </motion.button>

                        <div className="flex-1 min-w-0">
                          <div className="text-sm font-medium truncate">{track.name}</div>
                          <div className="text-xs text-muted-foreground">
                            {track.type.toUpperCase()}
                          </div>
                        </div>
                      </div>

                      {/* Track controls */}
                      <div className="flex items-center gap-1">
                        {/* Record arm */}
                        <motion.button
                          whileHover={{ scale: 1.1 }}
                          whileTap={{ scale: 0.9 }}
                          onClick={(e) => toggleRecordArm(track.id, e)}
                          className={`p-1.5 rounded-full transition-all ${
                            recordArmed.has(track.id)
                              ? 'bg-red-500 shadow-lg shadow-red-500/50 animate-pulse'
                              : 'bg-muted hover:bg-red-500/20'
                          }`}
                          title={recordArmed.has(track.id) ? 'Disarm track' : 'Arm track for recording'}
                        >
                          <Circle
                            className="h-3 w-3"
                            fill={recordArmed.has(track.id) ? 'currentColor' : 'none'}
                            stroke="currentColor"
                          />
                        </motion.button>
                      </div>
                    </div>
                  </div>

                  {/* Automation Lanes */}
                  <AnimatePresence>
                    {expandedAutomation.has(track.id) && automationLanes.get(track.id) && (
                      <motion.div
                        initial={{ height: 0, opacity: 0 }}
                        animate={{ height: 'auto', opacity: 1 }}
                        exit={{ height: 0, opacity: 0 }}
                        transition={{ duration: 0.2 }}
                        className="overflow-hidden"
                      >
                        {automationLanes.get(track.id)!.map((lane) => (
                          <AutomationLaneComponent
                            key={lane.id}
                            lane={lane}
                            width={1600} // 32 bars × 50px per beat
                            height={80}
                            onPointAdd={(time, value) => {
                              console.log('Add automation point:', time, value);
                            }}
                            onPointMove={(index, time, value) => {
                              console.log('Move automation point:', index, time, value);
                            }}
                            onPointDelete={(index) => {
                              console.log('Delete automation point:', index);
                            }}
                            onModeChange={(mode) => {
                              setAutomationLanes((prev) => {
                                const newMap = new Map(prev);
                                const lanes = newMap.get(track.id) || [];
                                const updatedLanes = lanes.map((l) =>
                                  l.id === lane.id ? { ...l, mode } : l
                                );
                                newMap.set(track.id, updatedLanes);
                                return newMap;
                              });
                            }}
                          />
                        ))}
                      </motion.div>
                    )}
                  </AnimatePresence>
                </motion.div>
              ))}
            </AnimatePresence>
          </div>
        )}
      </div>

      {/* Timeline */}
      <div className="flex-1 relative overflow-x-auto">
        {/* Ruler */}
        <div className="h-8 bg-card/50 border-b border-border/30 flex items-center backdrop-blur-xl sticky top-0 z-10">
          {Array.from({ length: 16 }).map((_, i) => (
            <motion.div
              key={i}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ delay: i * 0.02 }}
              className="flex-shrink-0 w-32 border-r border-border/20 px-2 text-xs text-muted-foreground font-mono"
            >
              {i + 1}
            </motion.div>
          ))}
        </div>

        {/* Track Lanes */}
        <div>
          <AnimatePresence>
            {tracks.map((track, index) => (
              <motion.div
                key={track.id}
                initial={{ opacity: 0, height: 0 }}
                animate={{ opacity: 1, height: 64 }}
                exit={{ opacity: 0, height: 0 }}
                transition={{
                  type: 'spring',
                  stiffness: 300,
                  damping: 25,
                  delay: index * 0.05,
                }}
                className="border-b border-border/20 flex items-center relative overflow-hidden group"
              >
                {Array.from({ length: 16 }).map((_, i) => (
                  <motion.div
                    key={i}
                    className="flex-shrink-0 w-32 h-full border-r border-border/10 hover:bg-primary/5 transition-colors cursor-crosshair"
                    whileHover={{ backgroundColor: 'rgba(59, 130, 246, 0.1)' }}
                  />
                ))}
              </motion.div>
            ))}
          </AnimatePresence>
        </div>

        {tracks.length === 0 && (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            className="absolute inset-0 flex items-center justify-center text-muted-foreground"
          >
            <div className="text-center">
              <p className="text-lg font-semibold mb-2">Arrangement View</p>
              <p className="text-sm">Add tracks to start arranging</p>
            </div>
          </motion.div>
        )}
      </div>
      </div>

      {/* Track Context Menu */}
      <AnimatePresence>
        {contextMenu && (
          <ContextMenu
            x={contextMenu.x}
            y={contextMenu.y}
            items={getTrackContextMenuItems(contextMenu.trackId)}
            onClose={() => setContextMenu(null)}
          />
        )}
      </AnimatePresence>
    </motion.div>
  );
}

function SessionView({ tracks }: { tracks: Track[] }) {
  const [clips, setClips] = useState<Array<{id: string; sceneIndex: number; trackId: string; color: string; isPlaying: boolean}>>([]);
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; clipId: string } | null>(null);
  const SCENES = 8;
  const CLIP_COLORS = ['#ef4444', '#f97316', '#eab308', '#22c55e', '#3b82f6', '#8b5cf6', '#ec4899', '#14b8a6'];

  const getClip = (trackId: string, sceneIndex: number) => {
    return clips.find(c => c.trackId === trackId && c.sceneIndex === sceneIndex);
  };

  const handleClipClick = (trackId: string, sceneIndex: number) => {
    const existingClip = getClip(trackId, sceneIndex);

    if (existingClip) {
      // Toggle playback
      setClips(clips.map(c =>
        c.id === existingClip.id
          ? { ...c, isPlaying: !c.isPlaying }
          : c
      ));
    } else {
      // Create new clip
      const newClip = {
        id: `clip-${Date.now()}`,
        trackId,
        sceneIndex,
        color: CLIP_COLORS[Math.floor(Math.random() * CLIP_COLORS.length)],
        isPlaying: false,
      };
      setClips([...clips, newClip]);
    }
  };

  const handleSceneLaunch = (sceneIndex: number) => {
    // Launch all clips in this scene
    setClips(clips.map(c => ({
      ...c,
      isPlaying: c.sceneIndex === sceneIndex,
    })));
  };

  const handleClipContextMenu = (e: React.MouseEvent, clipId: string) => {
    e.preventDefault();
    e.stopPropagation();
    setContextMenu({ x: e.clientX, y: e.clientY, clipId });
  };

  const getClipContextMenuItems = (clipId: string): ContextMenuItem[] => {
    return [
      {
        label: 'Duplicate Clip',
        icon: <Copy className="h-4 w-4" />,
        shortcut: 'Ctrl+D',
        onClick: () => {
          console.log('Duplicate clip:', clipId);
          // TODO: Implement duplicate
        },
      },
      {
        label: 'Rename Clip',
        icon: <Edit3 className="h-4 w-4" />,
        shortcut: 'F2',
        onClick: () => {
          console.log('Rename clip:', clipId);
          // TODO: Implement rename
        },
      },
      {
        label: 'Change Color',
        icon: <Palette className="h-4 w-4" />,
        onClick: () => {
          console.log('Change color:', clipId);
          // TODO: Implement color picker
        },
      },
      { divider: true, label: '', onClick: () => {} },
      {
        label: 'Delete Clip',
        icon: <Trash2 className="h-4 w-4" />,
        shortcut: 'Del',
        danger: true,
        onClick: () => {
          setClips(clips.filter(c => c.id !== clipId));
          console.log('Deleted clip:', clipId);
        },
      },
    ];
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 10 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: -10 }}
      transition={{ duration: 0.2 }}
      className="h-full flex flex-col p-4 gap-4"
    >
      {tracks.length === 0 ? (
        <div className="flex-1 flex items-center justify-center text-muted-foreground">
          <div className="text-center">
            <motion.div animate={{ y: [0, -5, 0] }} transition={{ repeat: Infinity, duration: 2 }}>
              <Plus className="h-12 w-12 mx-auto mb-2 opacity-50" />
            </motion.div>
            <p className="text-lg font-semibold mb-2">Session View</p>
            <p className="text-sm">Add tracks to start launching clips</p>
          </div>
        </div>
      ) : (
        <div className="flex gap-3 overflow-auto">
          {/* Track columns */}
          {tracks.map((track, trackIndex) => (
            <div key={track.id} className="flex flex-col gap-2" style={{ minWidth: '120px' }}>
              {/* Track header */}
              <div className="h-12 rounded-lg bg-card/50 border border-border/30 px-3 flex flex-col justify-center">
                <div className="text-sm font-medium truncate">{track.name}</div>
                <div className="text-xs text-muted-foreground">{track.type}</div>
              </div>

              {/* Clip slots */}
              {Array.from({ length: SCENES }).map((_, sceneIndex) => {
                const clip = getClip(track.id, sceneIndex);

                return (
                  <motion.button
                    key={sceneIndex}
                    onClick={() => handleClipClick(track.id, sceneIndex)}
                    onContextMenu={(e) => clip && handleClipContextMenu(e, clip.id)}
                    className={`h-16 rounded-lg transition-all relative overflow-hidden ${
                      clip
                        ? 'border-2'
                        : 'border border-dashed border-border/30 hover:border-primary/50 hover:bg-primary/5'
                    }`}
                    style={{
                      backgroundColor: clip ? `${clip.color}20` : 'transparent',
                      borderColor: clip ? clip.color : undefined,
                    }}
                    whileHover={{ scale: 1.02 }}
                    whileTap={{ scale: 0.98 }}
                  >
                    {clip && (
                      <>
                        {/* Playing indicator */}
                        {clip.isPlaying && (
                          <motion.div
                            className="absolute inset-0"
                            style={{ backgroundColor: clip.color }}
                            animate={{ opacity: [0.1, 0.3, 0.1] }}
                            transition={{ repeat: Infinity, duration: 0.8 }}
                          />
                        )}

                        {/* Clip content */}
                        <div className="relative z-10 h-full flex flex-col items-center justify-center px-2">
                          <div className="text-xs font-medium truncate w-full text-center">
                            Clip {sceneIndex + 1}
                          </div>
                          {clip.isPlaying && (
                            <div className="text-[10px] text-muted-foreground">Playing</div>
                          )}
                        </div>
                      </>
                    )}

                    {!clip && (
                      <div className="flex items-center justify-center h-full text-muted-foreground/30">
                        <Plus className="h-4 w-4" />
                      </div>
                    )}
                  </motion.button>
                );
              })}
            </div>
          ))}

          {/* Scene launch column */}
          <div className="flex flex-col gap-2" style={{ minWidth: '60px' }}>
            {/* Header */}
            <div className="h-12 rounded-lg bg-card/50 border border-border/30 px-2 flex items-center justify-center">
              <span className="text-xs font-medium">Scenes</span>
            </div>

            {/* Scene buttons */}
            {Array.from({ length: SCENES }).map((_, sceneIndex) => (
              <motion.button
                key={sceneIndex}
                onClick={() => handleSceneLaunch(sceneIndex)}
                className="h-16 rounded-lg bg-gradient-to-r from-primary/20 to-primary/10 border border-primary/30 hover:border-primary/60 hover:from-primary/30 hover:to-primary/20 transition-all"
                whileHover={{ scale: 1.05 }}
                whileTap={{ scale: 0.95 }}
                title={`Launch Scene ${sceneIndex + 1}`}
              >
                <div className="text-sm font-semibold">{sceneIndex + 1}</div>
              </motion.button>
            ))}
          </div>

          {/* Clip Context Menu */}
          <AnimatePresence>
            {contextMenu && (
              <ContextMenu
                x={contextMenu.x}
                y={contextMenu.y}
                items={getClipContextMenuItems(contextMenu.clipId)}
                onClose={() => setContextMenu(null)}
              />
            )}
          </AnimatePresence>
        </div>
      )}
    </motion.div>
  );
}
