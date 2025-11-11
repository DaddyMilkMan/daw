/**
 * MEGA Connector
 *
 * SDK: megajs (npm package) - optional dependency
 * API: https://mega.io/developers
 *
 * Install megajs for full functionality: npm install megajs
 * Dynamically loads megajs to avoid requiring it as a hard dependency
 */

import {
  ConnectorConfig,
  ConnectorProvider,
  ICloudConnector,
  ConnectorStatus,
  ConnectorFile,
  ConnectorUploadOptions,
  ConnectorDownloadOptions,
} from '../../types/connectors';

// Dynamic import type
type MegaStorage = any;

export class MEGAConnector implements ICloudConnector {
  readonly provider: ConnectorProvider = 'mega';
  readonly config: ConnectorConfig;
  private storage: MegaStorage | null = null;
  private megajs: any = null;

  constructor(config: ConnectorConfig) {
    this.config = config;
  }

  /**
   * Lazy-load megajs library
   */
  private async loadMegaJS(): Promise<boolean> {
    if (this.megajs) return true;

    try {
      // Dynamic import to avoid hard dependency
      this.megajs = await import('megajs');
      return true;
    } catch (error) {
      console.warn('megajs library not installed. Run: npm install megajs');
      return false;
    }
  }

  async authenticate(): Promise<boolean> {
    const loaded = await this.loadMegaJS();
    if (!loaded) {
      console.warn('MEGA connector requires megajs package');
      return false;
    }

    try {
      const { Storage } = this.megajs;

      // Create storage instance with credentials
      this.storage = new Storage({
        email: this.config.credentials.email,
        password: this.config.credentials.password,
      });

      // Wait for ready event
      await new Promise<void>((resolve, reject) => {
        this.storage.once('ready', () => resolve());
        this.storage.once('error', (err: Error) => reject(err));
      });

      console.log('[MEGA] Authenticated successfully');
      return true;
    } catch (error) {
      console.error('[MEGA] Authentication failed:', error);
      this.storage = null;
      return false;
    }
  }

  async refreshAuth(): Promise<boolean> {
    if (!this.storage) {
      return this.authenticate();
    }
    return this.isAuthenticated();
  }

  async disconnect(): Promise<void> {
    if (this.storage) {
      try {
        this.storage.close();
      } catch (error) {
        console.error('[MEGA] Error disconnecting:', error);
      }
      this.storage = null;
    }
    this.config.credentials = {};
  }

  isAuthenticated(): boolean {
    return !!this.storage;
  }

  async uploadFile(options: ConnectorUploadOptions): Promise<ConnectorFile> {
    if (!this.storage) {
      throw new Error('Not authenticated with MEGA');
    }

    try {
      // Convert blob to buffer for upload
      const buffer = await options.blob.arrayBuffer();
      const uint8Array = new Uint8Array(buffer);

      // Find or create root folder
      const root = this.storage.root;
      const targetFolder = options.folderId
        ? this.storage.files[options.folderId]
        : root;

      if (!targetFolder) {
        throw new Error('Target folder not found');
      }

      // Upload file
      const uploadedFile = await targetFolder.upload({
        name: options.fileName,
        size: options.blob.size,
      }, uint8Array).complete;

      return {
        id: uploadedFile.nodeId,
        name: uploadedFile.name,
        size: uploadedFile.size,
        mimeType: options.blob.type,
        createdTime: new Date().toISOString(),
        modifiedTime: new Date().toISOString(),
        isFolder: false,
      };
    } catch (error) {
      console.error('[MEGA] Upload failed:', error);
      throw new Error(`MEGA upload failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async downloadFile(options: ConnectorDownloadOptions): Promise<Blob> {
    if (!this.storage) {
      throw new Error('Not authenticated with MEGA');
    }

    try {
      const file = this.storage.files[options.fileId];
      if (!file) {
        throw new Error('File not found');
      }

      // Download file
      const buffer = await file.downloadBuffer();

      // Convert to Blob
      return new Blob([buffer], { type: 'application/octet-stream' });
    } catch (error) {
      console.error('[MEGA] Download failed:', error);
      throw new Error(`MEGA download failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async deleteFile(fileId: string): Promise<void> {
    if (!this.storage) {
      throw new Error('Not authenticated with MEGA');
    }

    try {
      const file = this.storage.files[fileId];
      if (!file) {
        throw new Error('File not found');
      }

      await file.delete(true); // permanent = true
      console.log('[MEGA] File deleted:', fileId);
    } catch (error) {
      console.error('[MEGA] Delete failed:', error);
      throw new Error(`MEGA delete failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async listFiles(folderId?: string): Promise<ConnectorFile[]> {
    if (!this.storage) {
      return [];
    }

    try {
      const folder = folderId
        ? this.storage.files[folderId]
        : this.storage.root;

      if (!folder || !folder.children) {
        return [];
      }

      return folder.children.map((child: any) => ({
        id: child.nodeId,
        name: child.name,
        size: child.size || 0,
        mimeType: child.directory ? 'folder' : 'application/octet-stream',
        createdTime: new Date(child.timestamp * 1000).toISOString(),
        modifiedTime: new Date(child.timestamp * 1000).toISOString(),
        isFolder: !!child.directory,
      }));
    } catch (error) {
      console.error('[MEGA] List files failed:', error);
      return [];
    }
  }

  async searchFiles(query: string): Promise<ConnectorFile[]> {
    // MEGA doesn't have built-in search, so we'll filter files manually
    const allFiles = await this.listFiles();
    const lowerQuery = query.toLowerCase();

    return allFiles.filter(file =>
      file.name.toLowerCase().includes(lowerQuery)
    );
  }

  async createFolder(name: string, parentId?: string): Promise<ConnectorFile> {
    if (!this.storage) {
      throw new Error('Not authenticated with MEGA');
    }

    try {
      const parent = parentId
        ? this.storage.files[parentId]
        : this.storage.root;

      if (!parent) {
        throw new Error('Parent folder not found');
      }

      const newFolder = await parent.mkdir(name);

      return {
        id: newFolder.nodeId,
        name: newFolder.name,
        size: 0,
        mimeType: 'folder',
        createdTime: new Date().toISOString(),
        modifiedTime: new Date().toISOString(),
        isFolder: true,
      };
    } catch (error) {
      console.error('[MEGA] Create folder failed:', error);
      throw new Error(`MEGA create folder failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async deleteFolder(folderId: string): Promise<void> {
    // Folders are deleted the same way as files in MEGA
    await this.deleteFile(folderId);
  }

  async getStorageInfo(): Promise<{ used: number; total: number }> {
    if (!this.storage) {
      return { used: 0, total: 0 };
    }

    try {
      const accountInfo = this.storage.getAccountInfo();
      return {
        used: accountInfo?.used || 0,
        total: accountInfo?.total || 0,
      };
    } catch (error) {
      console.error('[MEGA] Get storage info failed:', error);
      return { used: 0, total: 0 };
    }
  }

  async getStatus(): Promise<ConnectorStatus> {
    return {
      id: this.config.id,
      connected: this.isAuthenticated(),
      syncing: false,
      lastError: !this.megajs ? 'MEGA SDK not installed (run: npm install megajs)' : undefined,
    };
  }
}
