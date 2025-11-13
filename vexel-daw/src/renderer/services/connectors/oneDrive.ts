/**
 * Microsoft OneDrive Connector
 * Implementation using Microsoft Graph API
 *
 * Authentication: OAuth 2.0
 * API: https://learn.microsoft.com/en-us/graph/api/resources/onedrive
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

export class OneDriveConnector implements ICloudConnector {
  readonly provider: ConnectorProvider = 'onedrive';
  readonly config: ConnectorConfig;

  private readonly API_BASE = 'https://graph.microsoft.com/v1.0';

  constructor(config: ConnectorConfig) {
    this.config = config;
  }

  async authenticate(): Promise<boolean> {
    try {
      const oauthConfig = {
        ...OAUTH_CONFIGS['onedrive'],
        clientId: this.config.credentials.clientId || '',
        clientSecret: this.config.credentials.clientSecret,
      };

      const tokens = await oauth2Service.authenticate(oauthConfig as any);

      this.config.credentials.accessToken = tokens.access_token;
      this.config.credentials.refreshToken = tokens.refresh_token;
      this.config.credentials.expiresAt = oauth2Service.calculateExpiresAt(tokens.expires_in);

      return true;
    } catch (error) {
      console.error('OneDrive authentication failed:', error);
      return false;
    }
  }

  async refreshAuth(): Promise<boolean> {
    if (!this.config.credentials.refreshToken) {
      return false;
    }

    try {
      const oauthConfig = {
        ...OAUTH_CONFIGS['onedrive'],
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

    let uploadPath = `/me/drive/root:/${options.fileName}:/content`;
    if (options.folder) {
      uploadPath = `/me/drive/items/${options.folder}:/${options.fileName}:/content`;
    }

    let fileData: ArrayBuffer;
    if (options.blob) {
      fileData = await options.blob.arrayBuffer();
    } else if (options.filePath) {
      throw new Error('File path upload not implemented');
    } else {
      throw new Error('No file data provided');
    }

    const response = await fetch(`${this.API_BASE}${uploadPath}`, {
      method: 'PUT',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': options.mimeType,
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

    // Get download URL first
    const response = await fetch(`${this.API_BASE}/me/drive/items/${options.fileId}`, {
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`Get file info failed: ${await response.text()}`);
    }

    const fileInfo = await response.json();
    const downloadUrl = fileInfo['@microsoft.graph.downloadUrl'];

    if (!downloadUrl) {
      throw new Error('No download URL available');
    }

    // Download file
    const downloadResponse = await fetch(downloadUrl);
    if (!downloadResponse.ok) {
      throw new Error(`Download failed: ${await downloadResponse.text()}`);
    }

    return await downloadResponse.blob();
  }

  async deleteFile(fileId: string): Promise<void> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/me/drive/items/${fileId}`, {
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

    const path = folderId
      ? `/me/drive/items/${folderId}/children`
      : '/me/drive/root/children';

    const response = await fetch(`${this.API_BASE}${path}`, {
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`List files failed: ${await response.text()}`);
    }

    const data = await response.json();
    return data.value.map((file: any) => this.parseFile(file));
  }

  async searchFiles(query: string): Promise<ConnectorFile[]> {
    await this.ensureAuth();

    const response = await fetch(
      `${this.API_BASE}/me/drive/root/search(q='${encodeURIComponent(query)}')`,
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
    return data.value.map((file: any) => this.parseFile(file));
  }

  async createFolder(name: string, parentId?: string): Promise<ConnectorFile> {
    await this.ensureAuth();

    const path = parentId
      ? `/me/drive/items/${parentId}/children`
      : '/me/drive/root/children';

    const response = await fetch(`${this.API_BASE}${path}`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        name,
        folder: {},
        '@microsoft.graph.conflictBehavior': 'rename',
      }),
    });

    if (!response.ok) {
      throw new Error(`Create folder failed: ${await response.text()}`);
    }

    const folder = await response.json();
    return this.parseFile(folder);
  }

  async deleteFolder(folderId: string): Promise<void> {
    await this.deleteFile(folderId);
  }

  async getStorageInfo(): Promise<{ used: number; total: number }> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/me/drive`, {
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`Get storage info failed: ${await response.text()}`);
    }

    const data = await response.json();
    const quota = data.quota;

    return {
      used: quota.used || 0,
      total: quota.total || 0,
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
      mimeType: file.file?.mimeType || 'application/octet-stream',
      modifiedTime: new Date(file.lastModifiedDateTime).getTime(),
      isFolder: !!file.folder,
    };
  }
}
