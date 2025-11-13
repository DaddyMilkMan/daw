/**
 * Plugin Host Service
 * Implements Web Audio Modules (WAM) 2.0 Standard
 *
 * Features:
 * - Load WAM plugins from URLs
 * - Manage plugin instances
 * - Parameter automation
 * - State persistence
 * - Preset management
 *
 * Based on:
 * - WAM 2.0 Standard (https://github.com/webaudiomodules/api)
 * - WebAssembly audio processing
 * - AudioWorklet integration
 */

import { useAudioStore } from '../store/audioStore';
import {
  PluginDescriptor,
  PluginInstance,
  WAMNode,
  WAMGroup,
  WAMParameterValues,
  PluginPreset,
  PluginScanResult,
} from '../types/plugin';

export class PluginHostService {
  private static instance: PluginHostService | null = null;
  private loadedPlugins: Map<string, WAMGroup> = new Map();
  private activeInstances: Map<string, PluginInstance> = new Map();

  private constructor() {}

  static getInstance(): PluginHostService {
    if (!PluginHostService.instance) {
      PluginHostService.instance = new PluginHostService();
    }
    return PluginHostService.instance;
  }

  /**
   * Scan and load a plugin from URL
   */
  async scanPlugin(url: string): Promise<PluginScanResult> {
    try {
      console.log(`Scanning plugin from: ${url}`);

      // Dynamic import of WAM plugin
      const module = await import(/* webpackIgnore: true */ url);

      if (!module.default || typeof module.default.createInstance !== 'function') {
        throw new Error('Invalid WAM plugin: missing createInstance method');
      }

      const wamGroup: WAMGroup = module.default;

      // Get descriptor
      const descriptor = await wamGroup.getDescriptor();

      // Create plugin descriptor
      const pluginDescriptor: PluginDescriptor = {
        id: `${descriptor.vendor}-${descriptor.name}-${descriptor.version}`.replace(/\s+/g, '-').toLowerCase(),
        name: descriptor.name,
        vendor: descriptor.vendor,
        version: descriptor.version,
        category: this.inferCategory(descriptor),
        isInstrument: descriptor.isInstrument,
        thumbnail: descriptor.thumbnail,
        url,
        wamGroup,
      };

      // Cache the WAM group
      this.loadedPlugins.set(pluginDescriptor.id, wamGroup);

      console.log(`Successfully scanned plugin: ${pluginDescriptor.name}`);

      return {
        success: true,
        descriptor: pluginDescriptor,
      };
    } catch (error) {
      console.error('Failed to scan plugin:', error);
      return {
        success: false,
        error: error instanceof Error ? error.message : 'Unknown error',
      };
    }
  }

  /**
   * Create plugin instance on track
   */
  async createInstance(
    pluginId: string,
    trackId: string,
    position: number
  ): Promise<PluginInstance | null> {
    try {
      const store = useAudioStore.getState();
      const context = store.audioContext.context;

      if (!context) {
        throw new Error('AudioContext not initialized');
      }

      const wamGroup = this.loadedPlugins.get(pluginId);
      if (!wamGroup) {
        throw new Error(`Plugin not loaded: ${pluginId}`);
      }

      // Create WAM node
      const wamNode = await wamGroup.createWAM(context);

      // Get default parameter values
      const parameterInfo = await wamNode.getParameterInfo();
      const parameterValues: WAMParameterValues = {};

      for (const [id, info] of Object.entries(parameterInfo)) {
        parameterValues[id] = info.defaultValue;
      }

      // Create plugin instance
      const instance: PluginInstance = {
        id: `instance-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
        pluginId,
        trackId,
        position,
        enabled: true,
        wamNode,
        parameterValues,
      };

      this.activeInstances.set(instance.id, instance);

      console.log(`Created plugin instance: ${instance.id} on track ${trackId}`);

      return instance;
    } catch (error) {
      console.error('Failed to create plugin instance:', error);
      return null;
    }
  }

  /**
   * Insert plugin into audio chain
   */
  insertIntoChain(
    instance: PluginInstance,
    inputNode: AudioNode,
    outputNode: AudioNode
  ): void {
    if (!instance.wamNode) {
      throw new Error('WAM node not initialized');
    }

    // Connect: inputNode -> wamNode.audioNode -> outputNode
    // Safely disconnect (Web Audio API throws InvalidAccessError if no connections exist)
    try {
      inputNode.disconnect();
    } catch (e) {
      // No connections exist, which is fine
    }
    inputNode.connect(instance.wamNode.audioNode);
    instance.wamNode.audioNode.connect(outputNode);

    console.log(`Inserted plugin ${instance.id} into audio chain`);
  }

  /**
   * Remove plugin from audio chain
   */
  removeFromChain(instance: PluginInstance, reconnect: AudioNode[]): void {
    if (!instance.wamNode) {
      return;
    }

    // Disconnect plugin (safely handle case where no connections exist)
    try {
      instance.wamNode.audioNode.disconnect();
    } catch (e) {
      // No connections exist, which is fine
    }

    // Reconnect bypassed chain
    if (reconnect.length >= 2) {
      reconnect[0].connect(reconnect[1]);
    }

    console.log(`Removed plugin ${instance.id} from audio chain`);
  }

  /**
   * Toggle plugin bypass
   */
  setEnabled(instanceId: string, enabled: boolean): void {
    const instance = this.activeInstances.get(instanceId);
    if (!instance) {
      return;
    }

    instance.enabled = enabled;

    // Note: Actual bypass requires reconnecting audio chain
    // This should be handled by the mixing engine

    console.log(`Plugin ${instanceId} ${enabled ? 'enabled' : 'bypassed'}`);
  }

  /**
   * Set plugin parameter
   */
  async setParameter(instanceId: string, parameterId: string, value: number): Promise<void> {
    const instance = this.activeInstances.get(instanceId);
    if (!instance || !instance.wamNode) {
      return;
    }

    instance.parameterValues[parameterId] = value;

    await instance.wamNode.setParameterValues({
      [parameterId]: value,
    });

    console.log(`Set parameter ${parameterId} = ${value} on ${instanceId}`);
  }

  /**
   * Get plugin parameter
   */
  async getParameter(instanceId: string, parameterId: string): Promise<number | undefined> {
    const instance = this.activeInstances.get(instanceId);
    if (!instance || !instance.wamNode) {
      return undefined;
    }

    const values = await instance.wamNode.getParameterValues();
    return values[parameterId];
  }

  /**
   * Get all parameters
   */
  async getAllParameters(instanceId: string): Promise<WAMParameterValues | null> {
    const instance = this.activeInstances.get(instanceId);
    if (!instance || !instance.wamNode) {
      return null;
    }

    return await instance.wamNode.getParameterValues();
  }

  /**
   * Save plugin state
   */
  async saveState(instanceId: string): Promise<any> {
    const instance = this.activeInstances.get(instanceId);
    if (!instance || !instance.wamNode) {
      return null;
    }

    return await instance.wamNode.getState();
  }

  /**
   * Load plugin state
   */
  async loadState(instanceId: string, state: any): Promise<void> {
    const instance = this.activeInstances.get(instanceId);
    if (!instance || !instance.wamNode) {
      return;
    }

    await instance.wamNode.setState(state);

    // Update cached parameter values
    instance.parameterValues = await instance.wamNode.getParameterValues();

    console.log(`Loaded state for plugin ${instanceId}`);
  }

  /**
   * Save preset
   */
  async savePreset(instanceId: string, presetName: string): Promise<PluginPreset | null> {
    const instance = this.activeInstances.get(instanceId);
    if (!instance || !instance.wamNode) {
      return null;
    }

    const parameterValues = await instance.wamNode.getParameterValues();

    const preset: PluginPreset = {
      id: `preset-${Date.now()}`,
      name: presetName,
      pluginId: instance.pluginId,
      parameterValues,
      createdAt: Date.now(),
    };

    console.log(`Saved preset: ${presetName} for plugin ${instance.pluginId}`);

    return preset;
  }

  /**
   * Load preset
   */
  async loadPreset(instanceId: string, preset: PluginPreset): Promise<void> {
    const instance = this.activeInstances.get(instanceId);
    if (!instance || !instance.wamNode) {
      return;
    }

    await instance.wamNode.setParameterValues(preset.parameterValues);
    instance.parameterValues = preset.parameterValues;
    instance.presetName = preset.name;

    console.log(`Loaded preset: ${preset.name} for plugin ${instanceId}`);
  }

  /**
   * Destroy plugin instance
   */
  destroyInstance(instanceId: string): void {
    const instance = this.activeInstances.get(instanceId);
    if (!instance) {
      return;
    }

    if (instance.wamNode) {
      instance.wamNode.destroy();
    }

    this.activeInstances.delete(instanceId);

    console.log(`Destroyed plugin instance: ${instanceId}`);
  }

  /**
   * Get instance
   */
  getInstance(instanceId: string): PluginInstance | undefined {
    return this.activeInstances.get(instanceId);
  }

  /**
   * Get all instances for track
   */
  getTrackInstances(trackId: string): PluginInstance[] {
    return Array.from(this.activeInstances.values())
      .filter((instance) => instance.trackId === trackId)
      .sort((a, b) => a.position - b.position);
  }

  /**
   * Clear all instances
   */
  clearAll(): void {
    for (const instance of this.activeInstances.values()) {
      if (instance.wamNode) {
        instance.wamNode.destroy();
      }
    }

    this.activeInstances.clear();
    console.log('Cleared all plugin instances');
  }

  /**
   * Infer plugin category from descriptor
   */
  private inferCategory(descriptor: any): string {
    const name = descriptor.name.toLowerCase();
    const keywords = (descriptor.keywords || []).map((k: string) => k.toLowerCase());

    if (descriptor.isInstrument) return 'instrument';
    if (name.includes('eq') || keywords.includes('eq')) return 'eq';
    if (
      name.includes('comp') ||
      name.includes('limit') ||
      keywords.includes('dynamics')
    )
      return 'dynamics';
    if (name.includes('reverb') || keywords.includes('reverb')) return 'reverb';
    if (name.includes('delay') || keywords.includes('delay')) return 'delay';
    if (
      name.includes('chorus') ||
      name.includes('flanger') ||
      name.includes('phaser')
    )
      return 'modulation';
    if (name.includes('dist') || name.includes('overdrive')) return 'distortion';
    if (name.includes('filter')) return 'filter';

    return 'other';
  }
}

export const pluginHost = PluginHostService.getInstance();
