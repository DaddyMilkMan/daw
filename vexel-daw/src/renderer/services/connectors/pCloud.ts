/**
 * pCloud Connector (Placeholder)
 *
 * SDK: pcloud-sdk-js (npm package) - not installed by default
 * API: https://docs.pcloud.com/
 *
 * Note: This is a placeholder. Install pcloud-sdk-js for full functionality:
 * npm install pcloud-sdk-js
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

export class PCloudConnector implements ICloudConnector {
  readonly provider: ConnectorProvider = 'pcloud';
  readonly config: ConnectorConfig;

  constructor(config: ConnectorConfig) {
    this.config = config;
  }

  async authenticate(): Promise<boolean> {
    console.warn('pCloud connector requires pcloud-sdk-js package');
    return false;
  }

  async refreshAuth(): Promise<boolean> {
    return this.isAuthenticated();
  }

  async disconnect(): Promise<void> {
    this.config.credentials = {};
  }

  isAuthenticated(): boolean {
    return !!this.config.credentials.authToken;
  }

  async uploadFile(options: ConnectorUploadOptions): Promise<ConnectorFile> {
    throw new Error('pCloud connector not fully implemented');
  }

  async downloadFile(options: ConnectorDownloadOptions): Promise<Blob> {
    throw new Error('pCloud connector not fully implemented');
  }

  async deleteFile(fileId: string): Promise<void> {
    throw new Error('pCloud connector not fully implemented');
  }

  async listFiles(folderId?: string): Promise<ConnectorFile[]> {
    return [];
  }

  async searchFiles(query: string): Promise<ConnectorFile[]> {
    return [];
  }

  async createFolder(name: string, parentId?: string): Promise<ConnectorFile> {
    throw new Error('pCloud connector not fully implemented');
  }

  async deleteFolder(folderId: string): Promise<void> {
    throw new Error('pCloud connector not fully implemented');
  }

  async getStorageInfo(): Promise<{ used: number; total: number }> {
    return { used: 0, total: 0 };
  }

  async getStatus(): Promise<ConnectorStatus> {
    return {
      id: this.config.id,
      connected: false,
      syncing: false,
      lastError: 'pCloud SDK not installed',
    };
  }
}
