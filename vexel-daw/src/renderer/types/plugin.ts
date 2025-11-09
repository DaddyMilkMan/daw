// Plugin System Types

export interface PluginParameter {
  id: string;
  name: string;
  value: number;
  min: number;
  max: number;
  default: number;
  unit?: string;
  step?: number;
}

export interface PluginState {
  id: string;
  name: string;
  type: string;
  enabled: boolean;
  bypassed: boolean;
  parameters: Record<string, number>;
}

export interface Plugin {
  id: string;
  name: string;
  type: string;
  enabled: boolean;
  bypassed: boolean;

  // Audio processing
  connect(destination: AudioNode): void;
  disconnect(): void;

  // Parameter management
  getParameters(): PluginParameter[];
  getParameter(id: string): PluginParameter | undefined;
  setParameter(id: string, value: number): void;

  // State management
  getState(): PluginState;
  setState(state: PluginState): void;

  // Lifecycle
  dispose(): void;
}

export interface PluginChain {
  plugins: Plugin[];
  inputNode: GainNode;
  outputNode: GainNode;

  addPlugin(plugin: Plugin, index?: number): void;
  removePlugin(pluginId: string): void;
  movePlugin(pluginId: string, newIndex: number): void;
  bypassPlugin(pluginId: string, bypass: boolean): void;
  clearPlugins(): void;

  getState(): PluginState[];
  setState(states: PluginState[]): void;

  dispose(): void;
}

// Built-in effect types
export type BuiltInEffectType = 'equalizer' | 'compressor' | 'reverb';

export interface BuiltInEffect {
  type: BuiltInEffectType;
  name: string;
  description: string;
}

export const BUILTIN_EFFECTS: BuiltInEffect[] = [
  {
    type: 'equalizer',
    name: 'Equalizer',
    description: '3-band parametric EQ',
  },
  {
    type: 'compressor',
    name: 'Compressor',
    description: 'Dynamic range compressor',
  },
  {
    type: 'reverb',
    name: 'Reverb',
    description: 'Convolution reverb',
  },
];
