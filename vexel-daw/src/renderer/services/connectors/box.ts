/**
 * Box Connector
 * Implementation using Box Content API
 *
 * Authentication: OAuth 2.0
 * SDK: box-node-sdk (npm package) - not installed by default
 * API: https://developer.box.com/reference/
 *
 * Note: This is a placeholder implementation. For production use, install:
 * npm install box-node-sdk
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

export class BoxConnector implements ICloudConnector {
  readonly provider: ConnectorProvider = 'box';
  readonly config: ConnectorConfig;

  private readonly API_BASE = 'https://api.box.com/2.0';
  private readonly UPLOAD_API_BASE = 'https://upload.box.com/api/2.0';

  constructor(config: ConnectorConfig) {
    this.config = config;
  }

  async authenticate(): Promise<boolean> {
    try {
      const oauthConfig = {
        ...OAUTH_CONFIGS['box'],
        clientId: this.config.credentials.clientId || '',
        clientSecret: this.config.credentials.clientSecret,
      };

      const tokens = await oauth2Service.authenticate(oauthConfig as any);

      this.config.credentials.accessToken = tokens.access_token;
      this.config.credentials.refreshToken = tokens.refresh_token;
      this.config.credentials.expiresAt = oauth2Service.calculateExpiresAt(tokens.expires_in);

      return true;
    } catch (error) {
      console.error('Box authentication failed:', error);
      return false;
    }
  }

  async refreshAuth(): Promise<boolean> {
    if (!this.config.credentials.refreshToken) {
      return false;
    }

    try {
      const oauthConfig = {
        ...OAUTH_CONFIGS['box'],
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

    const formData = new FormData();
    formData.append('attributes', JSON.stringify({
      name: options.fileName,
      parent: { id: options.folder || '0' },
    }));

    if (options.blob) {
      formData.append('file', options.blob, options.fileName);
    } else {
      throw new Error('No file data provided');
    }

    const response = await fetch(`${this.UPLOAD_API_BASE}/files/content`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
      body: formData,
    });

    if (!response.ok) {
      throw new Error(`Upload failed: ${await response.text()}`);
    }

    const data = await response.json();
    return this.parseFile(data.entries[0]);
  }

  async downloadFile(options: ConnectorDownloadOptions): Promise<Blob> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/files/${options.fileId}/content`, {
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`Download failed: ${await response.text()}`);
    }

    return await response.blob();
  }

  async deleteFile(fileId: string): Promise<void> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/files/${fileId}`, {
      method: 'DELETE',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`Delete failed: ${await response.text()}`);
    }
  }

  async listFiles(folderId?: string): Promise<ConnectorFile[]> {
    await this.ensureAuth();

    const folder = folderId || '0';

    const response = await fetch(`${this.API_BASE}/folders/${folder}/items`, {
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`List files failed: ${await response.text()}`);
    }

    const data = await response.json();
    return data.entries.map((file: any) => this.parseFile(file));
  }

  async searchFiles(query: string): Promise<ConnectorFile[]> {
    await this.ensureAuth();

    const response = await fetch(
      `${this.API_BASE}/search?query=${encodeURIComponent(query)}&type=file`,
      {
        headers: {
          Authorization: `Bearer ${this.config.credentials.accessToken}`,
        },
      }
    );

    if (!response.ok) {
      throw new Error(`Search failed: ${await response.text()}`);
    }

    const data = await response.json();
    return data.entries.map((file: any) => this.parseFile(file));
  }

  async createFolder(name: string, parentId?: string): Promise<ConnectorFile> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/folders`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        name,
        parent: { id: parentId || '0' },
      }),
    });

    if (!response.ok) {
      throw new Error(`Create folder failed: ${await response.text()}`);
    }

    const folder = await response.json();
    return this.parseFile(folder);
  }

  async deleteFolder(folderId: string): Promise<void> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/folders/${folderId}?recursive=true`, {
      method: 'DELETE',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`Delete folder failed: ${await response.text()}`);
    }
  }

  async getStorageInfo(): Promise<{ used: number; total: number }> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/users/me`, {
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`Get storage info failed: ${await response.text()}`);
    }

    const data = await response.json();

    return {
      used: data.space_used || 0,
      total: data.space_amount || 0,
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
      id: file.id,
      name: file.name,
      path: file.name,
      size: file.size || 0,
      mimeType: file.type === 'folder' ? 'folder' : 'application/octet-stream',
      modifiedTime: file.modified_at ? new Date(file.modified_at).getTime() : Date.now(),
      isFolder: file.type === 'folder',
    };
  }
}
