import { Search, Folder, Music, Drum, Zap } from 'lucide-react';
import { useState } from 'react';

export default function LeftPanel() {
  const [searchQuery, setSearchQuery] = useState('');

  return (
    <div className="w-64 bg-card border-r border-border flex flex-col">
      {/* Header */}
      <div className="h-12 border-b border-border flex items-center px-4">
        <h2 className="font-semibold">Browser</h2>
      </div>

      {/* Search */}
      <div className="p-4 border-b border-border">
        <div className="relative">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 h-4 w-4 text-muted-foreground" />
          <input
            type="text"
            placeholder="Search sounds, plugins..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-10 pr-3 py-2 bg-background border border-input rounded text-sm focus:outline-none focus:ring-2 focus:ring-primary"
          />
        </div>
      </div>

      {/* Categories */}
      <div className="flex-1 overflow-y-auto p-2">
        <div className="space-y-1">
          <CategoryItem icon={<Music />} label="Instruments" count={24} />
          <CategoryItem icon={<Zap />} label="Effects" count={42} />
          <CategoryItem icon={<Drum />} label="Drums" count={156} />
          <CategoryItem icon={<Folder />} label="Samples" count={892} />
          <CategoryItem icon={<Music />} label="MIDI" count={18} />
        </div>

        {/* Sample Items */}
        <div className="mt-6">
          <div className="px-2 py-1 text-xs font-semibold text-muted-foreground">
            RECENT
          </div>
          <div className="space-y-1 mt-2">
            <SampleItem name="Kick - Deep" type="Audio" />
            <SampleItem name="Serum" type="Plugin" />
            <SampleItem name="Reverb Pro" type="Effect" />
            <SampleItem name="Snare - Crisp" type="Audio" />
          </div>
        </div>
      </div>
    </div>
  );
}

interface CategoryItemProps {
  icon: React.ReactNode;
  label: string;
  count: number;
}

function CategoryItem({ icon, label, count }: CategoryItemProps) {
  return (
    <button className="w-full flex items-center gap-3 px-3 py-2 rounded hover:bg-accent/50 transition-colors group">
      <div className="text-muted-foreground group-hover:text-foreground transition-colors">
        {icon}
      </div>
      <span className="flex-1 text-left text-sm">{label}</span>
      <span className="text-xs text-muted-foreground">{count}</span>
    </button>
  );
}

interface SampleItemProps {
  name: string;
  type: string;
}

function SampleItem({ name, type }: SampleItemProps) {
  return (
    <div className="px-3 py-2 rounded hover:bg-accent/50 cursor-pointer transition-colors">
      <div className="text-sm font-medium">{name}</div>
      <div className="text-xs text-muted-foreground">{type}</div>
    </div>
  );
}
