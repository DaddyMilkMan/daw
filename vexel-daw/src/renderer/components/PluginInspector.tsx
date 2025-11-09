import { motion } from 'framer-motion';
import { PluginState, PluginParameter } from '../types/plugin';
import { PluginHost, createPlugin } from '../services/PluginHost';
import { useState, useEffect } from 'react';
import { X, RotateCcw } from 'lucide-react';

interface PluginInspectorProps {
  plugin: PluginState;
  onParameterChange: (pluginId: string, parameterId: string, value: number) => void;
  onClose: () => void;
}

export default function PluginInspector({
  plugin,
  onParameterChange,
  onClose,
}: PluginInspectorProps) {
  const [parameters, setParameters] = useState<PluginParameter[]>([]);

  useEffect(() => {
    // Create a temporary plugin instance to get parameter definitions
    const pluginHost = PluginHost.getInstance();
    const audioContext = pluginHost.getAudioContext();
    const tempPlugin = createPlugin(audioContext, plugin.type as any);

    if (tempPlugin) {
      // Get parameter definitions
      const params = tempPlugin.getParameters();

      // Update values from saved state
      params.forEach(param => {
        if (plugin.parameters[param.id] !== undefined) {
          param.value = plugin.parameters[param.id];
        }
      });

      setParameters(params);
      tempPlugin.dispose();
    }
  }, [plugin]);

  const handleParameterChange = (param: PluginParameter, value: number) => {
    // Update local state
    setParameters(prev =>
      prev.map(p => (p.id === param.id ? { ...p, value } : p))
    );

    // Notify parent
    onParameterChange(plugin.id, param.id, value);
  };

  const resetParameter = (param: PluginParameter) => {
    handleParameterChange(param, param.default);
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 10 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: 10 }}
      className="bg-card border border-border rounded-lg p-4 space-y-4"
    >
      {/* Header */}
      <div className="flex items-center justify-between border-b border-border pb-3">
        <div>
          <h3 className="text-sm font-bold">{plugin.name}</h3>
          <p className="text-xs text-muted-foreground capitalize">{plugin.type}</p>
        </div>
        <motion.button
          whileHover={{ scale: 1.1 }}
          whileTap={{ scale: 0.9 }}
          onClick={onClose}
          className="p-1 rounded hover:bg-accent transition-colors"
        >
          <X className="h-4 w-4" />
        </motion.button>
      </div>

      {/* Parameters */}
      <div className="space-y-3 max-h-96 overflow-y-auto">
        {parameters.map((param) => (
          <ParameterControl
            key={param.id}
            parameter={param}
            onChange={(value) => handleParameterChange(param, value)}
            onReset={() => resetParameter(param)}
          />
        ))}
      </div>
    </motion.div>
  );
}

interface ParameterControlProps {
  parameter: PluginParameter;
  onChange: (value: number) => void;
  onReset: () => void;
}

function ParameterControl({ parameter, onChange, onReset }: ParameterControlProps) {
  const [isDragging, setIsDragging] = useState(false);

  const formatValue = (value: number): string => {
    // Format based on parameter unit
    if (parameter.unit === 'dB' || parameter.unit === 'Hz') {
      return value.toFixed(1);
    } else if (parameter.unit === 's' || parameter.unit === 'ms') {
      return value.toFixed(3);
    } else if (parameter.unit === '%') {
      return Math.round(value).toString();
    } else {
      return value.toFixed(2);
    }
  };

  return (
    <div className="space-y-1.5">
      {/* Label and Value */}
      <div className="flex items-center justify-between">
        <label className="text-xs font-medium text-muted-foreground">
          {parameter.name}
        </label>
        <div className="flex items-center gap-2">
          <span className="text-xs font-mono font-medium">
            {formatValue(parameter.value)}
            {parameter.unit && <span className="text-muted-foreground ml-0.5">{parameter.unit}</span>}
          </span>
          <motion.button
            whileHover={{ scale: 1.1 }}
            whileTap={{ scale: 0.9 }}
            onClick={onReset}
            className="p-0.5 rounded hover:bg-accent transition-colors opacity-0 group-hover:opacity-100"
            title="Reset to default"
          >
            <RotateCcw className="h-3 w-3 text-muted-foreground" />
          </motion.button>
        </div>
      </div>

      {/* Slider */}
      <div className="relative group">
        <input
          type="range"
          min={parameter.min}
          max={parameter.max}
          step={parameter.step || 0.01}
          value={parameter.value}
          onChange={(e) => onChange(parseFloat(e.target.value))}
          onMouseDown={() => setIsDragging(true)}
          onMouseUp={() => setIsDragging(false)}
          className="w-full h-2 bg-accent/30 rounded-full appearance-none cursor-pointer [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-3 [&::-webkit-slider-thumb]:h-3 [&::-webkit-slider-thumb]:rounded-full [&::-webkit-slider-thumb]:bg-primary [&::-webkit-slider-thumb]:cursor-pointer [&::-webkit-slider-thumb]:shadow-lg [&::-webkit-slider-thumb]:transition-transform hover:[&::-webkit-slider-thumb]:scale-110"
        />

        {/* Progress Fill */}
        <div
          className="absolute top-0 left-0 h-2 bg-primary/30 rounded-full pointer-events-none"
          style={{
            width: `${((parameter.value - parameter.min) / (parameter.max - parameter.min)) * 100}%`,
          }}
        />

        {/* Center Mark (for parameters that can go negative) */}
        {parameter.min < 0 && parameter.max > 0 && (
          <div
            className="absolute top-1/2 w-0.5 h-3 bg-foreground/20 pointer-events-none"
            style={{
              left: `${(Math.abs(parameter.min) / (parameter.max - parameter.min)) * 100}%`,
              transform: 'translate(-50%, -50%)',
            }}
          />
        )}
      </div>

      {/* Min/Max Labels */}
      <div className="flex justify-between text-[10px] text-muted-foreground">
        <span>{formatValue(parameter.min)}</span>
        <span>{formatValue(parameter.max)}</span>
      </div>
    </div>
  );
}
