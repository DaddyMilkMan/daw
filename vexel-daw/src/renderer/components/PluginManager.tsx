/**
 * Plugin Manager Component
 * Browse, scan, and manage WAM plugins
 */

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { X, Plus, Search, Download, Trash2, Settings, RefreshCw } from 'lucide-react';
import { Button } from './ui/button';
import { pluginHost } from '../services/pluginHost';
import { PluginDescriptor, PluginCategory } from '../types/plugin';

interface PluginManagerProps {
  open: boolean;
  onClose: () => void;
  onSelectPlugin?: (plugin: PluginDescriptor) => void;
}

const PLUGIN_CATEGORIES: { id: PluginCategory; label: string }[] = [
  { id: 'eq', label: 'EQ' },
  { id: 'dynamics', label: 'Dynamics' },
  { id: 'reverb', label: 'Reverb' },
  { id: 'delay', label: 'Delay' },
  { id: 'modulation', label: 'Modulation' },
  { id: 'distortion', label: 'Distortion' },
  { id: 'filter', label: 'Filter' },
  { id: 'instrument', label: 'Instrument' },
  { id: 'utility', label: 'Utility' },
  { id: 'other', label: 'Other' },
];

export default function PluginManager({ open, onClose, onSelectPlugin }: PluginManagerProps) {
  const [plugins, setPlugins] = useState<PluginDescriptor[]>([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedCategory, setSelectedCategory] = useState<PluginCategory | 'all'>('all');
  const [scanningUrl, setScanningUrl] = useState('');
  const [isScanning, setIsScanning] = useState(false);
  const [scanError, setScanError] = useState<string | null>(null);

  // Load plugins from local storage on mount
  useEffect(() => {
    if (open) {
      loadPlugins();
    }
  }, [open]);

  const loadPlugins = () => {
    // Load from local storage
    const stored = localStorage.getItem('zenith-daw-plugins');
    if (stored) {
      try {
        const parsed = JSON.parse(stored);
        setPlugins(parsed);
      } catch (error) {
        console.error('Failed to load plugins:', error);
      }
    }
  };

  const savePlugins = (pluginList: PluginDescriptor[]) => {
    localStorage.setItem('zenith-daw-plugins', JSON.stringify(pluginList));
    setPlugins(pluginList);
  };

  const handleScanPlugin = async () => {
    if (!scanningUrl.trim()) {
      setScanError('Please enter a plugin URL');
      return;
    }

    setIsScanning(true);
    setScanError(null);

    const result = await pluginHost.scanPlugin(scanningUrl);

    setIsScanning(false);

    if (result.success && result.descriptor) {
      // Add to list
      const updated = [...plugins, result.descriptor];
      savePlugins(updated);
      setScanningUrl('');
      alert(`Successfully added plugin: ${result.descriptor.name}`);
    } else {
      setScanError(result.error || 'Failed to scan plugin');
    }
  };

  const handleDeletePlugin = (pluginId: string) => {
    if (confirm('Are you sure you want to remove this plugin?')) {
      const updated = plugins.filter((p) => p.id !== pluginId);
      savePlugins(updated);
    }
  };

  const handleSelectPlugin = (plugin: PluginDescriptor) => {
    if (onSelectPlugin) {
      onSelectPlugin(plugin);
      onClose();
    }
  };

  // Filter plugins
  const filteredPlugins = plugins.filter((plugin) => {
    const matchesSearch =
      plugin.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
      plugin.vendor.toLowerCase().includes(searchQuery.toLowerCase());

    const matchesCategory = selectedCategory === 'all' || plugin.category === selectedCategory;

    return matchesSearch && matchesCategory;
  });

  if (!open) return null;

  return (
    <AnimatePresence>
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        exit={{ opacity: 0 }}
        className="fixed inset-0 bg-black/50 backdrop-blur-sm z-50 flex items-center justify-center p-4"
        onClick={onClose}
      >
        <motion.div
          initial={{ scale: 0.95, opacity: 0 }}
          animate={{ scale: 1, opacity: 1 }}
          exit={{ scale: 0.95, opacity: 0 }}
          onClick={(e) => e.stopPropagation()}
          className="bg-card border border-border rounded-lg shadow-2xl w-full max-w-4xl max-h-[80vh] overflow-hidden flex flex-col"
        >
          {/* Header */}
          <div className="flex items-center justify-between p-6 border-b border-border">
            <div>
              <h2 className="text-2xl font-bold text-foreground">Plugin Manager</h2>
              <p className="text-sm text-muted-foreground mt-1">
                Browse and manage Web Audio Modules (WAM)
              </p>
            </div>
            <Button size="icon" variant="ghost" onClick={onClose}>
              <X className="h-5 w-5" />
            </Button>
          </div>

          {/* Scan Section */}
          <div className="p-6 border-b border-border bg-muted/30">
            <div className="flex gap-3">
              <div className="flex-1">
                <input
                  type="text"
                  placeholder="Enter plugin URL (e.g., https://example.com/plugin.js)"
                  value={scanningUrl}
                  onChange={(e) => setScanningUrl(e.target.value)}
                  onKeyDown={(e) => {
                    if (e.key === 'Enter') {
                      handleScanPlugin();
                    }
                  }}
                  className="w-full px-4 py-2 bg-background border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary text-foreground"
                />
              </div>
              <Button
                onClick={handleScanPlugin}
                disabled={isScanning}
                className="bg-primary hover:bg-primary/90"
              >
                {isScanning ? (
                  <RefreshCw className="h-5 w-5 animate-spin" />
                ) : (
                  <>
                    <Download className="h-5 w-5 mr-2" />
                    Scan Plugin
                  </>
                )}
              </Button>
            </div>
            {scanError && (
              <p className="text-sm text-red-500 mt-2">{scanError}</p>
            )}
          </div>

          {/* Search and Filter */}
          <div className="p-6 border-b border-border">
            <div className="flex gap-4 mb-4">
              <div className="flex-1 relative">
                <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 h-5 w-5 text-muted-foreground" />
                <input
                  type="text"
                  placeholder="Search plugins..."
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  className="w-full pl-10 pr-4 py-2 bg-background border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary text-foreground"
                />
              </div>
            </div>

            {/* Category Filter */}
            <div className="flex flex-wrap gap-2">
              <Button
                size="sm"
                variant={selectedCategory === 'all' ? 'default' : 'outline'}
                onClick={() => setSelectedCategory('all')}
              >
                All
              </Button>
              {PLUGIN_CATEGORIES.map((cat) => (
                <Button
                  key={cat.id}
                  size="sm"
                  variant={selectedCategory === cat.id ? 'default' : 'outline'}
                  onClick={() => setSelectedCategory(cat.id)}
                >
                  {cat.label}
                </Button>
              ))}
            </div>
          </div>

          {/* Plugin List */}
          <div className="flex-1 overflow-y-auto p-6">
            {filteredPlugins.length === 0 ? (
              <div className="text-center py-12">
                <p className="text-muted-foreground text-lg">No plugins found</p>
                <p className="text-sm text-muted-foreground mt-2">
                  Scan a plugin URL to get started
                </p>
              </div>
            ) : (
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                {filteredPlugins.map((plugin) => (
                  <motion.div
                    key={plugin.id}
                    layout
                    initial={{ opacity: 0, y: 20 }}
                    animate={{ opacity: 1, y: 0 }}
                    className="bg-muted/50 border border-border rounded-lg p-4 hover:bg-muted/70 transition-colors cursor-pointer"
                    onClick={() => handleSelectPlugin(plugin)}
                  >
                    <div className="flex items-start justify-between">
                      <div className="flex-1">
                        <h3 className="font-semibold text-foreground">{plugin.name}</h3>
                        <p className="text-sm text-muted-foreground">{plugin.vendor}</p>
                        <div className="flex items-center gap-2 mt-2">
                          <span className="text-xs bg-primary/20 text-primary px-2 py-1 rounded">
                            {plugin.category}
                          </span>
                          {plugin.isInstrument && (
                            <span className="text-xs bg-green-500/20 text-green-500 px-2 py-1 rounded">
                              Instrument
                            </span>
                          )}
                          <span className="text-xs text-muted-foreground">v{plugin.version}</span>
                        </div>
                      </div>
                      <Button
                        size="icon"
                        variant="ghost"
                        onClick={(e) => {
                          e.stopPropagation();
                          handleDeletePlugin(plugin.id);
                        }}
                        className="text-red-500 hover:text-red-600 hover:bg-red-500/10"
                      >
                        <Trash2 className="h-4 w-4" />
                      </Button>
                    </div>
                  </motion.div>
                ))}
              </div>
            )}
          </div>

          {/* Footer */}
          <div className="p-6 border-t border-border bg-muted/30">
            <p className="text-xs text-muted-foreground">
              <strong>Note:</strong> WAM plugins must be hosted on a web server with CORS enabled.
              Visit{' '}
              <a
                href="https://github.com/webaudiomodules"
                target="_blank"
                rel="noopener noreferrer"
                className="text-primary hover:underline"
              >
                github.com/webaudiomodules
              </a>{' '}
              for available plugins.
            </p>
          </div>
        </motion.div>
      </motion.div>
    </AnimatePresence>
  );
}
