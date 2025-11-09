import { motion, AnimatePresence } from 'framer-motion';
import { Plus } from 'lucide-react';
import { useState, useRef, useEffect } from 'react';
import { PluginState, BUILTIN_EFFECTS, BuiltInEffectType } from '../types/plugin';
import PluginSlot from './PluginSlot';
import PluginInspector from './PluginInspector';

interface PluginChainViewProps {
  trackId: string;
  plugins: PluginState[];
  onAddPlugin: (trackId: string, type: BuiltInEffectType) => void;
  onRemovePlugin: (trackId: string, pluginId: string) => void;
  onBypassPlugin: (trackId: string, pluginId: string, bypass: boolean) => void;
  onParameterChange: (trackId: string, pluginId: string, parameterId: string, value: number) => void;
}

export default function PluginChainView({
  trackId,
  plugins,
  onAddPlugin,
  onRemovePlugin,
  onBypassPlugin,
  onParameterChange,
}: PluginChainViewProps) {
  const [showAddMenu, setShowAddMenu] = useState(false);
  const [selectedPluginId, setSelectedPluginId] = useState<string | null>(null);
  const menuRef = useRef<HTMLDivElement>(null);

  // Close menu when clicking outside
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (menuRef.current && !menuRef.current.contains(event.target as Node)) {
        setShowAddMenu(false);
      }
    };

    if (showAddMenu) {
      document.addEventListener('mousedown', handleClickOutside);
    }

    return () => {
      document.removeEventListener('mousedown', handleClickOutside);
    };
  }, [showAddMenu]);

  const handleAddPlugin = (type: BuiltInEffectType) => {
    onAddPlugin(trackId, type);
    setShowAddMenu(false);
  };

  const selectedPlugin = plugins.find((p) => p.id === selectedPluginId);

  return (
    <div className="space-y-2">
      {/* Plugin Slots */}
      <AnimatePresence>
        {plugins.map((plugin, index) => (
          <PluginSlot
            key={plugin.id}
            plugin={plugin}
            index={index}
            onBypass={(pluginId, bypass) => onBypassPlugin(trackId, pluginId, bypass)}
            onRemove={(pluginId) => onRemovePlugin(trackId, pluginId)}
            onSelect={setSelectedPluginId}
            isSelected={selectedPluginId === plugin.id}
          />
        ))}
      </AnimatePresence>

      {/* Empty State */}
      {plugins.length === 0 && (
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          className="px-2 py-3 bg-background/30 border border-dashed border-border/30 rounded text-center"
        >
          <p className="text-xs text-muted-foreground">No plugins</p>
        </motion.div>
      )}

      {/* Add Plugin Button */}
      <div className="relative" ref={menuRef}>
        <motion.button
          whileHover={{ scale: 1.02 }}
          whileTap={{ scale: 0.98 }}
          onClick={() => setShowAddMenu(!showAddMenu)}
          className="w-full px-2 py-2 bg-accent/30 hover:bg-accent/50 border border-dashed border-border/50 rounded text-xs font-medium flex items-center justify-center gap-2 transition-colors"
        >
          <Plus className="h-3 w-3" />
          <span>Add Plugin</span>
        </motion.button>

        {/* Add Plugin Menu */}
        <AnimatePresence>
          {showAddMenu && (
            <motion.div
              initial={{ opacity: 0, y: -10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              transition={{ duration: 0.15 }}
              className="absolute z-50 top-full mt-1 w-full bg-card border border-border rounded-lg shadow-2xl overflow-hidden"
            >
              <div className="p-1">
                <div className="px-2 py-1.5 text-[10px] font-semibold text-muted-foreground uppercase tracking-wider">
                  Built-in Effects
                </div>
                {BUILTIN_EFFECTS.map((effect) => (
                  <motion.button
                    key={effect.type}
                    whileHover={{ backgroundColor: 'rgba(255,255,255,0.05)' }}
                    whileTap={{ scale: 0.98 }}
                    onClick={() => handleAddPlugin(effect.type)}
                    className="w-full px-2 py-2 rounded text-left transition-colors"
                  >
                    <div className="text-xs font-medium">{effect.name}</div>
                    <div className="text-[10px] text-muted-foreground">{effect.description}</div>
                  </motion.button>
                ))}
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </div>

      {/* Plugin Inspector */}
      <AnimatePresence>
        {selectedPlugin && (
          <PluginInspector
            plugin={selectedPlugin}
            onParameterChange={(pluginId, parameterId, value) =>
              onParameterChange(trackId, pluginId, parameterId, value)
            }
            onClose={() => setSelectedPluginId(null)}
          />
        )}
      </AnimatePresence>
    </div>
  );
}
