// Audio Clip Component with Trim, Fade, and Drag handles

import { useState, useRef, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { AudioClip } from '../types/clip';
import { useClipStore } from '../store/clipStore';
import WaveformCanvas from './WaveformCanvas';
import {
  Scissors,
  Copy,
  Trash2,
  Volume2,
  Music,
  Lock,
  Unlock,
  Play,
  TrendingUp,
} from 'lucide-react';

interface AudioClipComponentProps {
  clip: AudioClip;
  pixelsPerBeat: number;
  trackHeight: number;
  bpm: number;
  onContextMenu?: (e: React.MouseEvent, clipId: string) => void;
}

type DragMode =
  | 'none'
  | 'move'
  | 'trimStart'
  | 'trimEnd'
  | 'fadeIn'
  | 'fadeOut';

export default function AudioClipComponent({
  clip,
  pixelsPerBeat,
  trackHeight,
  bpm,
  onContextMenu,
}: AudioClipComponentProps) {
  const {
    updateClip,
    selectClip,
    moveClip,
    trimClip,
    setClipFade,
  } = useClipStore();

  const [dragMode, setDragMode] = useState<DragMode>('none');
  const [dragStart, setDragStart] = useState({ x: 0, y: 0 });
  const [initialValue, setInitialValue] = useState(0);
  const [isHovered, setIsHovered] = useState(false);

  const clipRef = useRef<HTMLDivElement>(null);

  // Calculate clip dimensions
  const clipDuration = clip.endBeat - clip.startBeat;
  const clipWidth = clipDuration * pixelsPerBeat;
  const clipLeft = clip.startBeat * pixelsPerBeat;

  // Calculate fade handle positions (in pixels)
  const fadeInWidth = clip.fadeIn ? (clip.fadeIn * bpm * pixelsPerBeat) / 60 : 0;
  const fadeOutWidth = clip.fadeOut ? (clip.fadeOut * bpm * pixelsPerBeat) / 60 : 0;

  // Handle clip selection
  const handleClick = (e: React.MouseEvent) => {
    e.stopPropagation();
    const addToSelection = e.shiftKey || e.ctrlKey || e.metaKey;
    selectClip(clip.id, addToSelection);
  };

  // Handle drag start
  const handleMouseDown = (e: React.MouseEvent, mode: DragMode) => {
    if (clip.isLocked) return;

    e.stopPropagation();
    setDragMode(mode);
    setDragStart({ x: e.clientX, y: e.clientY });

    switch (mode) {
      case 'trimStart':
        setInitialValue(clip.trimStart);
        break;
      case 'trimEnd':
        setInitialValue(clip.trimEnd);
        break;
      case 'fadeIn':
        setInitialValue(clip.fadeIn);
        break;
      case 'fadeOut':
        setInitialValue(clip.fadeOut);
        break;
      case 'move':
        setInitialValue(clip.startBeat);
        break;
    }
  };

  // Handle mouse move
  useEffect(() => {
    if (dragMode === 'none') return;

    const handleMouseMove = (e: MouseEvent) => {
      const deltaX = e.clientX - dragStart.x;
      const deltaBeats = deltaX / pixelsPerBeat;

      switch (dragMode) {
        case 'move': {
          const newStartBeat = initialValue + deltaBeats;
          moveClip(clip.id, newStartBeat);
          break;
        }
        case 'trimStart': {
          const deltaSeconds = (deltaBeats * 60) / bpm;
          const newTrimStart = Math.max(0, initialValue + deltaSeconds);
          trimClip(clip.id, newTrimStart, clip.trimEnd);
          break;
        }
        case 'trimEnd': {
          const deltaSeconds = -(deltaBeats * 60) / bpm;
          const newTrimEnd = Math.max(0, initialValue + deltaSeconds);
          trimClip(clip.id, clip.trimStart, newTrimEnd);
          break;
        }
        case 'fadeIn': {
          const deltaSeconds = (deltaBeats * 60) / bpm;
          const newFadeIn = Math.max(0, initialValue + deltaSeconds);
          setClipFade(clip.id, 'fadeIn', newFadeIn);
          break;
        }
        case 'fadeOut': {
          const deltaSeconds = -(deltaBeats * 60) / bpm;
          const newFadeOut = Math.max(0, initialValue + deltaSeconds);
          setClipFade(clip.id, 'fadeOut', newFadeOut);
          break;
        }
      }
    };

    const handleMouseUp = () => {
      setDragMode('none');
    };

    document.addEventListener('mousemove', handleMouseMove);
    document.addEventListener('mouseup', handleMouseUp);

    return () => {
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
    };
  }, [dragMode, dragStart, initialValue, clip.id, pixelsPerBeat, bpm]);

  return (
    <motion.div
      ref={clipRef}
      layout
      initial={{ opacity: 0, scale: 0.9 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.9 }}
      className={`absolute rounded-md overflow-hidden cursor-move group ${
        clip.isSelected ? 'ring-2 ring-primary ring-offset-1' : ''
      } ${clip.isMuted ? 'opacity-50' : ''} ${
        clip.isLocked ? 'cursor-not-allowed' : ''
      }`}
      style={{
        left: `${clipLeft}px`,
        width: `${clipWidth}px`,
        height: `${trackHeight - 8}px`,
        top: '4px',
        backgroundColor: `${clip.color}20`,
        borderLeft: `2px solid ${clip.color}`,
        borderRight: `2px solid ${clip.color}`,
      }}
      onClick={handleClick}
      onMouseEnter={() => setIsHovered(true)}
      onMouseLeave={() => setIsHovered(false)}
      onContextMenu={(e) => onContextMenu && onContextMenu(e, clip.id)}
      onMouseDown={(e) => handleMouseDown(e, 'move')}
    >
      {/* Waveform */}
      {clip.audioFile?.peaksData && (
        <div className="absolute inset-0 pointer-events-none">
          <WaveformCanvas
            clip={clip}
            width={clipWidth}
            height={trackHeight - 8}
          />
        </div>
      )}

      {/* Fade In Overlay */}
      {clip.fadeIn > 0 && (
        <div
          className="absolute inset-y-0 left-0 pointer-events-none"
          style={{
            width: `${fadeInWidth}px`,
            background: `linear-gradient(to right, rgba(0,0,0,0.5), transparent)`,
          }}
        />
      )}

      {/* Fade Out Overlay */}
      {clip.fadeOut > 0 && (
        <div
          className="absolute inset-y-0 right-0 pointer-events-none"
          style={{
            width: `${fadeOutWidth}px`,
            background: `linear-gradient(to left, rgba(0,0,0,0.5), transparent)`,
          }}
        />
      )}

      {/* Clip Info */}
      <div className="absolute top-1 left-2 flex items-center gap-1 pointer-events-none">
        <span className="text-xs font-medium text-foreground drop-shadow-md truncate max-w-[200px]">
          {clip.name}
        </span>
        {clip.playbackRate !== 1 && (
          <TrendingUp className="h-3 w-3 text-yellow-400" />
        )}
        {clip.isMuted && <Volume2 className="h-3 w-3 text-red-400" />}
        {clip.isLocked && <Lock className="h-3 w-3 text-gray-400" />}
      </div>

      {/* Trim Start Handle */}
      {!clip.isLocked && (
        <motion.div
          className="absolute left-0 top-0 bottom-0 w-1 cursor-ew-resize bg-primary opacity-0 group-hover:opacity-100 hover:!opacity-100 hover:w-2 transition-all"
          onMouseDown={(e) => {
            e.stopPropagation();
            handleMouseDown(e, 'trimStart');
          }}
          whileHover={{ scaleX: 2 }}
          title="Drag to trim start"
        />
      )}

      {/* Trim End Handle */}
      {!clip.isLocked && (
        <motion.div
          className="absolute right-0 top-0 bottom-0 w-1 cursor-ew-resize bg-primary opacity-0 group-hover:opacity-100 hover:!opacity-100 hover:w-2 transition-all"
          onMouseDown={(e) => {
            e.stopPropagation();
            handleMouseDown(e, 'trimEnd');
          }}
          whileHover={{ scaleX: 2 }}
          title="Drag to trim end"
        />
      )}

      {/* Fade In Handle */}
      {!clip.isLocked && isHovered && (
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          className="absolute left-0 top-0 bottom-0 w-2 cursor-col-resize"
          onMouseDown={(e) => {
            e.stopPropagation();
            handleMouseDown(e, 'fadeIn');
          }}
        >
          <div
            className="absolute left-0 top-0 bottom-0 w-full"
            style={{
              width: `${Math.max(fadeInWidth, 10)}px`,
              background: 'linear-gradient(to right, rgba(59, 130, 246, 0.5), transparent)',
              borderLeft: '2px solid rgb(59, 130, 246)',
            }}
            title="Drag to adjust fade in"
          />
        </motion.div>
      )}

      {/* Fade Out Handle */}
      {!clip.isLocked && isHovered && (
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          className="absolute right-0 top-0 bottom-0 w-2 cursor-col-resize"
          onMouseDown={(e) => {
            e.stopPropagation();
            handleMouseDown(e, 'fadeOut');
          }}
        >
          <div
            className="absolute right-0 top-0 bottom-0 w-full"
            style={{
              width: `${Math.max(fadeOutWidth, 10)}px`,
              background: 'linear-gradient(to left, rgba(59, 130, 246, 0.5), transparent)',
              borderRight: '2px solid rgb(59, 130, 246)',
            }}
            title="Drag to adjust fade out"
          />
        </motion.div>
      )}

      {/* Crossfade Indicator */}
      {clip.crossfade && (
        <div className="absolute top-1 right-2 flex items-center gap-1 pointer-events-none">
          <div className="h-2 w-8 bg-gradient-to-r from-transparent via-primary to-transparent opacity-60 rounded" />
        </div>
      )}

      {/* Time-stretch indicator */}
      {clip.playbackRate !== 1 && (
        <div className="absolute bottom-1 right-2 text-[10px] font-mono text-yellow-400 drop-shadow-md pointer-events-none">
          {(clip.playbackRate * 100).toFixed(0)}%
        </div>
      )}
    </motion.div>
  );
}
