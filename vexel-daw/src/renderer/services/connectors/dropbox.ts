/**
 * Dropbox Connector
 * Implementation using Dropbox API v2
 *
 * Authentication: OAuth 2.0
 * SDK: dropbox (npm package) - not installed by default
 * API: https://www.dropbox.com/developers/documentation/http/documentation
 *
 * Note: This is a placeholder implementation. For production use, install:
 * npm install dropbox
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
import { oauth2Service, OAUTH_CONFIGS } from '../oauth';

export class DropboxConnector implements ICloudConnector {
  readonly provider: ConnectorProvider = 'dropbox';
  readonly config: ConnectorConfig;

  private readonly API_BASE = 'https://api.dropboxapi.com/2';
  private readonly CONTENT_API_BASE = 'https://content.dropboxapi.com/2';

  constructor(config: ConnectorConfig) {
    this.config = config;
  }

  async authenticate(): Promise<boolean> {
    try {
      const oauthConfig = {
        ...OAUTH_CONFIGS['dropbox'],
        clientId: this.config.credentials.clientId || '',
        clientSecret: this.config.credentials.clientSecret,
      };

      const tokens = await oauth2Service.authenticate(oauthConfig as any);

      this.config.credentials.accessToken = tokens.access_token;
      this.config.credentials.refreshToken = tokens.refresh_token;
      this.config.credentials.expiresAt = oauth2Service.calculateExpiresAt(tokens.expires_in);

      return true;
    } catch (error) {
      console.error('Dropbox authentication failed:', error);
      return false;
    }
  }

  async refreshAuth(): Promise<boolean> {
    if (!this.config.credentials.refreshToken) {
      return false;
    }

    try {
      const oauthConfig = {
        ...OAUTH_CONFIGS['dropbox'],
        clientId: this.config.credentials.clientId || '',
        clientSecret: this.config.credentials.clientSecret,
      };

      const tokens = await oauth2Service.refreshToken(
        oauthConfig as any,
        this.config.credentials.refreshToken
      );

      this.config.credentials.accessToken = tokens.access_token;
      if (tokens.refresh_token) {
        this.config.credentials.refreshToken = tokens.refresh_token;
      }
      this.config.credentials.expiresAt = oauth2Service.calculateExpiresAt(tokens.expires_in);

      return true;
    } catch (error) {
      console.error('Token refresh failed:', error);
      return false;
    }
  }

  async disconnect(): Promise<void> {
    this.config.credentials = {};
  }

  isAuthenticated(): boolean {
    return !!(
      this.config.credentials.accessToken &&
      this.config.credentials.expiresAt &&
      !oauth2Service.isTokenExpired(this.config.credentials.expiresAt)
    );
  }

  async uploadFile(options: ConnectorUploadOptions): Promise<ConnectorFile> {
    await this.ensureAuth();

    const path = options.folder
      ? `${options.folder}/${options.fileName}`
      : `/${options.fileName}`;

    let fileData: ArrayBuffer;
    if (options.blob) {
      fileData = await options.blob.arrayBuffer();
    } else if (options.filePath) {
      throw new Error('File path upload not implemented');
    } else {
      throw new Error('No file data provided');
    }

    const response = await fetch(`${this.CONTENT_API_BASE}/files/upload`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/octet-stream',
        'Dropbox-API-Arg': JSON.stringify({
          path,
          mode: 'add',
          autorename: true,
          mute: false,
        }),
      },
      body: fileData,
    });

    if (!response.ok) {
      throw new Error(`Upload failed: ${await response.text()}`);
    }

    const file = await response.json();
    return this.parseFile(file);
  }

  async downloadFile(options: ConnectorDownloadOptions): Promise<Blob> {
    await this.ensureAuth();

    const response = await fetch(`${this.CONTENT_API_BASE}/files/download`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Dropbox-API-Arg': JSON.stringify({
          path: options.fileId,
        }),
      },
    });

    if (!response.ok) {
      throw new Error(`Download failed: ${await response.text()}`);
    }

    return await response.blob();
  }

  async deleteFile(fileId: string): Promise<void> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/files/delete_v2`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ path: fileId }),
    });

    if (!response.ok) {
      throw new Error(`Delete failed: ${await response.text()}`);
    }
  }

  async listFiles(folderId?: string): Promise<ConnectorFile[]> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/files/list_folder`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        path: folderId || '',
        recursive: false,
      }),
    });

    if (!response.ok) {
      throw new Error(`List files failed: ${await response.text()}`);
    }

    const data = await response.json();
    return data.entries.map((file: any) => this.parseFile(file));
  }

  async searchFiles(query: string): Promise<ConnectorFile[]> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/files/search_v2`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        query,
        options: {
          path: '',
          max_results: 100,
        },
      }),
    });

    if (!response.ok) {
      throw new Error(`Search failed: ${await response.text()}`);
    }

    const data = await response.json();
    return data.matches.map((match: any) => this.parseFile(match.metadata.metadata));
  }

  async createFolder(name: string, parentId?: string): Promise<ConnectorFile> {
    await this.ensureAuth();

    const path = parentId ? `${parentId}/${name}` : `/${name}`;

    const response = await fetch(`${this.API_BASE}/files/create_folder_v2`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ path }),
    });

    if (!response.ok) {
      throw new Error(`Create folder failed: ${await response.text()}`);
    }

    const data = await response.json();
    return this.parseFile(data.metadata);
  }

  async deleteFolder(folderId: string): Promise<void> {
    await this.deleteFile(folderId);
  }

  async getStorageInfo(): Promise<{ used: number; total: number }> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/users/get_space_usage`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/json',
      },
    });

    if (!response.ok) {
      throw new Error(`Get storage info failed: ${await response.text()}`);
    }

    const data = await response.json();

    return {
      used: data.used || 0,
      total: data.allocation.allocated || 0,
    };
  }

  async getStatus(): Promise<ConnectorStatus> {
    try {
      const storage = await this.getStorageInfo();

      return {
        id: this.config.id,
        connected: this.isAuthenticated(),
        syncing: false,
        storageUsed: storage.used,
        storageTotal: storage.total,
        lastSync: this.config.lastSync,
      };
    } catch (error) {
      return {
        id: this.config.id,
        connected: false,
        syncing: false,
        lastError: error instanceof Error ? error.message : 'Unknown error',
      };
    }
  }

  private async ensureAuth(): Promise<void> {
    if (!this.isAuthenticated()) {
      const refreshed = await this.refreshAuth();
      if (!refreshed) {
        throw new Error('Authentication required');
      }
    }
  }

  private parseFile(file: any): ConnectorFile {
    return {
      id: file.path_lower || file.id,
      name: file.name,
      path: file.path_display || file.path_lower,
      size: file.size || 0,
      mimeType: file['.tag'] === 'folder' ? 'folder' : 'application/octet-stream',
      modifiedTime: file.server_modified
        ? new Date(file.server_modified).getTime()
        : Date.now(),
      isFolder: file['.tag'] === 'folder',
    };
  }
}
