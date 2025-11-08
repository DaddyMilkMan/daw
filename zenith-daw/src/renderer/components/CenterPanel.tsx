import { Track } from '@/types/audio';
import { Plus, Grid3X3, List } from 'lucide-react';
import { Button } from './ui/button';
import { useState } from 'react';

interface CenterPanelProps {
  tracks: Track[];
}

export default function CenterPanel({ tracks }: CenterPanelProps) {
  const [view, setView] = useState<'session' | 'arrangement'>('arrangement');

  const handleCreateTrack = () => {
    const name = `Track ${tracks.length + 1}`;
    window.electron.createTrack(name, 'midi');
  };

  return (
    <div className="flex-1 bg-background flex flex-col min-w-0">
      {/* Header */}
      <div className="h-12 border-b border-border flex items-center px-4 justify-between">
        <div className="flex items-center gap-2">
          <Button
            size="sm"
            variant={view === 'session' ? 'default' : 'ghost'}
            onClick={() => setView('session')}
          >
            <Grid3X3 className="h-4 w-4 mr-2" />
            Session
          </Button>
          <Button
            size="sm"
            variant={view === 'arrangement' ? 'default' : 'ghost'}
            onClick={() => setView('arrangement')}
          >
            <List className="h-4 w-4 mr-2" />
            Arrangement
          </Button>
        </div>

        <Button size="sm" onClick={handleCreateTrack}>
          <Plus className="h-4 w-4 mr-2" />
          Add Track
        </Button>
      </div>

      {/* View Content */}
      <div className="flex-1 overflow-auto">
        {view === 'arrangement' ? (
          <ArrangementView tracks={tracks} />
        ) : (
          <SessionView tracks={tracks} />
        )}
      </div>
    </div>
  );
}

function ArrangementView({ tracks }: { tracks: Track[] }) {
  return (
    <div className="h-full flex">
      {/* Track List */}
      <div className="w-48 border-r border-border bg-card">
        {tracks.length === 0 ? (
          <div className="h-full flex items-center justify-center text-muted-foreground text-sm p-4 text-center">
            No tracks yet. Click "Add Track" to get started.
          </div>
        ) : (
          <div>
            {tracks.map((track) => (
              <div
                key={track.id}
                className="h-16 border-b border-border px-3 py-2 hover:bg-accent/20 cursor-pointer"
              >
                <div className="text-sm font-medium truncate">{track.name}</div>
                <div className="text-xs text-muted-foreground">{track.type.toUpperCase()}</div>
              </div>
            ))}
          </div>
        )}
      </div>

      {/* Timeline */}
      <div className="flex-1 relative overflow-x-auto">
        {/* Ruler */}
        <div className="h-8 bg-card border-b border-border flex items-center">
          {Array.from({ length: 16 }).map((_, i) => (
            <div
              key={i}
              className="flex-shrink-0 w-32 border-r border-border px-2 text-xs text-muted-foreground"
            >
              {i + 1}
            </div>
          ))}
        </div>

        {/* Track Lanes */}
        <div>
          {tracks.map((track) => (
            <div
              key={track.id}
              className="h-16 border-b border-border flex items-center relative"
            >
              {Array.from({ length: 16 }).map((_, i) => (
                <div
                  key={i}
                  className="flex-shrink-0 w-32 h-full border-r border-border/50 hover:bg-primary/5"
                />
              ))}
            </div>
          ))}
        </div>

        {tracks.length === 0 && (
          <div className="absolute inset-0 flex items-center justify-center text-muted-foreground">
            <div className="text-center">
              <p className="text-lg font-semibold mb-2">Arrangement View</p>
              <p className="text-sm">Add tracks to start arranging</p>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

function SessionView({ tracks }: { tracks: Track[] }) {
  return (
    <div className="h-full flex items-center justify-center text-muted-foreground">
      <div className="text-center">
        <Grid3X3 className="h-12 w-12 mx-auto mb-4 opacity-50" />
        <p className="text-lg font-semibold mb-2">Session View</p>
        <p className="text-sm">Clip launcher coming soon</p>
      </div>
    </div>
  );
}
