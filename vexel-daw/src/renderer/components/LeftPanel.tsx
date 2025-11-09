import { Search, Folder, Music, Drum, Zap, Star, Play, Pause, Grid3X3, List, SortAsc, SortDesc, Plus, X, Heart } from 'lucide-react';
import { useState, useMemo, useRef, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Button } from './ui/button';
import { BrowserItem, BrowserCollection, SAMPLE_BROWSER_ITEMS } from '../types/browser';

export default function LeftPanel() {
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedCategory, setSelectedCategory] = useState<string | null>(null);
  const [selectedTags, setSelectedTags] = useState<string[]>([]);
  const [showFavoritesOnly, setShowFavoritesOnly] = useState(false);
  const [sortBy, setSortBy] = useState<'name' | 'date-added' | 'last-used'>('name');
  const [sortOrder, setSortOrder] = useState<'asc' | 'desc'>('asc');
  const [viewMode, setViewMode] = useState<'list' | 'grid'>('list');
  const [items, setItems] = useState<BrowserItem[]>(SAMPLE_BROWSER_ITEMS);
  const [previewingItem, setPreviewingItem] = useState<string | null>(null);
  const [isPreviewPlaying, setIsPreviewPlaying] = useState(false);
  const [collections, setCollections] = useState<BrowserCollection[]>([]);
  const [showCollections, setShowCollections] = useState(false);

  // Filter and sort items
  const filteredItems = useMemo(() => {
    let filtered = items;

    // Filter by search query
    if (searchQuery) {
      const query = searchQuery.toLowerCase();
      filtered = filtered.filter(
        (item) =>
          item.name.toLowerCase().includes(query) ||
          item.tags.some((tag) => tag.toLowerCase().includes(query))
      );
    }

    // Filter by category
    if (selectedCategory) {
      filtered = filtered.filter((item) => item.category === selectedCategory);
    }

    // Filter by tags
    if (selectedTags.length > 0) {
      filtered = filtered.filter((item) =>
        selectedTags.every((tag) => item.tags.includes(tag))
      );
    }

    // Filter by favorites
    if (showFavoritesOnly) {
      filtered = filtered.filter((item) => item.isFavorite);
    }

    // Sort items
    filtered.sort((a, b) => {
      let comparison = 0;

      switch (sortBy) {
        case 'name':
          comparison = a.name.localeCompare(b.name);
          break;
        case 'date-added':
          comparison = a.dateAdded.getTime() - b.dateAdded.getTime();
          break;
        case 'last-used':
          comparison = (a.lastUsed?.getTime() || 0) - (b.lastUsed?.getTime() || 0);
          break;
      }

      return sortOrder === 'asc' ? comparison : -comparison;
    });

    return filtered;
  }, [items, searchQuery, selectedCategory, selectedTags, showFavoritesOnly, sortBy, sortOrder]);

  // Get all unique tags from items
  const allTags = useMemo(() => {
    const tagSet = new Set<string>();
    items.forEach((item) => {
      item.tags.forEach((tag) => tagSet.add(tag));
    });
    return Array.from(tagSet).sort();
  }, [items]);

  // Category counts
  const categoryCounts = useMemo(() => {
    return {
      instruments: items.filter((i) => i.category === 'instruments').length,
      effects: items.filter((i) => i.category === 'effects').length,
      drums: items.filter((i) => i.category === 'drums').length,
      samples: items.filter((i) => i.category === 'samples').length,
      midi: items.filter((i) => i.category === 'midi').length,
    };
  }, [items]);

  const toggleFavorite = (itemId: string) => {
    setItems((prev) =>
      prev.map((item) =>
        item.id === itemId ? { ...item, isFavorite: !item.isFavorite } : item
      )
    );
  };

  const togglePreview = (itemId: string) => {
    if (previewingItem === itemId) {
      setIsPreviewPlaying(!isPreviewPlaying);
    } else {
      setPreviewingItem(itemId);
      setIsPreviewPlaying(true);
    }
  };

  const toggleTag = (tag: string) => {
    setSelectedTags((prev) =>
      prev.includes(tag) ? prev.filter((t) => t !== tag) : [...prev, tag]
    );
  };

  const toggleSort = () => {
    setSortOrder((prev) => (prev === 'asc' ? 'desc' : 'asc'));
  };

  return (
    <div className="w-64 bg-card border-r border-border flex flex-col">
      {/* Header */}
      <div className="h-12 border-b border-border/50 flex items-center justify-between px-4 bg-card/50 backdrop-blur-xl">
        <h2 className="font-semibold">Browser</h2>
        <div className="flex items-center gap-1">
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={() => setShowFavoritesOnly(!showFavoritesOnly)}
            className={`p-1.5 rounded transition-colors ${
              showFavoritesOnly
                ? 'bg-primary text-primary-foreground'
                : 'hover:bg-accent'
            }`}
            title="Show favorites only"
          >
            <Heart className="h-4 w-4" fill={showFavoritesOnly ? 'currentColor' : 'none'} />
          </motion.button>
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={() => setViewMode(viewMode === 'list' ? 'grid' : 'list')}
            className="p-1.5 rounded hover:bg-accent transition-colors"
            title={`Switch to ${viewMode === 'list' ? 'grid' : 'list'} view`}
          >
            {viewMode === 'list' ? (
              <Grid3X3 className="h-4 w-4" />
            ) : (
              <List className="h-4 w-4" />
            )}
          </motion.button>
        </div>
      </div>

      {/* Search */}
      <div className="p-3 border-b border-border/50 space-y-2">
        <div className="relative">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 h-4 w-4 text-muted-foreground" />
          <input
            type="text"
            placeholder="Search sounds, plugins..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-10 pr-3 py-2 bg-background/50 border border-input rounded text-sm focus:outline-none focus:ring-2 focus:ring-primary transition-all"
          />
        </div>

        {/* Sort Controls */}
        <div className="flex items-center gap-2">
          <select
            value={sortBy}
            onChange={(e) => setSortBy(e.target.value as any)}
            className="flex-1 px-2 py-1.5 bg-background/50 border border-input rounded text-xs focus:outline-none focus:ring-2 focus:ring-primary"
          >
            <option value="name">Name</option>
            <option value="date-added">Date Added</option>
            <option value="last-used">Last Used</option>
          </select>
          <motion.button
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            onClick={toggleSort}
            className="p-1.5 rounded hover:bg-accent transition-colors"
            title={`Sort ${sortOrder === 'asc' ? 'descending' : 'ascending'}`}
          >
            {sortOrder === 'asc' ? (
              <SortAsc className="h-4 w-4" />
            ) : (
              <SortDesc className="h-4 w-4" />
            )}
          </motion.button>
        </div>
      </div>

      {/* Selected Tags */}
      {selectedTags.length > 0 && (
        <div className="px-3 py-2 border-b border-border/50 bg-accent/30">
          <div className="flex flex-wrap gap-1">
            {selectedTags.map((tag) => (
              <motion.button
                key={tag}
                initial={{ scale: 0 }}
                animate={{ scale: 1 }}
                exit={{ scale: 0 }}
                whileHover={{ scale: 1.05 }}
                onClick={() => toggleTag(tag)}
                className="px-2 py-0.5 bg-primary/20 text-primary border border-primary/30 rounded-full text-xs flex items-center gap-1"
              >
                {tag}
                <X className="h-3 w-3" />
              </motion.button>
            ))}
          </div>
        </div>
      )}

      {/* Categories */}
      <div className="px-2 py-3 border-b border-border/50">
        <div className="space-y-0.5">
          <CategoryItem
            icon={<Music className="h-4 w-4" />}
            label="Instruments"
            count={categoryCounts.instruments}
            isActive={selectedCategory === 'instruments'}
            onClick={() =>
              setSelectedCategory(selectedCategory === 'instruments' ? null : 'instruments')
            }
          />
          <CategoryItem
            icon={<Zap className="h-4 w-4" />}
            label="Effects"
            count={categoryCounts.effects}
            isActive={selectedCategory === 'effects'}
            onClick={() =>
              setSelectedCategory(selectedCategory === 'effects' ? null : 'effects')
            }
          />
          <CategoryItem
            icon={<Drum className="h-4 w-4" />}
            label="Drums"
            count={categoryCounts.drums}
            isActive={selectedCategory === 'drums'}
            onClick={() =>
              setSelectedCategory(selectedCategory === 'drums' ? null : 'drums')
            }
          />
          <CategoryItem
            icon={<Folder className="h-4 w-4" />}
            label="Samples"
            count={categoryCounts.samples}
            isActive={selectedCategory === 'samples'}
            onClick={() =>
              setSelectedCategory(selectedCategory === 'samples' ? null : 'samples')
            }
          />
          <CategoryItem
            icon={<Music className="h-4 w-4" />}
            label="MIDI"
            count={categoryCounts.midi}
            isActive={selectedCategory === 'midi'}
            onClick={() =>
              setSelectedCategory(selectedCategory === 'midi' ? null : 'midi')
            }
          />
        </div>
      </div>

      {/* Items List */}
      <div className="flex-1 overflow-y-auto p-2">
        <div className="space-y-1">
          <AnimatePresence mode="popLayout">
            {filteredItems.length === 0 ? (
              <motion.div
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                exit={{ opacity: 0 }}
                className="text-center py-8 text-muted-foreground text-sm"
              >
                <Search className="h-8 w-8 mx-auto mb-2 opacity-50" />
                <p>No items found</p>
                <p className="text-xs mt-1">Try adjusting your filters</p>
              </motion.div>
            ) : (
              filteredItems.map((item, index) => (
                <BrowserItemComponent
                  key={item.id}
                  item={item}
                  index={index}
                  viewMode={viewMode}
                  isPreviewPlaying={previewingItem === item.id && isPreviewPlaying}
                  onToggleFavorite={() => toggleFavorite(item.id)}
                  onTogglePreview={() => togglePreview(item.id)}
                  onTagClick={toggleTag}
                />
              ))
            )}
          </AnimatePresence>
        </div>

        {/* Popular Tags */}
        {filteredItems.length > 0 && (
          <div className="mt-6 pb-4">
            <div className="px-2 py-1 text-xs font-semibold text-muted-foreground">
              POPULAR TAGS
            </div>
            <div className="flex flex-wrap gap-1 mt-2">
              {allTags.slice(0, 12).map((tag) => (
                <motion.button
                  key={tag}
                  whileHover={{ scale: 1.05 }}
                  whileTap={{ scale: 0.95 }}
                  onClick={() => toggleTag(tag)}
                  className={`px-2 py-0.5 rounded-full text-xs transition-colors ${
                    selectedTags.includes(tag)
                      ? 'bg-primary text-primary-foreground'
                      : 'bg-accent/50 hover:bg-accent text-muted-foreground hover:text-foreground'
                  }`}
                >
                  {tag}
                </motion.button>
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

interface CategoryItemProps {
  icon: React.ReactNode;
  label: string;
  count: number;
  isActive: boolean;
  onClick: () => void;
}

function CategoryItem({ icon, label, count, isActive, onClick }: CategoryItemProps) {
  return (
    <motion.button
      whileHover={{ x: 2 }}
      whileTap={{ scale: 0.98 }}
      onClick={onClick}
      className={`w-full flex items-center gap-3 px-3 py-2 rounded transition-all ${
        isActive
          ? 'bg-primary text-primary-foreground shadow-sm'
          : 'hover:bg-accent/50 text-muted-foreground hover:text-foreground'
      }`}
    >
      <div className="transition-colors">{icon}</div>
      <span className="flex-1 text-left text-sm font-medium">{label}</span>
      <span className="text-xs opacity-70">{count}</span>
    </motion.button>
  );
}

interface BrowserItemComponentProps {
  item: BrowserItem;
  index: number;
  viewMode: 'list' | 'grid';
  isPreviewPlaying: boolean;
  onToggleFavorite: () => void;
  onTogglePreview: () => void;
  onTagClick: (tag: string) => void;
}

function BrowserItemComponent({
  item,
  index,
  viewMode,
  isPreviewPlaying,
  onToggleFavorite,
  onTogglePreview,
  onTagClick,
}: BrowserItemComponentProps) {
  const hasAudio = item.type === 'drum' || item.type === 'sample';

  return (
    <motion.div
      layout
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, scale: 0.9 }}
      transition={{
        type: 'spring',
        stiffness: 400,
        damping: 30,
        delay: index * 0.02,
      }}
      className="group"
    >
      <div className="px-3 py-2 rounded hover:bg-accent/50 cursor-pointer transition-all">
        <div className="flex items-start justify-between gap-2">
          <div className="flex-1 min-w-0">
            <div className="flex items-center gap-2">
              <div className="text-sm font-medium truncate">{item.name}</div>
              {item.isFavorite && (
                <Star className="h-3 w-3 text-yellow-500 flex-shrink-0" fill="currentColor" />
              )}
            </div>
            <div className="text-xs text-muted-foreground mt-0.5">
              {item.type.charAt(0).toUpperCase() + item.type.slice(1)}
              {item.duration && ` • ${item.duration.toFixed(1)}s`}
              {item.tempo && ` • ${item.tempo} BPM`}
              {item.key && ` • ${item.key}`}
            </div>

            {/* Waveform Preview */}
            {hasAudio && item.waveform && (
              <div className="mt-2 h-8 flex items-center gap-0.5">
                {item.waveform.map((value, i) => (
                  <motion.div
                    key={i}
                    className="flex-1 bg-primary/30 rounded-sm"
                    style={{ height: `${value * 100}%` }}
                    animate={
                      isPreviewPlaying
                        ? {
                            height: [`${value * 100}%`, `${value * 120}%`, `${value * 100}%`],
                            backgroundColor: [
                              'rgba(59, 130, 246, 0.3)',
                              'rgba(59, 130, 246, 0.6)',
                              'rgba(59, 130, 246, 0.3)',
                            ],
                          }
                        : {}
                    }
                    transition={{
                      repeat: isPreviewPlaying ? Infinity : 0,
                      duration: 0.5,
                      delay: i * 0.05,
                    }}
                  />
                ))}
              </div>
            )}

            {/* Tags */}
            <div className="flex flex-wrap gap-1 mt-2">
              {item.tags.slice(0, 3).map((tag) => (
                <button
                  key={tag}
                  onClick={(e) => {
                    e.stopPropagation();
                    onTagClick(tag);
                  }}
                  className="px-1.5 py-0.5 bg-accent/50 hover:bg-accent rounded text-[10px] text-muted-foreground hover:text-foreground transition-colors"
                >
                  {tag}
                </button>
              ))}
            </div>
          </div>

          {/* Actions */}
          <div className="flex flex-col gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
            <motion.button
              whileHover={{ scale: 1.1 }}
              whileTap={{ scale: 0.9 }}
              onClick={(e) => {
                e.stopPropagation();
                onToggleFavorite();
              }}
              className="p-1 rounded hover:bg-background/50 transition-colors"
              title={item.isFavorite ? 'Remove from favorites' : 'Add to favorites'}
            >
              <Heart
                className="h-3.5 w-3.5 text-muted-foreground hover:text-yellow-500 transition-colors"
                fill={item.isFavorite ? 'currentColor' : 'none'}
              />
            </motion.button>
            {hasAudio && (
              <motion.button
                whileHover={{ scale: 1.1 }}
                whileTap={{ scale: 0.9 }}
                onClick={(e) => {
                  e.stopPropagation();
                  onTogglePreview();
                }}
                className="p-1 rounded hover:bg-background/50 transition-colors"
                title={isPreviewPlaying ? 'Pause preview' : 'Play preview'}
              >
                {isPreviewPlaying ? (
                  <Pause className="h-3.5 w-3.5 text-primary" fill="currentColor" />
                ) : (
                  <Play className="h-3.5 w-3.5 text-muted-foreground hover:text-primary transition-colors" />
                )}
              </motion.button>
            )}
          </div>
        </div>
      </div>
    </motion.div>
  );
}
