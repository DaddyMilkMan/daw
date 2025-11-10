/**
 * Plugin System Types
 * Based on Web Audio Modules (WAM) 2.0 Standard
 *
 * WAM provides a standardized interface for WebAssembly audio plugins
 * Compatible with VST/AU plugins compiled to WASM
 */

export interface WAMNode extends AudioNode {
  readonly instanceId: string;
  readonly moduleId: string;
  readonly audioNode: AudioNode;

  // Parameters
  getParameterInfo(): Promise<WAMParameterInfoMap>;
  getParameterValues(): Promise<WAMParameterValues>;
  setParameterValues(values: WAMParameterValues): Promise<void>;

  // State
  getState(): Promise<any>;
  setState(state: any): Promise<void>;

  // Events
  addEventListener(type: string, listener: EventListener): void;
  removeEventListener(type: string, listener: EventListener): void;

  // Destruction
  destroy(): void;
}

export interface WAMParameterInfo {
  id: string;
  label: string;
  type: 'float' | 'int' | 'boolean' | 'choice';
  defaultValue: number;
  minValue?: number;
  maxValue?: number;
  discreteStep?: number;
  choices?: string[];
  units?: string;
}

export type WAMParameterInfoMap = { [id: string]: WAMParameterInfo };
export type WAMParameterValues = { [id: string]: number };

export interface WAMDescriptor {
  name: string;
  vendor: string;
  version: string;
  sdkVersion: string;
  thumbnail?: string;
  keywords?: string[];
  isInstrument: boolean;
  website?: string;
}

export interface WAMGroup {
  createWAM(audioContext: AudioContext): Promise<WAMNode>;
  getDescriptor(): Promise<WAMDescriptor>;
}

/**
 * Plugin Instance (loaded into track)
 */
export interface PluginInstance {
  id: string;
  pluginId: string; // Reference to PluginDescriptor
  trackId: string;
  position: number; // Insert position in chain
  enabled: boolean;
  wamNode: WAMNode | null;
  parameterValues: WAMParameterValues;
  presetName?: string;
}

/**
 * Plugin Descriptor (available plugin)
 */
export interface PluginDescriptor {
  id: string;
  name: string;
  vendor: string;
  version: string;
  category: PluginCategory;
  isInstrument: boolean;
  thumbnail?: string;
  url: string; // URL to load plugin from
  wamGroup?: WAMGroup;
}

export type PluginCategory =
  | 'eq'
  | 'dynamics'
  | 'reverb'
  | 'delay'
  | 'modulation'
  | 'distortion'
  | 'filter'
  | 'instrument'
  | 'utility'
  | 'other';

/**
 * Plugin Preset
 */
export interface PluginPreset {
  id: string;
  name: string;
  pluginId: string;
  parameterValues: WAMParameterValues;
  createdAt: number;
}

/**
 * Plugin Scan Result
 */
export interface PluginScanResult {
  success: boolean;
  descriptor?: PluginDescriptor;
  error?: string;
}
