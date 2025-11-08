import { Track } from '@/types/audio';
import { Volume2, PanelRight, Sliders } from 'lucide-react';
import { useState } from 'react';

interface RightPanelProps {
  tracks: Track[];
}

export default function RightPanel({ tracks }: RightPanelProps) {
  const [tab, setTab] = useState<'mixer' | 'inspector'>('mixer');

  return (
    <div className="w-72 bg-card border-l border-border flex flex-col">
      {/* Header */}
      <div className="h-12 border-b border-border flex items-center">
        <button
          onClick={() => setTab('mixer')}
          className={`flex-1 h-full flex items-center justify-center gap-2 border-r border-border ${
            tab === 'mixer' ? 'bg-accent/50' : 'hover:bg-accent/20'
          }`}
        >
          <Sliders className="h-4 w-4" />
          <span className="text-sm font-medium">Mixer</span>
        </button>
        <button
          onClick={() => setTab('inspector')}
          className={`flex-1 h-full flex items-center justify-center gap-2 ${
            tab === 'inspector' ? 'bg-accent/50' : 'hover:bg-accent/20'
          }`}
        >
          <PanelRight className="h-4 w-4" />
          <span className="text-sm font-medium">Inspector</span>
        </button>
      </div>

      {/* Content */}
      <div className="flex-1 overflow-y-auto">
        {tab === 'mixer' ? <MixerView tracks={tracks} /> : <InspectorView />}
      </div>
    </div>
  );
}

function MixerView({ tracks }: { tracks: Track[] }) {
  if (tracks.length === 0) {
    return (
      <div className="h-full flex items-center justify-center text-muted-foreground text-sm p-4 text-center">
        <div>
          <Sliders className="h-8 w-8 mx-auto mb-2 opacity-50" />
          <p>No tracks to mix</p>
        </div>
      </div>
    );
  }

  return (
    <div className="p-4 space-y-6">
      {tracks.map((track) => (
        <div key={track.id} className="space-y-3">
          <div className="flex items-center justify-between">
            <span className="text-sm font-medium truncate">{track.name}</span>
            <span className="text-xs text-muted-foreground">{track.type}</span>
          </div>

          {/* Volume */}
          <div>
            <div className="flex items-center justify-between mb-2">
              <span className="text-xs text-muted-foreground">Volume</span>
              <span className="text-xs font-mono">
                {Math.round((track.volume - 1) * 20)} dB
              </span>
            </div>
            <input
              type="range"
              min="0"
              max="1"
              step="0.01"
              value={track.volume}
              className="w-full"
            />
          </div>

          {/* Pan */}
          <div>
            <div className="flex items-center justify-between mb-2">
              <span className="text-xs text-muted-foreground">Pan</span>
              <span className="text-xs font-mono">
                {track.pan === 0 ? 'C' : track.pan > 0 ? `${Math.round(track.pan * 100)}R` : `${Math.round(Math.abs(track.pan) * 100)}L`}
              </span>
            </div>
            <input
              type="range"
              min="-1"
              max="1"
              step="0.01"
              value={track.pan}
              className="w-full"
            />
          </div>

          {/* Buttons */}
          <div className="flex gap-2">
            <button
              className={`flex-1 px-3 py-1 text-xs rounded ${
                track.muted ? 'bg-yellow-500/20 text-yellow-500' : 'bg-secondary'
              }`}
            >
              M
            </button>
            <button
              className={`flex-1 px-3 py-1 text-xs rounded ${
                track.solo ? 'bg-primary/20 text-primary' : 'bg-secondary'
              }`}
            >
              S
            </button>
          </div>

          <div className="border-t border-border" />
        </div>
      ))}
    </div>
  );
}

function InspectorView() {
  return (
    <div className="h-full flex items-center justify-center text-muted-foreground text-sm p-4 text-center">
      <div>
        <PanelRight className="h-8 w-8 mx-auto mb-2 opacity-50" />
        <p className="mb-2">Inspector</p>
        <p className="text-xs">Select a track or clip to view properties</p>
      </div>
    </div>
  );
}
