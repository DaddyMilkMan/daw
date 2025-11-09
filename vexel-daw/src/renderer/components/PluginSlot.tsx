import { motion } from 'framer-motion';
import { Power, X, GripVertical, Settings } from 'lucide-react';
import { PluginState } from '../types/plugin';

interface PluginSlotProps {
  plugin: PluginState;
  index: number;
  onBypass: (pluginId: string, bypass: boolean) => void;
  onRemove: (pluginId: string) => void;
  onSelect: (pluginId: string) => void;
  onDragStart?: (index: number) => void;
  onDragEnd?: () => void;
  isSelected?: boolean;
}

export default function PluginSlot({
  plugin,
  index,
  onBypass,
  onRemove,
  onSelect,
  onDragStart,
  onDragEnd,
  isSelected = false,
}: PluginSlotProps) {
  return (
    <motion.div
      layout
      initial={{ opacity: 0, y: -10 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: -10 }}
      transition={{ duration: 0.2 }}
      draggable
      onDragStart={() => onDragStart?.(index)}
      onDragEnd={onDragEnd}
      className={`group relative bg-gradient-to-r from-accent/40 to-accent/20 border rounded cursor-pointer transition-all ${
        isSelected
          ? 'border-primary shadow-lg shadow-primary/20'
          : plugin.bypassed
          ? 'border-border/30 opacity-60'
          : 'border-border/50 hover:border-primary/50'
      }`}
      onClick={() => onSelect(plugin.id)}
    >
      {/* Drag Handle */}
      <div className="absolute left-1 top-1/2 -translate-y-1/2 opacity-0 group-hover:opacity-50 transition-opacity cursor-grab active:cursor-grabbing">
        <GripVertical className="h-4 w-4" />
      </div>

      <div className="flex items-center gap-2 px-8 py-2">
        {/* Plugin Icon/Type Indicator */}
        <div
          className={`flex-shrink-0 w-2 h-2 rounded-full ${
            plugin.bypassed ? 'bg-muted-foreground' : 'bg-primary'
          }`}
        />

        {/* Plugin Name */}
        <div className="flex-1 min-w-0">
          <div className="text-xs font-medium truncate">{plugin.name}</div>
          <div className="text-[10px] text-muted-foreground truncate">{plugin.type}</div>
        </div>

        {/* Controls */}
        <div className="flex items-center gap-1">
          {/* Settings Button */}
          <motion.button
            whileHover={{ scale: 1.1 }}
            whileTap={{ scale: 0.9 }}
            onClick={(e) => {
              e.stopPropagation();
              onSelect(plugin.id);
            }}
            className="p-1 rounded hover:bg-accent transition-colors"
            title="Settings"
          >
            <Settings className="h-3 w-3" />
          </motion.button>

          {/* Bypass Button */}
          <motion.button
            whileHover={{ scale: 1.1 }}
            whileTap={{ scale: 0.9 }}
            onClick={(e) => {
              e.stopPropagation();
              onBypass(plugin.id, !plugin.bypassed);
            }}
            className={`p-1 rounded transition-colors ${
              plugin.bypassed ? 'text-muted-foreground' : 'text-primary hover:bg-accent'
            }`}
            title={plugin.bypassed ? 'Bypassed' : 'Active'}
          >
            <Power className="h-3 w-3" />
          </motion.button>

          {/* Remove Button */}
          <motion.button
            whileHover={{ scale: 1.1 }}
            whileTap={{ scale: 0.9 }}
            onClick={(e) => {
              e.stopPropagation();
              onRemove(plugin.id);
            }}
            className="p-1 rounded hover:bg-destructive/20 text-muted-foreground hover:text-destructive transition-colors"
            title="Remove"
          >
            <X className="h-3 w-3" />
          </motion.button>
        </div>
      </div>
    </motion.div>
  );
}
