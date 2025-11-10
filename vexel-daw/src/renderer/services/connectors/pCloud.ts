/**
 * pCloud Connector
 *
 * SDK: pcloud-sdk-js (npm package) - optional dependency
 * API: https://docs.pcloud.com/
 *
 * Install pcloud-sdk-js for full functionality: npm install pcloud-sdk-js
 * Dynamically loads pcloud-sdk-js to avoid requiring it as a hard dependency
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
type PCloudClient = any;

export class PCloudConnector implements ICloudConnector {
  readonly provider: ConnectorProvider = 'pcloud';
  readonly config: ConnectorConfig;
  private client: PCloudClient | null = null;
  private pcloudSdk: any = null;

  constructor(config: ConnectorConfig) {
    this.config = config;
  }

  /**
   * Lazy-load pcloud-sdk-js library
   */
  private async loadPCloudSDK(): Promise<boolean> {
    if (this.pcloudSdk) return true;

    try {
      // Dynamic import to avoid hard dependency
      this.pcloudSdk = await import('pcloud-sdk-js');
      return true;
    } catch (error) {
      console.warn('pcloud-sdk-js library not installed. Run: npm install pcloud-sdk-js');
      return false;
    }
  }

  async authenticate(): Promise<boolean> {
    const loaded = await this.loadPCloudSDK();
    if (!loaded) {
      console.warn('pCloud connector requires pcloud-sdk-js package');
      return false;
    }

    try {
      // Create pCloud client with OAuth token
      this.client = this.pcloudSdk.createClient(this.config.credentials.authToken);

      // Verify authentication by getting user info
      const userInfo = await this.client.api('userinfo');

      if (userInfo && userInfo.result === 0) {
        console.log('[pCloud] Authenticated successfully');
        return true;
      }

      console.error('[pCloud] Authentication failed');
      this.client = null;
      return false;
    } catch (error) {
      console.error('[pCloud] Authentication failed:', error);
      this.client = null;
      return false;
    }
  }

  async refreshAuth(): Promise<boolean> {
    if (!this.client) {
      return this.authenticate();
    }
    return this.isAuthenticated();
  }

  async disconnect(): Promise<void> {
    this.client = null;
    this.config.credentials = {};
  }

  isAuthenticated(): boolean {
    return !!this.client;
  }

  async uploadFile(options: ConnectorUploadOptions): Promise<ConnectorFile> {
    if (!this.client) {
      throw new Error('Not authenticated with pCloud');
    }

    try {
      // Convert blob to file for upload
      const arrayBuffer = await options.blob.arrayBuffer();
      const uint8Array = new Uint8Array(arrayBuffer);

      // Upload file using pCloud API
      const folderId = options.folderId || 0; // 0 is root folder
      const result = await this.client.upload(
        uint8Array,
        folderId,
        options.fileName
      );

      if (result.result !== 0) {
        throw new Error(`pCloud upload failed: ${result.error || 'Unknown error'}`);
      }

      const metadata = result.metadata[0];

      return {
        id: metadata.fileid.toString(),
        name: metadata.name,
        size: metadata.size,
        mimeType: options.blob.type,
        createdTime: new Date(metadata.created * 1000).toISOString(),
        modifiedTime: new Date(metadata.modified * 1000).toISOString(),
        isFolder: false,
      };
    } catch (error) {
      console.error('[pCloud] Upload failed:', error);
      throw new Error(`pCloud upload failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async downloadFile(options: ConnectorDownloadOptions): Promise<Blob> {
    if (!this.client) {
      throw new Error('Not authenticated with pCloud');
    }

    try {
      // Get download link
      const result = await this.client.api('getfilelink', {
        fileid: options.fileId,
      });

      if (result.result !== 0) {
        throw new Error(`Failed to get download link: ${result.error || 'Unknown error'}`);
      }

      // Download file from the link
      const downloadUrl = `https://${result.hosts[0]}${result.path}`;
      const response = await fetch(downloadUrl);

      if (!response.ok) {
        throw new Error(`Download failed: ${response.statusText}`);
      }

      return await response.blob();
    } catch (error) {
      console.error('[pCloud] Download failed:', error);
      throw new Error(`pCloud download failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async deleteFile(fileId: string): Promise<void> {
    if (!this.client) {
      throw new Error('Not authenticated with pCloud');
    }

    try {
      const result = await this.client.api('deletefile', {
        fileid: fileId,
      });

      if (result.result !== 0) {
        throw new Error(`Delete failed: ${result.error || 'Unknown error'}`);
      }

      console.log('[pCloud] File deleted:', fileId);
    } catch (error) {
      console.error('[pCloud] Delete failed:', error);
      throw new Error(`pCloud delete failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async listFiles(folderId?: string): Promise<ConnectorFile[]> {
    if (!this.client) {
      return [];
    }

    try {
      const result = await this.client.api('listfolder', {
        folderid: folderId || 0,
      });

      if (result.result !== 0) {
        console.error('[pCloud] List files failed:', result.error);
        return [];
      }

      const contents = result.metadata?.contents || [];

      return contents.map((item: any) => ({
        id: (item.fileid || item.folderid)?.toString() || '',
        name: item.name,
        size: item.size || 0,
        mimeType: item.isfolder ? 'folder' : (item.contenttype || 'application/octet-stream'),
        createdTime: new Date((item.created || 0) * 1000).toISOString(),
        modifiedTime: new Date((item.modified || 0) * 1000).toISOString(),
        isFolder: !!item.isfolder,
      }));
    } catch (error) {
      console.error('[pCloud] List files failed:', error);
      return [];
    }
  }

  async searchFiles(query: string): Promise<ConnectorFile[]> {
    // pCloud doesn't have a simple search API in the SDK
    // We'll do a manual search by listing and filtering
    const allFiles = await this.listFiles();
    const lowerQuery = query.toLowerCase();

    return allFiles.filter(file =>
      file.name.toLowerCase().includes(lowerQuery)
    );
  }

  async createFolder(name: string, parentId?: string): Promise<ConnectorFile> {
    if (!this.client) {
      throw new Error('Not authenticated with pCloud');
    }

    try {
      const result = await this.client.api('createfolder', {
        folderid: parentId || 0,
        name: name,
      });

      if (result.result !== 0) {
        throw new Error(`Create folder failed: ${result.error || 'Unknown error'}`);
      }

      const metadata = result.metadata;

      return {
        id: metadata.folderid.toString(),
        name: metadata.name,
        size: 0,
        mimeType: 'folder',
        createdTime: new Date(metadata.created * 1000).toISOString(),
        modifiedTime: new Date(metadata.modified * 1000).toISOString(),
        isFolder: true,
      };
    } catch (error) {
      console.error('[pCloud] Create folder failed:', error);
      throw new Error(`pCloud create folder failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async deleteFolder(folderId: string): Promise<void> {
    if (!this.client) {
      throw new Error('Not authenticated with pCloud');
    }

    try {
      const result = await this.client.api('deletefolder', {
        folderid: folderId,
      });

      if (result.result !== 0) {
        throw new Error(`Delete folder failed: ${result.error || 'Unknown error'}`);
      }

      console.log('[pCloud] Folder deleted:', folderId);
    } catch (error) {
      console.error('[pCloud] Delete folder failed:', error);
      throw new Error(`pCloud delete folder failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  async getStorageInfo(): Promise<{ used: number; total: number }> {
    if (!this.client) {
      return { used: 0, total: 0 };
    }

    try {
      const result = await this.client.api('userinfo');

      if (result.result !== 0) {
        return { used: 0, total: 0 };
      }

      return {
        used: result.usedquota || 0,
        total: result.quota || 0,
      };
    } catch (error) {
      console.error('[pCloud] Get storage info failed:', error);
      return { used: 0, total: 0 };
    }
  }

  async getStatus(): Promise<ConnectorStatus> {
    return {
      id: this.config.id,
      connected: this.isAuthenticated(),
      syncing: false,
      lastError: !this.pcloudSdk ? 'pCloud SDK not installed (run: npm install pcloud-sdk-js)' : undefined,
    };
  }
}
