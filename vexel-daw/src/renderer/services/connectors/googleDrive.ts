/**
 * Google Drive Connector
 * Implementation using Google Drive API v3
 *
 * Authentication: OAuth 2.0
 * API: https://developers.google.com/drive/api/v3/reference
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

export class GoogleDriveConnector implements ICloudConnector {
  readonly provider: ConnectorProvider = 'google-drive';
  readonly config: ConnectorConfig;

  private readonly API_BASE = 'https://www.googleapis.com/drive/v3';
  private readonly UPLOAD_API_BASE = 'https://www.googleapis.com/upload/drive/v3';

  constructor(config: ConnectorConfig) {
    this.config = config;
  }

  async authenticate(): Promise<boolean> {
    try {
      const oauthConfig = {
        ...OAUTH_CONFIGS['google-drive'],
        clientId: this.config.credentials.clientId || '',
        clientSecret: this.config.credentials.clientSecret,
      };

      const tokens = await oauth2Service.authenticate(oauthConfig as any);

      // Update credentials
      this.config.credentials.accessToken = tokens.access_token;
      this.config.credentials.refreshToken = tokens.refresh_token;
      this.config.credentials.expiresAt = oauth2Service.calculateExpiresAt(tokens.expires_in);

      return true;
    } catch (error) {
      console.error('Google Drive authentication failed:', error);
      return false;
    }
  }

  async refreshAuth(): Promise<boolean> {
    if (!this.config.credentials.refreshToken) {
      return false;
    }

    try {
      const oauthConfig = {
        ...OAUTH_CONFIGS['google-drive'],
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
    // Revoke token
    if (this.config.credentials.accessToken) {
      try {
        await fetch(
          `https://oauth2.googleapis.com/revoke?token=${this.config.credentials.accessToken}`,
          { method: 'POST' }
        );
      } catch (error) {
        console.error('Failed to revoke token:', error);
      }
    }

    // Clear credentials
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

    const metadata = {
      name: options.fileName,
      mimeType: options.mimeType,
      parents: options.folder ? [options.folder] : undefined,
    };

    // Use multipart upload for files with data
    const boundary = '-------314159265358979323846';
    const delimiter = `\r\n--${boundary}\r\n`;
    const close_delim = `\r\n--${boundary}--`;

    let fileData: ArrayBuffer;
    if (options.blob) {
      fileData = await options.blob.arrayBuffer();
    } else if (options.filePath) {
      // Read file from path using Electron's fs
      // In Electron renderer with nodeIntegration, we can use fs
      try {
        const fs = await import('fs/promises');
        const buffer = await fs.readFile(options.filePath);
        fileData = buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength);
      } catch (error) {
        throw new Error(`Failed to read file from path: ${error}`);
      }
    } else {
      throw new Error('No file data provided');
    }

    const multipartRequestBody =
      delimiter +
      'Content-Type: application/json\r\n\r\n' +
      JSON.stringify(metadata) +
      delimiter +
      `Content-Type: ${options.mimeType}\r\n` +
      'Content-Transfer-Encoding: base64\r\n\r\n' +
      this.arrayBufferToBase64(fileData) +
      close_delim;

    const response = await fetch(`${this.UPLOAD_API_BASE}/files?uploadType=multipart`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': `multipart/related; boundary=${boundary}`,
      },
      body: multipartRequestBody,
    });

    if (!response.ok) {
      throw new Error(`Upload failed: ${await response.text()}`);
    }

    const file = await response.json();

    return this.parseFile(file);
  }

  async downloadFile(options: ConnectorDownloadOptions): Promise<Blob> {
    await this.ensureAuth();

    const response = await fetch(`${this.API_BASE}/files/${options.fileId}?alt=media`, {
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

    const query = folderId
      ? `'${folderId}' in parents and trashed=false`
      : "'root' in parents and trashed=false";

    const response = await fetch(
      `${this.API_BASE}/files?q=${encodeURIComponent(query)}&fields=files(id,name,mimeType,size,modifiedTime)`,
      {
        headers: {
          Authorization: `Bearer ${this.config.credentials.accessToken}`,
        },
      }
    );

    if (!response.ok) {
      throw new Error(`List files failed: ${await response.text()}`);
    }

    const data = await response.json();
    return data.files.map((file: any) => this.parseFile(file));
  }

  async searchFiles(query: string): Promise<ConnectorFile[]> {
    await this.ensureAuth();

    const searchQuery = `name contains '${query}' and trashed=false`;

    const response = await fetch(
      `${this.API_BASE}/files?q=${encodeURIComponent(searchQuery)}&fields=files(id,name,mimeType,size,modifiedTime)`,
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
    return data.files.map((file: any) => this.parseFile(file));
  }

  async createFolder(name: string, parentId?: string): Promise<ConnectorFile> {
    await this.ensureAuth();

    const metadata = {
      name,
      mimeType: 'application/vnd.google-apps.folder',
      parents: parentId ? [parentId] : undefined,
    };

    const response = await fetch(`${this.API_BASE}/files`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(metadata),
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

    const response = await fetch(`${this.API_BASE}/about?fields=storageQuota`, {
      headers: {
        Authorization: `Bearer ${this.config.credentials.accessToken}`,
      },
    });

    if (!response.ok) {
      throw new Error(`Get storage info failed: ${await response.text()}`);
    }

    const data = await response.json();
    const quota = data.storageQuota;

    return {
      used: parseInt(quota.usage || '0'),
      total: parseInt(quota.limit || '0'),
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
      size: parseInt(file.size || '0'),
      mimeType: file.mimeType,
      modifiedTime: new Date(file.modifiedTime).getTime(),
      isFolder: file.mimeType === 'application/vnd.google-apps.folder',
    };
  }

  private arrayBufferToBase64(buffer: ArrayBuffer): string {
    const bytes = new Uint8Array(buffer);
    let binary = '';
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }
}
