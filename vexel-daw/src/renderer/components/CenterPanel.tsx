import { Track } from '@/types/audio';
import { Plus, Grid3X3, List } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { Button } from './ui/button';
import { useState } from 'react';

interface CenterPanelProps {
  tracks: Track[];
  onOpenPianoRoll: (trackId: string, trackName: string) => void;
}

export default function CenterPanel({ tracks, onOpenPianoRoll }: CenterPanelProps) {
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
            <ArrangementView key="arrangement" tracks={tracks} onOpenPianoRoll={onOpenPianoRoll} />
          ) : (
            <SessionView key="session" tracks={tracks} />
          )}
        </AnimatePresence>
      </div>
    </div>
  );
}

function ArrangementView({ tracks, onOpenPianoRoll }: { tracks: Track[]; onOpenPianoRoll: (trackId: string, trackName: string) => void }) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 10 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: -10 }}
      transition={{ duration: 0.2 }}
      className="h-full flex"
    >
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
                  className="h-16 border-b border-border/30 px-3 py-2 cursor-pointer transition-colors"
                  onDoubleClick={() => onOpenPianoRoll(track.id, track.name)}
                  title="Double-click to open Piano Roll"
                >
                  <div className="text-sm font-medium truncate">{track.name}</div>
                  <div className="text-xs text-muted-foreground">
                    {track.type.toUpperCase()}
                  </div>
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
    </motion.div>
  );
}

function SessionView({ tracks }: { tracks: Track[] }) {
  return (
    <motion.div
      initial={{ opacity: 0, scale: 0.98 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.98 }}
      transition={{ duration: 0.2 }}
      className="h-full flex items-center justify-center text-muted-foreground"
    >
      <div className="text-center">
        <motion.div
          animate={{
            rotate: [0, 360],
          }}
          transition={{
            duration: 20,
            repeat: Infinity,
            ease: 'linear',
          }}
        >
          <Grid3X3 className="h-12 w-12 mx-auto mb-4 opacity-50" />
        </motion.div>
        <p className="text-lg font-semibold mb-2">Session View</p>
        <p className="text-sm">Clip launcher coming soon</p>
        <p className="text-xs text-muted-foreground/60 mt-4">
          Press <kbd className="px-2 py-1 bg-secondary rounded">Tab</kbd> to switch views
        </p>
      </div>
    </motion.div>
  );
}
