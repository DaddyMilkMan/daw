import { Plugin, PluginParameter, PluginState, PluginChain, BuiltInEffectType } from '../types/plugin';
import { EqualizerEffect } from './effects/EqualizerEffect';
import { CompressorEffect } from './effects/CompressorEffect';
import { ReverbEffect } from './effects/ReverbEffect';

// Base Plugin class
export abstract class BasePlugin implements Plugin {
  id: string;
  name: string;
  type: string;
  enabled: boolean;
  bypassed: boolean;

  protected audioContext: AudioContext;
  protected inputNode: GainNode;
  protected outputNode: GainNode;
  protected parameters: Map<string, PluginParameter>;

  constructor(audioContext: AudioContext, type: string, name: string) {
    this.id = `${type}-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
    this.name = name;
    this.type = type;
    this.enabled = true;
    this.bypassed = false;
    this.audioContext = audioContext;
    this.parameters = new Map();

    // Create input/output nodes
    this.inputNode = audioContext.createGain();
    this.outputNode = audioContext.createGain();

    // Initialize effect-specific nodes
    this.initializeNodes();
  }

  protected abstract initializeNodes(): void;

  connect(destination: AudioNode): void {
    this.outputNode.connect(destination);
  }

  disconnect(): void {
    this.outputNode.disconnect();
  }

  getParameters(): PluginParameter[] {
    return Array.from(this.parameters.values());
  }

  getParameter(id: string): PluginParameter | undefined {
    return this.parameters.get(id);
  }

  setParameter(id: string, value: number): void {
    const param = this.parameters.get(id);
    if (param) {
      param.value = Math.max(param.min, Math.min(param.max, value));
      this.updateParameter(id, param.value);
    }
  }

  protected abstract updateParameter(id: string, value: number): void;

  getState(): PluginState {
    const parameters: Record<string, number> = {};
    this.parameters.forEach((param, id) => {
      parameters[id] = param.value;
    });

    return {
      id: this.id,
      name: this.name,
      type: this.type,
      enabled: this.enabled,
      bypassed: this.bypassed,
      parameters,
    };
  }

  setState(state: PluginState): void {
    this.id = state.id;
    this.name = state.name;
    this.type = state.type;
    this.enabled = state.enabled;
    this.bypassed = state.bypassed;

    Object.entries(state.parameters).forEach(([id, value]) => {
      this.setParameter(id, value);
    });
  }

  dispose(): void {
    this.disconnect();
    this.inputNode.disconnect();
    this.outputNode.disconnect();
  }
}

// Plugin Chain Implementation
export class PluginChainImpl implements PluginChain {
  plugins: Plugin[];
  inputNode: GainNode;
  outputNode: GainNode;

  private audioContext: AudioContext;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.plugins = [];
    this.inputNode = audioContext.createGain();
    this.outputNode = audioContext.createGain();

    // Initially connect input directly to output
    this.reconnectChain();
  }

  addPlugin(plugin: Plugin, index?: number): void {
    const insertIndex = index !== undefined ? index : this.plugins.length;
    this.plugins.splice(insertIndex, 0, plugin);
    this.reconnectChain();
  }

  removePlugin(pluginId: string): void {
    const index = this.plugins.findIndex((p) => p.id === pluginId);
    if (index !== -1) {
      const plugin = this.plugins[index];
      plugin.dispose();
      this.plugins.splice(index, 1);
      this.reconnectChain();
    }
  }

  movePlugin(pluginId: string, newIndex: number): void {
    const currentIndex = this.plugins.findIndex((p) => p.id === pluginId);
    if (currentIndex !== -1) {
      const [plugin] = this.plugins.splice(currentIndex, 1);
      this.plugins.splice(newIndex, 0, plugin);
      this.reconnectChain();
    }
  }

  bypassPlugin(pluginId: string, bypass: boolean): void {
    const plugin = this.plugins.find((p) => p.id === pluginId);
    if (plugin) {
      plugin.bypassed = bypass;
      this.reconnectChain();
    }
  }

  clearPlugins(): void {
    this.plugins.forEach((plugin) => plugin.dispose());
    this.plugins = [];
    this.reconnectChain();
  }

  private reconnectChain(): void {
    // Disconnect everything first
    this.inputNode.disconnect();
    this.plugins.forEach((plugin) => plugin.disconnect());

    if (this.plugins.length === 0) {
      // No plugins, direct connection
      this.inputNode.connect(this.outputNode);
    } else {
      // Connect through plugin chain
      let currentNode: AudioNode = this.inputNode;

      this.plugins.forEach((plugin) => {
        if (plugin.bypassed) {
          // Skip bypassed plugins - they stay disconnected
          return;
        }

        // Connect current node to plugin input
        currentNode.connect((plugin as BasePlugin).inputNode);

        // Update current node to plugin output
        currentNode = (plugin as BasePlugin).outputNode;
      });

      // Connect last node to output
      currentNode.connect(this.outputNode);
    }
  }

  getState(): PluginState[] {
    return this.plugins.map((plugin) => plugin.getState());
  }

  setState(states: PluginState[]): void {
    this.clearPlugins();

    // Recreate plugins from state
    states.forEach((state) => {
      const plugin = createPluginFromState(this.audioContext, state);
      if (plugin) {
        this.addPlugin(plugin);
      }
    });
  }

  dispose(): void {
    this.clearPlugins();
    this.inputNode.disconnect();
    this.outputNode.disconnect();
  }
}

// Factory function to create plugins from saved state
function createPluginFromState(audioContext: AudioContext, state: PluginState): Plugin | null {
  let plugin: Plugin | null = null;

  switch (state.type) {
    case 'equalizer':
      plugin = new EqualizerEffect(audioContext);
      break;
    case 'compressor':
      plugin = new CompressorEffect(audioContext);
      break;
    case 'reverb':
      plugin = new ReverbEffect(audioContext);
      break;
    default:
      console.warn(`Unknown plugin type: ${state.type}`);
      return null;
  }

  if (plugin) {
    plugin.setState(state);
  }

  return plugin;
}

// Factory function to create new plugin instances by type
export function createPlugin(audioContext: AudioContext, type: BuiltInEffectType): Plugin {
  switch (type) {
    case 'equalizer':
      return new EqualizerEffect(audioContext);
    case 'compressor':
      return new CompressorEffect(audioContext);
    case 'reverb':
      return new ReverbEffect(audioContext);
    default:
      throw new Error(`Unknown plugin type: ${type}`);
  }
}

// Plugin Host - manages the global audio context and plugin creation
export class PluginHost {
  private static instance: PluginHost;
  private audioContext: AudioContext | null = null;

  private constructor() {}

  static getInstance(): PluginHost {
    if (!PluginHost.instance) {
      PluginHost.instance = new PluginHost();
    }
    return PluginHost.instance;
  }

  initialize(): void {
    if (!this.audioContext) {
      this.audioContext = new AudioContext();
    }
  }

  getAudioContext(): AudioContext {
    if (!this.audioContext) {
      this.initialize();
    }
    return this.audioContext!;
  }

  createPluginChain(): PluginChain {
    return new PluginChainImpl(this.getAudioContext());
  }

  suspend(): void {
    this.audioContext?.suspend();
  }

  resume(): void {
    this.audioContext?.resume();
  }

  close(): void {
    this.audioContext?.close();
    this.audioContext = null;
  }
}
