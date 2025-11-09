// Browser types
export interface BrowserItem {
  id: string;
  name: string;
  type: 'instrument' | 'effect' | 'drum' | 'sample' | 'midi';
  category: string;
  tags: string[];
  path: string;
  duration?: number; // in seconds, for audio files
  tempo?: number; // BPM for loops
  key?: string; // Musical key for samples
  isFavorite: boolean;
  dateAdded: Date;
  lastUsed?: Date;
  fileSize?: number; // in bytes
  waveform?: number[]; // Simplified waveform data for visualization
}

export interface BrowserCollection {
  id: string;
  name: string;
  items: string[]; // Array of item IDs
  color: string;
  createdAt: Date;
}

export interface BrowserState {
  items: BrowserItem[];
  collections: BrowserCollection[];
  selectedCategory: string | null;
  searchQuery: string;
  selectedTags: string[];
  sortBy: 'name' | 'date-added' | 'last-used' | 'type';
  sortOrder: 'asc' | 'desc';
  viewMode: 'list' | 'grid';
  showFavoritesOnly: boolean;
  previewingItem: string | null;
  isPreviewPlaying: boolean;
}

export const BROWSER_CATEGORIES = [
  { id: 'instruments', label: 'Instruments', icon: 'Music' },
  { id: 'effects', label: 'Effects', icon: 'Zap' },
  { id: 'drums', label: 'Drums', icon: 'Drum' },
  { id: 'samples', label: 'Samples', icon: 'Folder' },
  { id: 'midi', label: 'MIDI', icon: 'Music' },
] as const;

// Sample data for demonstration
export const SAMPLE_BROWSER_ITEMS: BrowserItem[] = [
  {
    id: '1',
    name: 'Deep Kick',
    type: 'drum',
    category: 'drums',
    tags: ['kick', 'bass', 'deep', '808'],
    path: '/samples/drums/kick_deep.wav',
    duration: 1.2,
    tempo: 128,
    isFavorite: true,
    dateAdded: new Date('2024-01-15'),
    lastUsed: new Date('2024-01-20'),
    waveform: [0.1, 0.8, 0.9, 0.7, 0.4, 0.2, 0.1, 0.05, 0.02, 0.01],
  },
  {
    id: '2',
    name: 'Serum - Lead Synth',
    type: 'instrument',
    category: 'instruments',
    tags: ['synth', 'lead', 'edm', 'serum'],
    path: '/plugins/serum.vst',
    isFavorite: true,
    dateAdded: new Date('2024-01-10'),
    lastUsed: new Date('2024-01-22'),
  },
  {
    id: '3',
    name: 'Reverb Pro',
    type: 'effect',
    category: 'effects',
    tags: ['reverb', 'spatial', 'hall'],
    path: '/plugins/reverb_pro.vst',
    isFavorite: false,
    dateAdded: new Date('2024-01-12'),
  },
  {
    id: '4',
    name: 'Crisp Snare',
    type: 'drum',
    category: 'drums',
    tags: ['snare', 'crisp', 'acoustic'],
    path: '/samples/drums/snare_crisp.wav',
    duration: 0.8,
    tempo: 120,
    isFavorite: false,
    dateAdded: new Date('2024-01-18'),
    waveform: [0.05, 0.3, 0.95, 0.8, 0.5, 0.3, 0.2, 0.1, 0.05, 0.02],
  },
  {
    id: '5',
    name: 'Ambient Pad',
    type: 'sample',
    category: 'samples',
    tags: ['pad', 'ambient', 'atmospheric', 'Cm'],
    path: '/samples/pads/ambient_pad.wav',
    duration: 4.0,
    tempo: 85,
    key: 'Cm',
    isFavorite: true,
    dateAdded: new Date('2024-01-14'),
    waveform: [0.2, 0.3, 0.4, 0.5, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1],
  },
  {
    id: '6',
    name: 'Bass Loop 128',
    type: 'sample',
    category: 'samples',
    tags: ['bass', 'loop', 'groove', 'Em'],
    path: '/samples/bass/bass_loop_128.wav',
    duration: 2.0,
    tempo: 128,
    key: 'Em',
    isFavorite: false,
    dateAdded: new Date('2024-01-16'),
    waveform: [0.3, 0.6, 0.8, 0.7, 0.5, 0.6, 0.8, 0.7, 0.5, 0.3],
  },
  {
    id: '7',
    name: 'Delay Time',
    type: 'effect',
    category: 'effects',
    tags: ['delay', 'echo', 'time-based'],
    path: '/plugins/delay_time.vst',
    isFavorite: false,
    dateAdded: new Date('2024-01-13'),
  },
  {
    id: '8',
    name: 'Hi-Hat Closed',
    type: 'drum',
    category: 'drums',
    tags: ['hi-hat', 'closed', 'crisp'],
    path: '/samples/drums/hihat_closed.wav',
    duration: 0.3,
    tempo: 140,
    isFavorite: true,
    dateAdded: new Date('2024-01-17'),
    waveform: [0.6, 0.8, 0.5, 0.3, 0.1, 0.05, 0.02, 0.01],
  },
  {
    id: '9',
    name: 'Piano Grand',
    type: 'instrument',
    category: 'instruments',
    tags: ['piano', 'acoustic', 'keys'],
    path: '/plugins/piano_grand.vst',
    isFavorite: false,
    dateAdded: new Date('2024-01-11'),
    lastUsed: new Date('2024-01-19'),
  },
  {
    id: '10',
    name: 'Vocal Chop',
    type: 'sample',
    category: 'samples',
    tags: ['vocal', 'chop', 'processed', 'Am'],
    path: '/samples/vocals/vocal_chop.wav',
    duration: 1.5,
    tempo: 124,
    key: 'Am',
    isFavorite: true,
    dateAdded: new Date('2024-01-19'),
    waveform: [0.4, 0.7, 0.6, 0.8, 0.5, 0.3, 0.2, 0.4, 0.6, 0.3],
  },
];
