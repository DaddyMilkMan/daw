/**
 * SessionView.tsx
 * Ableton-style clip launcher for live performance
 */

import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Play, Square, Circle, Plus } from 'lucide-react';
import { useAudioStore } from '../stores/audioStore';
import { Clip, Scene } from '../types/session';

interface SessionViewProps {
  onOpenPianoRoll?: (trackId: string, trackName: string) => void;
}

export default function SessionView({ onOpenPianoRoll }: SessionViewProps) {
  const { tracks } = useAudioStore();
  const [scenes, setScenes] = useState<Scene[]>([
    { id: 'scene-0', name: 'Intro', index: 0 },
    { id: 'scene-1', name: 'Verse', index: 1 },
    { id: 'scene-2', name: 'Chorus', index: 2 },
    { id: 'scene-3', name: 'Bridge', index: 3 },
    { id: 'scene-4', name: 'Outro', index: 4 },
  ]);

  // Generate clips from tracks (for now, each track has clips in each scene)
  const clips: Clip[] = [];
  tracks.forEach((track, trackIndex) => {
    scenes.forEach((scene, sceneIndex) => {
      // Only add clip if track has MIDI notes
      if ((track.type === 'midi' || track.type === 'instrument') && track.notes && track.notes.length > 0) {
        clips.push({
          id: `clip-${track.id}-${scene.id}`,
          name: `${track.name} ${scene.name}`,
          trackId: track.id,
          sceneIndex: scene.index,
          color: track.color,
          state: 'stopped',
          type: 'midi',
          midiNotes: track.notes,
          length: 8, // 8 beats
          loopEnabled: true,
          startOffset: 0,
        });
      }
    });
  });

  const handleClipTrigger = (clip: Clip) => {
    console.log('🎬 Clip triggered:', clip.name);
    // TODO: Implement clip triggering in audio engine
  };

  const handleSceneTrigger = (scene: Scene) => {
    console.log('🎭 Scene triggered:', scene.name);
    // TODO: Trigger all clips in this scene
  };

  return (
    <div className="h-full flex flex-col bg-background">
      {/* Header */}
      <div className="h-12 border-b border-border/50 flex items-center px-4 bg-card/50 backdrop-blur-xl">
        <h2 className="text-sm font-semibold">Session View</h2>
        <div className="flex-1" />
        <button className="text-xs text-muted-foreground hover:text-foreground transition-colors">
          + Add Scene
        </button>
      </div>

      {/* Clip Grid */}
      <div className="flex-1 overflow-auto">
        <div className="grid gap-2 p-4" style={{
          gridTemplateColumns: `120px repeat(${tracks.length}, 1fr)`,
        }}>
          {/* Header Row */}
          <div></div> {/* Empty corner */}
          {tracks.map((track) => (
            <div
              key={track.id}
              className="text-center p-2 bg-card/50 rounded-t border-b-2"
              style={{ borderColor: track.color }}
            >
              <div className="text-sm font-medium truncate">{track.name}</div>
              <div className="text-xs text-muted-foreground">{track.type.toUpperCase()}</div>
            </div>
          ))}

          {/* Scene Rows */}
          {scenes.map((scene) => (
            <>
              {/* Scene Trigger Button */}
              <motion.button
                key={`scene-btn-${scene.id}`}
                whileHover={{ scale: 1.02 }}
                whileTap={{ scale: 0.98 }}
                onClick={() => handleSceneTrigger(scene)}
                className="p-3 bg-primary/20 hover:bg-primary/30 rounded border border-primary/30 flex items-center gap-2 transition-colors"
              >
                <Play className="h-4 w-4 text-primary" />
                <div className="flex-1 text-left">
                  <div className="text-sm font-medium">{scene.name}</div>
                  <div className="text-xs text-muted-foreground">Scene {scene.index + 1}</div>
                </div>
              </motion.button>

              {/* Clip Slots */}
              {tracks.map((track) => {
                const clip = clips.find(
                  (c) => c.trackId === track.id && c.sceneIndex === scene.index
                );

                return (
                  <ClipSlot
                    key={`clip-${track.id}-${scene.id}`}
                    clip={clip}
                    onTrigger={handleClipTrigger}
                    onDoubleClick={() => {
                      if (clip && onOpenPianoRoll) {
                        onOpenPianoRoll(track.id, track.name);
                      }
                    }}
                  />
                );
              })}
            </>
          ))}
        </div>
      </div>
    </div>
  );
}

interface ClipSlotProps {
  clip?: Clip;
  onTrigger: (clip: Clip) => void;
  onDoubleClick?: () => void;
}

function ClipSlot({ clip, onTrigger, onDoubleClick }: ClipSlotProps) {
  if (!clip) {
    // Empty slot
    return (
      <motion.div
        whileHover={{ scale: 1.02, backgroundColor: 'rgba(255,255,255,0.05)' }}
        className="h-20 bg-card/30 border border-dashed border-border/30 rounded flex items-center justify-center cursor-pointer transition-colors"
      >
        <Plus className="h-6 w-6 text-muted-foreground/30" />
      </motion.div>
    );
  }

  // Clip exists
  const stateColors = {
    empty: 'bg-card/30 border-border/30',
    stopped: 'bg-card/50 border-border',
    playing: 'bg-green-500/20 border-green-500',
    recording: 'bg-red-500/20 border-red-500',
    queued: 'bg-yellow-500/20 border-yellow-500',
  };

  const stateIcons = {
    empty: null,
    stopped: <Square className="h-4 w-4" />,
    playing: <Play className="h-4 w-4 fill-current" />,
    recording: <Circle className="h-4 w-4 fill-current animate-pulse" />,
    queued: <Play className="h-4 w-4 animate-pulse" />,
  };

  return (
    <motion.div
      whileHover={{ scale: 1.05, y: -2 }}
      whileTap={{ scale: 0.95 }}
      onClick={() => onTrigger(clip)}
      onDoubleClick={onDoubleClick}
      className={`h-20 rounded border-2 cursor-pointer transition-all overflow-hidden relative group ${stateColors[clip.state]}`}
      style={{
        borderLeftWidth: '4px',
        borderLeftColor: clip.color,
      }}
    >
      {/* Clip Content */}
      <div className="p-2 h-full flex flex-col">
        <div className="flex items-start justify-between mb-1">
          <div className="text-xs font-medium truncate flex-1">{clip.name}</div>
          {clip.state !== 'empty' && (
            <div className="text-muted-foreground">
              {stateIcons[clip.state]}
            </div>
          )}
        </div>

        {/* Waveform / MIDI visualization placeholder */}
        {clip.type === 'midi' && (
          <div className="flex-1 flex items-end gap-0.5 opacity-50">
            {Array.from({ length: 16 }).map((_, i) => {
              const hasNote = clip.midiNotes?.some(
                (note) => Math.floor(note.startTime / (clip.length / 16)) === i
              );
              const height = hasNote ? Math.random() * 100 + 30 : 10;
              return (
                <motion.div
                  key={i}
                  className="flex-1 bg-current rounded-sm"
                  style={{ height: `${height}%` }}
                  initial={{ scaleY: 0 }}
                  animate={{ scaleY: 1 }}
                  transition={{ delay: i * 0.02 }}
                />
              );
            })}
          </div>
        )}

        {/* Clip info */}
        <div className="text-[10px] text-muted-foreground mt-1">
          {clip.length} beats {clip.loopEnabled && '∞'}
        </div>
      </div>

      {/* Hover overlay */}
      <div className="absolute inset-0 bg-gradient-to-r from-transparent via-white/5 to-transparent translate-x-[-100%] group-hover:translate-x-[100%] transition-transform duration-700" />
    </motion.div>
  );
}
