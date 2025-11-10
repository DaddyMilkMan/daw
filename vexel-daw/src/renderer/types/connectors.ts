/**
 * Cloud Storage Connectors
 * Types and interfaces for multi-provider cloud storage integration
 *
 * Supported Providers:
 * - Google Drive
 * - Microsoft OneDrive
 * - Dropbox
 * - Box
 * - pCloud
 * - MEGA
 * - AWS S3
 * - iCloud Drive (limited - third-party)
 */

export type ConnectorProvider =
  | 'google-drive'
  | 'onedrive'
  | 'dropbox'
  | 'box'
  | 'pcloud'
  | 'mega'
  | 's3'
  | 'icloud';

export interface ConnectorConfig {
  id: string;
  name: string;
  provider: ConnectorProvider;
  enabled: boolean;
  credentials: ConnectorCredentials;
  settings: ConnectorSettings;
  createdAt: number;
  lastSync?: number;
}

export interface ConnectorCredentials {
  // OAuth 2.0 (Google Drive, OneDrive, Dropbox, Box)
  accessToken?: string;
  refreshToken?: string;
  expiresAt?: number;
  clientId?: string;
  clientSecret?: string;

  // Username/Password (MEGA, pCloud)
  username?: string;
  password?: string;
  email?: string;

  // API Keys (S3, pCloud)
  accessKeyId?: string;
  secretAccessKey?: string;
  apiKey?: string;

  // Token-based (pCloud, MEGA)
  authToken?: string;

  // Region (S3)
  region?: string;
  bucket?: string;

  // Custom endpoint
  endpoint?: string;
}

export interface ConnectorSettings {
  autoSync: boolean;
  syncInterval: number; // milliseconds
  defaultFolder: string;
  uploadQuality: 'original' | 'compressed';
  conflictResolution: 'local' | 'remote' | 'ask';
  cacheEnabled: boolean;
  maxCacheSize: number; // MB
}

export interface ConnectorStatus {
  id: string;
  connected: boolean;
  syncing: boolean;
  lastError?: string;
  storageUsed?: number; // bytes
  storageTotal?: number; // bytes
  lastSync?: number;
}

export interface ConnectorFile {
  id: string;
  name: string;
  path: string;
  size: number;
  mimeType: string;
  modifiedTime: number;
  isFolder: boolean;
  thumbnailUrl?: string;
  downloadUrl?: string;
}

export interface ConnectorUploadOptions {
  fileName: string;
  filePath?: string;
  blob?: Blob;
  mimeType: string;
  folder?: string;
  onProgress?: (progress: number) => void;
}

export interface ConnectorDownloadOptions {
  fileId: string;
  destination?: string;
  onProgress?: (progress: number) => void;
}

/**
 * Base connector interface that all providers must implement
 */
export interface ICloudConnector {
  readonly provider: ConnectorProvider;
  readonly config: ConnectorConfig;

  // Authentication
  authenticate(): Promise<boolean>;
  refreshAuth(): Promise<boolean>;
  disconnect(): Promise<void>;
  isAuthenticated(): boolean;

  // File operations
  uploadFile(options: ConnectorUploadOptions): Promise<ConnectorFile>;
  downloadFile(options: ConnectorDownloadOptions): Promise<Blob>;
  deleteFile(fileId: string): Promise<void>;
  listFiles(folderId?: string): Promise<ConnectorFile[]>;
  searchFiles(query: string): Promise<ConnectorFile[]>;

  // Folder operations
  createFolder(name: string, parentId?: string): Promise<ConnectorFile>;
  deleteFolder(folderId: string): Promise<void>;

  // Storage info
  getStorageInfo(): Promise<{ used: number; total: number }>;
  getStatus(): Promise<ConnectorStatus>;
}

/**
 * OAuth 2.0 Configuration
 */
export interface OAuth2Config {
  clientId: string;
  clientSecret?: string;
  redirectUri: string;
  authorizationEndpoint: string;
  tokenEndpoint: string;
  scopes: string[];
}

/**
 * Provider metadata
 */
export interface ConnectorProviderMeta {
  id: ConnectorProvider;
  name: string;
  icon: string;
  authType: 'oauth2' | 'username-password' | 'api-key' | 'token';
  supportsSharing: boolean;
  maxFileSize: number; // bytes
  freeStorageGB: number;
  website: string;
  color: string;
}

export const CONNECTOR_PROVIDERS: Record<ConnectorProvider, ConnectorProviderMeta> = {
  'google-drive': {
    id: 'google-drive',
    name: 'Google Drive',
    icon: '📁',
    authType: 'oauth2',
    supportsSharing: true,
    maxFileSize: 5 * 1024 * 1024 * 1024, // 5GB
    freeStorageGB: 15,
    website: 'https://drive.google.com',
    color: '#4285F4',
  },
  onedrive: {
    id: 'onedrive',
    name: 'Microsoft OneDrive',
    icon: '☁️',
    authType: 'oauth2',
    supportsSharing: true,
    maxFileSize: 250 * 1024 * 1024 * 1024, // 250GB
    freeStorageGB: 5,
    website: 'https://onedrive.live.com',
    color: '#0078D4',
  },
  dropbox: {
    id: 'dropbox',
    name: 'Dropbox',
    icon: '📦',
    authType: 'oauth2',
    supportsSharing: true,
    maxFileSize: 50 * 1024 * 1024 * 1024, // 50GB via upload API
    freeStorageGB: 2,
    website: 'https://www.dropbox.com',
    color: '#0061FF',
  },
  box: {
    id: 'box',
    name: 'Box',
    icon: '🔷',
    authType: 'oauth2',
    supportsSharing: true,
    maxFileSize: 50 * 1024 * 1024 * 1024, // 50GB
    freeStorageGB: 10,
    website: 'https://www.box.com',
    color: '#0061D5',
  },
  pcloud: {
    id: 'pcloud',
    name: 'pCloud',
    icon: '☁️',
    authType: 'username-password',
    supportsSharing: true,
    maxFileSize: 50 * 1024 * 1024 * 1024, // 50GB
    freeStorageGB: 10,
    website: 'https://www.pcloud.com',
    color: '#00C9FF',
  },
  mega: {
    id: 'mega',
    name: 'MEGA',
    icon: '🔴',
    authType: 'username-password',
    supportsSharing: true,
    maxFileSize: Infinity, // No file size limit
    freeStorageGB: 20,
    website: 'https://mega.nz',
    color: '#D9272E',
  },
  s3: {
    id: 's3',
    name: 'AWS S3',
    icon: '🪣',
    authType: 'api-key',
    supportsSharing: false,
    maxFileSize: 5 * 1024 * 1024 * 1024 * 1024, // 5TB
    freeStorageGB: 0,
    website: 'https://aws.amazon.com/s3',
    color: '#FF9900',
  },
  icloud: {
    id: 'icloud',
    name: 'iCloud Drive',
    icon: '☁️',
    authType: 'username-password',
    supportsSharing: true,
    maxFileSize: 50 * 1024 * 1024 * 1024, // 50GB
    freeStorageGB: 5,
    website: 'https://www.icloud.com',
    color: '#007AFF',
  },
};
