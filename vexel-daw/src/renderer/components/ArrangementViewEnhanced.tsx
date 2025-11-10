// Enhanced Arrangement View with Audio Clip Editing Features

import { useState, useEffect, useRef, useCallback } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Track } from '../types/audio';
import { useClipStore } from '../store/clipStore';
import { useAudioFileLoader } from '../hooks/useAudioFileLoader';
import { useAudioFileDrop } from '../hooks/useAudioFileDrop';
import { useAudioStore } from '../stores/audioStore';
import { audioEngine } from '../lib/audioEngine';
import AudioClipComponent from './AudioClipComponent';
import ContextMenu, { ContextMenuItem } from './ContextMenu';
import TimelineRuler from './TimelineRuler';
import AutomationLaneComponent, { AutomationLane } from './AutomationLane';
import { createAudioClip, beatsFromSeconds } from '../types/clip';
import { ColorPickerDialog, RenameDialog, DeleteConfirmDialog } from './TrackManagement';
import {
  ChevronDown,
  ChevronRight,
  Circle,
  Magnet,
  MagnetOff,
  Play,
  Scissors,
  Copy,
  Trash2,
  Volume2,
  Lock,
  Unlock,
  Settings,
  Edit3,
  Palette,
  FileAudio,
  Loader2,
  FolderTree,
} from 'lucide-react';
import { Button } from './ui/button';

interface ArrangementViewEnhancedProps {
  tracks: Track[];
  bpm: number;
  onOpenPianoRoll: (trackId: string, trackName: string) => void;
}

const PIXELS_PER_BEAT = 50;
const TRACK_HEIGHT = 80;

export default function ArrangementViewEnhanced({
  tracks,
  bpm,
  onOpenPianoRoll,
}: ArrangementViewEnhancedProps) {
  const {
    clips,
    selectedClipIds,
    gridSnapEnabled,
    playheadPosition,
    addClip,
    selectClip,
    deselectAll,
    copySelectedClips,
    cutSelectedClips,
    pasteClips,
    duplicateSelectedClips,
    deleteSelectedClips,
    splitClipAtBeat,
    setGridSnap,
    setPlayheadPosition,
    updateClip,
  } = useClipStore();

  const { loadAudioFile, isLoading, progress } = useAudioFileLoader();
  const { addTrack, removeTrack, updateTrack } = useAudioStore();

  const [recordArmed, setRecordArmed] = useState<Set<string>>(new Set());
  const [expandedAutomation, setExpandedAutomation] = useState<Set<string>>(new Set());
  const [automationLanes, setAutomationLanes] = useState<Map<string, AutomationLane[]>>(new Map());
  const [contextMenu, setContextMenu] = useState<{
    x: number;
    y: number;
    type: 'track' | 'clip';
    id: string;
  } | null>(null);
  const [showSettings, setShowSettings] = useState(false);

  // Track management dialogs
  const [renameDialog, setRenameDialog] = useState<{ trackId: string; currentName: string } | null>(null);
  const [colorPickerDialog, setColorPickerDialog] = useState<{ trackId: string; currentColor: string; trackName: string } | null>(null);
  const [deleteDialog, setDeleteDialog] = useState<{ trackId: string; trackName: string } | null>(null);

  const timelineRef = useRef<HTMLDivElement>(null);

  // Initialize audio engine on mount
  useEffect(() => {
    const initAudio = async () => {
      if (!audioEngine.isReady()) {
        try {
          await audioEngine.initialize();
          console.log('✅ Audio engine initialized');
        } catch (error) {
          console.error('Failed to initialize audio engine:', error);
        }
      }
    };

    initAudio();
  }, []);

  // Handle audio file drop
  const handleFilesDropped = useCallback(
    async (files: File[], x: number, y: number) => {
      if (!timelineRef.current) return;

      // Determine which track based on Y position
      const trackIndex = Math.floor(y / TRACK_HEIGHT);
      if (trackIndex < 0 || trackIndex >= tracks.length) return;

      const track = tracks[trackIndex];

      // Calculate beat position from X
      const beat = x / PIXELS_PER_BEAT;

      // Load each file
      for (const file of files) {
        const result = await loadAudioFile(file);
        if (!result) continue;

        const clip = createAudioClip(track.id, beat, {
          path: file.name,
          name: result.name,
          buffer: result.buffer,
          duration: result.duration,
          sampleRate: result.sampleRate,
          numberOfChannels: result.numberOfChannels,
          peaksData: result.peaksData,
        });

        addClip(clip);
      }
    },
    [tracks, loadAudioFile, addClip]
  );

  const { state: dropState, handlers: dropHandlers } = useAudioFileDrop(handleFilesDropped);

  // Keyboard shortcuts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Prevent if typing in an input
      if ((e.target as HTMLElement).tagName === 'INPUT') return;

      const isCtrlOrCmd = e.ctrlKey || e.metaKey;

      // S - Split clip at playhead
      if (e.key === 's' || e.key === 'S') {
        e.preventDefault();
        if (selectedClipIds.length > 0) {
          selectedClipIds.forEach((clipId) => {
            splitClipAtBeat(clipId, playheadPosition);
          });
        }
      }

      // Ctrl+C - Copy
      if (isCtrlOrCmd && e.key === 'c') {
        e.preventDefault();
        copySelectedClips();
      }

      // Ctrl+X - Cut
      if (isCtrlOrCmd && e.key === 'x') {
        e.preventDefault();
        cutSelectedClips();
      }

      // Ctrl+V - Paste
      if (isCtrlOrCmd && e.key === 'v') {
        e.preventDefault();
        if (tracks.length > 0) {
          pasteClips(tracks[0].id, playheadPosition);
        }
      }

      // Ctrl+D - Duplicate
      if (isCtrlOrCmd && e.key === 'd') {
        e.preventDefault();
        duplicateSelectedClips();
      }

      // Delete or Backspace - Delete
      if (e.key === 'Delete' || e.key === 'Backspace') {
        e.preventDefault();
        deleteSelectedClips();
      }

      // Escape - Deselect all
      if (e.key === 'Escape') {
        deselectAll();
        setContextMenu(null);
      }

      // Spacebar - Play/Pause (implement later with transport)
      if (e.key === ' ' || e.key === 'Spacebar') {
        e.preventDefault();
        // TODO: Toggle playback
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [
    selectedClipIds,
    playheadPosition,
    tracks,
    copySelectedClips,
    cutSelectedClips,
    pasteClips,
    duplicateSelectedClips,
    deleteSelectedClips,
    splitClipAtBeat,
    deselectAll,
  ]);

  // Toggle functions
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

  // Context menu handlers
  const handleTrackContextMenu = (e: React.MouseEvent, trackId: string) => {
    e.preventDefault();
    setContextMenu({ x: e.clientX, y: e.clientY, type: 'track', id: trackId });
  };

  const handleClipContextMenu = (e: React.MouseEvent, clipId: string) => {
    e.preventDefault();
    setContextMenu({ x: e.clientX, y: e.clientY, type: 'clip', id: clipId });
  };

  const getClipContextMenuItems = (clipId: string): ContextMenuItem[] => {
    const clip = clips.find((c) => c.id === clipId);
    if (!clip) return [];

    return [
      {
        label: 'Split at Playhead',
        icon: <Scissors className="h-4 w-4" />,
        shortcut: 'S',
        onClick: () => splitClipAtBeat(clipId, playheadPosition),
      },
      {
        label: 'Duplicate',
        icon: <Copy className="h-4 w-4" />,
        shortcut: 'Ctrl+D',
        onClick: () => {
          selectClip(clipId);
          duplicateSelectedClips();
        },
      },
      { divider: true, label: '', onClick: () => {} },
      {
        label: clip.isMuted ? 'Unmute' : 'Mute',
        icon: <Volume2 className="h-4 w-4" />,
        shortcut: 'M',
        onClick: () => updateClip(clipId, { isMuted: !clip.isMuted }),
      },
      {
        label: clip.isLocked ? 'Unlock' : 'Lock',
        icon: clip.isLocked ? <Unlock className="h-4 w-4" /> : <Lock className="h-4 w-4" />,
        shortcut: 'L',
        onClick: () => updateClip(clipId, { isLocked: !clip.isLocked }),
      },
      {
        label: 'Change Color',
        icon: <Palette className="h-4 w-4" />,
        onClick: () => {
          // TODO: Implement color picker
          console.log('Change color for clip:', clipId);
        },
      },
      { divider: true, label: '', onClick: () => {} },
      {
        label: 'Delete',
        icon: <Trash2 className="h-4 w-4" />,
        shortcut: 'Del',
        danger: true,
        onClick: () => {
          selectClip(clipId);
          deleteSelectedClips();
        },
      },
    ];
  };

  // Track context menu items
  const getTrackContextMenuItems = (trackId: string): ContextMenuItem[] => {
    const track = tracks.find((t) => t.id === trackId);
    if (!track) return [];

    return [
      {
        label: 'Rename Track',
        icon: <Edit3 className="h-4 w-4" />,
        shortcut: 'F2',
        onClick: () => {
          setContextMenu(null);
          setRenameDialog({ trackId, currentName: track.name });
        },
      },
      {
        label: 'Duplicate Track',
        icon: <Copy className="h-4 w-4" />,
        shortcut: 'Ctrl+D',
        onClick: () => {
          // Duplicate track with all its properties
          const newTrackId = addTrack(track.type, `${track.name} (Copy)`);
          updateTrack(newTrackId, {
            volume: track.volume,
            pan: track.pan,
            color: track.color,
            muted: false,
            solo: false,
          });
          // Copy clips (if any)
          const trackClips = clips.filter((c) => c.trackId === trackId);
          trackClips.forEach((clip) => {
            const newClip = { ...clip, id: `${clip.id}-copy`, trackId: newTrackId };
            addClip(newClip);
          });
        },
      },
      {
        label: 'Change Color',
        icon: <Palette className="h-4 w-4" />,
        onClick: () => {
          setContextMenu(null);
          setColorPickerDialog({
            trackId,
            currentColor: track.color || '#3b82f6',
            trackName: track.name,
          });
        },
      },
      {
        label: 'Group Tracks',
        icon: <FolderTree className="h-4 w-4" />,
        onClick: () => {
          // TODO: Implement track grouping
          console.log('Group tracks:', trackId);
          // For now, just show a message
          alert('Track grouping will be implemented in a future update');
        },
      },
      { divider: true, label: '', onClick: () => {} },
      {
        label: 'Delete Track',
        icon: <Trash2 className="h-4 w-4" />,
        shortcut: 'Del',
        danger: true,
        onClick: () => {
          setContextMenu(null);
          setDeleteDialog({ trackId, trackName: track.name });
        },
      },
    ];
  };

  // Handle timeline click (set playhead or deselect)
  const handleTimelineClick = (e: React.MouseEvent<HTMLDivElement>) => {
    if (e.target === e.currentTarget || (e.target as HTMLElement).classList.contains('track-lane')) {
      const rect = e.currentTarget.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const beat = x / PIXELS_PER_BEAT;
      setPlayheadPosition(beat);
      deselectAll();
    }
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 10 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: -10 }}
      transition={{ duration: 0.2 }}
      className="h-full flex flex-col relative"
    >
      {/* Toolbar */}
      <div className="h-10 border-b border-border/30 bg-card/30 backdrop-blur-sm flex items-center justify-between px-3 gap-2">
        <div className="flex items-center gap-2">
          {/* Grid Snap Toggle */}
          <motion.div whileTap={{ scale: 0.95 }}>
            <Button
              size="sm"
              variant={gridSnapEnabled ? 'default' : 'ghost'}
              onClick={() => setGridSnap(!gridSnapEnabled)}
              title="Toggle grid snap (Cmd+G)"
            >
              {gridSnapEnabled ? (
                <Magnet className="h-4 w-4 mr-2" />
              ) : (
                <MagnetOff className="h-4 w-4 mr-2" />
              )}
              Snap
            </Button>
          </motion.div>

          {/* Shortcuts info */}
          <div className="text-xs text-muted-foreground ml-4 hidden md:block">
            <span className="font-mono bg-muted px-1 rounded">S</span> Split •{' '}
            <span className="font-mono bg-muted px-1 rounded">Ctrl+C/V/D</span> Copy/Paste/Duplicate •{' '}
            <span className="font-mono bg-muted px-1 rounded">Del</span> Delete
          </div>
        </div>

        {/* Loading indicator */}
        {isLoading && (
          <div className="flex items-center gap-2 text-sm text-muted-foreground">
            <Loader2 className="h-4 w-4 animate-spin" />
            <span>Loading audio... {progress}%</span>
          </div>
        )}
      </div>

      {/* Main Content */}
      <div className="flex-1 flex min-h-0">
        {/* Track List */}
        <div className="w-48 border-r border-border/50 bg-card/30 backdrop-blur-xl overflow-y-auto">
          {tracks.length === 0 ? (
            <motion.div
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              className="h-full flex items-center justify-center text-muted-foreground text-sm p-4 text-center"
            >
              <div>
                <FileAudio className="h-8 w-8 mx-auto mb-2 opacity-50" />
                <p>No tracks yet.</p>
                <p className="text-xs mt-1">Add tracks to start</p>
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
                    className="border-b border-border/30"
                    style={{ height: `${TRACK_HEIGHT}px` }}
                  >
                    <div
                      className="h-full px-3 py-2 cursor-pointer hover:bg-accent/50 transition-colors"
                      onDoubleClick={() => onOpenPianoRoll(track.id, track.name)}
                      onContextMenu={(e) => handleTrackContextMenu(e, track.id)}
                    >
                      <div className="flex items-center justify-between gap-2 h-full">
                        <div className="flex items-center gap-2 flex-1 min-w-0">
                          <motion.button
                            whileHover={{ scale: 1.1 }}
                            whileTap={{ scale: 0.9 }}
                            onClick={(e) => toggleAutomation(track.id, e)}
                            className="text-muted-foreground hover:text-foreground transition-colors"
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

                        <motion.button
                          whileHover={{ scale: 1.1 }}
                          whileTap={{ scale: 0.9 }}
                          onClick={(e) => toggleRecordArm(track.id, e)}
                          className={`p-1.5 rounded-full transition-all ${
                            recordArmed.has(track.id)
                              ? 'bg-red-500 shadow-lg shadow-red-500/50 animate-pulse'
                              : 'bg-muted hover:bg-red-500/20'
                          }`}
                        >
                          <Circle
                            className="h-3 w-3"
                            fill={recordArmed.has(track.id) ? 'currentColor' : 'none'}
                          />
                        </motion.button>
                      </div>
                    </div>
                  </motion.div>
                ))}
              </AnimatePresence>
            </div>
          )}
        </div>

        {/* Timeline Area */}
        <div
          ref={timelineRef}
          className="flex-1 relative overflow-auto"
          {...dropHandlers}
          onClick={handleTimelineClick}
        >
          {/* Drag overlay */}
          {dropState.isOver && (
            <motion.div
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              exit={{ opacity: 0 }}
              className="absolute inset-0 bg-primary/10 border-2 border-dashed border-primary z-50 pointer-events-none flex items-center justify-center"
            >
              <div className="bg-card/90 backdrop-blur-sm px-6 py-4 rounded-lg shadow-lg">
                <FileAudio className="h-12 w-12 mx-auto mb-2 text-primary" />
                <p className="text-lg font-semibold">Drop audio files here</p>
                <p className="text-sm text-muted-foreground">Supports MP3, WAV, OGG, FLAC, and more</p>
              </div>
            </motion.div>
          )}

          {/* Timeline Ruler */}
          <div className="sticky top-0 z-20 bg-card/50 backdrop-blur-sm">
            <TimelineRuler bars={32} beatsPerBar={4} tempo={bpm} pixelsPerBeat={PIXELS_PER_BEAT} />
          </div>

          {/* Track Lanes */}
          <div className="relative">
            {tracks.map((track, index) => {
              const trackClips = clips.filter((c) => c.trackId === track.id);

              return (
                <div
                  key={track.id}
                  className="border-b border-border/10 relative track-lane"
                  style={{ height: `${TRACK_HEIGHT}px` }}
                >
                  {/* Grid lines */}
                  {Array.from({ length: 32 * 4 }, (_, i) => (
                    <div
                      key={i}
                      className="absolute top-0 bottom-0 w-px bg-border/10"
                      style={{ left: `${i * PIXELS_PER_BEAT}px` }}
                    />
                  ))}

                  {/* Clips */}
                  <AnimatePresence>
                    {trackClips.map((clip) => (
                      <AudioClipComponent
                        key={clip.id}
                        clip={clip}
                        pixelsPerBeat={PIXELS_PER_BEAT}
                        trackHeight={TRACK_HEIGHT}
                        bpm={bpm}
                        onContextMenu={handleClipContextMenu}
                      />
                    ))}
                  </AnimatePresence>
                </div>
              );
            })}

            {/* Playhead */}
            <motion.div
              className="absolute top-0 bottom-0 w-0.5 bg-primary pointer-events-none z-30"
              style={{ left: `${playheadPosition * PIXELS_PER_BEAT}px` }}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
            >
              <div className="absolute top-0 left-1/2 -translate-x-1/2 w-3 h-3 bg-primary rounded-full" />
            </motion.div>
          </div>

          {/* Empty state */}
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

      {/* Context Menu */}
      <AnimatePresence>
        {contextMenu && (
          <ContextMenu
            x={contextMenu.x}
            y={contextMenu.y}
            items={
              contextMenu.type === 'clip'
                ? getClipContextMenuItems(contextMenu.id)
                : getTrackContextMenuItems(contextMenu.id)
            }
            onClose={() => setContextMenu(null)}
          />
        )}
      </AnimatePresence>

      {/* Track Management Dialogs */}
      <RenameDialog
        isOpen={!!renameDialog}
        currentName={renameDialog?.currentName || ''}
        onClose={() => setRenameDialog(null)}
        onRename={(newName) => {
          if (renameDialog) {
            updateTrack(renameDialog.trackId, { name: newName });
          }
        }}
      />

      <ColorPickerDialog
        isOpen={!!colorPickerDialog}
        currentColor={colorPickerDialog?.currentColor || '#3b82f6'}
        trackName={colorPickerDialog?.trackName || ''}
        onClose={() => setColorPickerDialog(null)}
        onColorChange={(color) => {
          if (colorPickerDialog) {
            updateTrack(colorPickerDialog.trackId, { color });
          }
        }}
      />

      <DeleteConfirmDialog
        isOpen={!!deleteDialog}
        trackName={deleteDialog?.trackName || ''}
        onClose={() => setDeleteDialog(null)}
        onConfirm={() => {
          if (deleteDialog) {
            removeTrack(deleteDialog.trackId);
          }
        }}
      />
    </motion.div>
  );
}
