/**
 * Cloud Connector Manager
 * Manages all cloud storage connectors
 *
 * Features:
 * - Add/remove connectors
 * - Authentication management
 * - Unified file operations across providers
 * - Connector status tracking
 * - Save to multiple destinations
 */

import {
  ConnectorConfig,
  ConnectorProvider,
  ICloudConnector,
  ConnectorStatus,
  ConnectorFile,
  ConnectorUploadOptions,
  ConnectorDownloadOptions,
} from '../types/connectors';

// Import individual connector implementations
import { GoogleDriveConnector } from './connectors/googleDrive';
import { OneDriveConnector } from './connectors/oneDrive';
import { DropboxConnector } from './connectors/dropbox';
import { BoxConnector } from './connectors/box';
import { PCloudConnector } from './connectors/pCloud';
import { MEGAConnector } from './connectors/mega';

export class ConnectorManager {
  private static instance: ConnectorManager | null = null;
  private connectors: Map<string, ICloudConnector> = new Map();
  private configs: Map<string, ConnectorConfig> = new Map();

  private constructor() {
    this.loadConfigs();
  }

  static getInstance(): ConnectorManager {
    if (!ConnectorManager.instance) {
      ConnectorManager.instance = new ConnectorManager();
    }
    return ConnectorManager.instance;
  }

  /**
   * Add a new connector
   */
  async addConnector(config: ConnectorConfig): Promise<ICloudConnector> {
    console.log(`Adding connector: ${config.name} (${config.provider})`);

    // Create connector instance based on provider
    const connector = this.createConnectorInstance(config);

    // Authenticate
    const authenticated = await connector.authenticate();
    if (!authenticated) {
      throw new Error(`Failed to authenticate ${config.name}`);
    }

    // Store connector
    this.connectors.set(config.id, connector);
    this.configs.set(config.id, config);

    // Save to storage
    this.saveConfigs();

    console.log(`Connector added successfully: ${config.id}`);

    return connector;
  }

  /**
   * Remove a connector
   */
  async removeConnector(connectorId: string): Promise<void> {
    const connector = this.connectors.get(connectorId);
    if (connector) {
      await connector.disconnect();
      this.connectors.delete(connectorId);
    }

    this.configs.delete(connectorId);
    this.saveConfigs();

    console.log(`Connector removed: ${connectorId}`);
  }

  /**
   * Get connector by ID
   */
  getConnector(connectorId: string): ICloudConnector | undefined {
    return this.connectors.get(connectorId);
  }

  /**
   * Get all connectors
   */
  getAllConnectors(): ICloudConnector[] {
    return Array.from(this.connectors.values());
  }

  /**
   * Get all connector configs
   */
  getAllConfigs(): ConnectorConfig[] {
    return Array.from(this.configs.values());
  }

  /**
   * Get enabled connectors
   */
  getEnabledConnectors(): ICloudConnector[] {
    return Array.from(this.connectors.values()).filter((connector) => {
      const config = this.configs.get(connector.config.id);
      return config?.enabled;
    });
  }

  /**
   * Update connector config
   */
  updateConfig(connectorId: string, updates: Partial<ConnectorConfig>): void {
    const config = this.configs.get(connectorId);
    if (!config) {
      throw new Error(`Connector not found: ${connectorId}`);
    }

    Object.assign(config, updates);
    this.saveConfigs();

    console.log(`Connector config updated: ${connectorId}`);
  }

  /**
   * Get connector status
   */
  async getStatus(connectorId: string): Promise<ConnectorStatus | null> {
    const connector = this.connectors.get(connectorId);
    if (!connector) {
      return null;
    }

    try {
      return await connector.getStatus();
    } catch (error) {
      console.error(`Failed to get status for ${connectorId}:`, error);
      return {
        id: connectorId,
        connected: false,
        syncing: false,
        lastError: error instanceof Error ? error.message : 'Unknown error',
      };
    }
  }

  /**
   * Get status of all connectors
   */
  async getAllStatuses(): Promise<ConnectorStatus[]> {
    const statuses: ConnectorStatus[] = [];

    for (const connector of this.connectors.values()) {
      const status = await this.getStatus(connector.config.id);
      if (status) {
        statuses.push(status);
      }
    }

    return statuses;
  }

  /**
   * Upload file to connector
   */
  async uploadFile(
    connectorId: string,
    options: ConnectorUploadOptions
  ): Promise<ConnectorFile> {
    const connector = this.connectors.get(connectorId);
    if (!connector) {
      throw new Error(`Connector not found: ${connectorId}`);
    }

    console.log(`Uploading to ${connector.config.name}: ${options.fileName}`);

    return await connector.uploadFile(options);
  }

  /**
   * Download file from connector
   */
  async downloadFile(
    connectorId: string,
    options: ConnectorDownloadOptions
  ): Promise<Blob> {
    const connector = this.connectors.get(connectorId);
    if (!connector) {
      throw new Error(`Connector not found: ${connectorId}`);
    }

    console.log(`Downloading from ${connector.config.name}`);

    return await connector.downloadFile(options);
  }

  /**
   * List files in connector
   */
  async listFiles(connectorId: string, folderId?: string): Promise<ConnectorFile[]> {
    const connector = this.connectors.get(connectorId);
    if (!connector) {
      throw new Error(`Connector not found: ${connectorId}`);
    }

    return await connector.listFiles(folderId);
  }

  /**
   * Search files across all connectors
   */
  async searchAllConnectors(query: string): Promise<Map<string, ConnectorFile[]>> {
    const results = new Map<string, ConnectorFile[]>();

    for (const connector of this.getEnabledConnectors()) {
      try {
        const files = await connector.searchFiles(query);
        results.set(connector.config.id, files);
      } catch (error) {
        console.error(`Search failed for ${connector.config.name}:`, error);
      }
    }

    return results;
  }

  /**
   * Create connector instance based on provider
   */
  private createConnectorInstance(config: ConnectorConfig): ICloudConnector {
    switch (config.provider) {
      case 'google-drive':
        return new GoogleDriveConnector(config);

      case 'onedrive':
        return new OneDriveConnector(config);

      case 'dropbox':
        return new DropboxConnector(config);

      case 'box':
        return new BoxConnector(config);

      case 'pcloud':
        return new PCloudConnector(config);

      case 'mega':
        return new MEGAConnector(config);

      default:
        throw new Error(`Unsupported provider: ${config.provider}`);
    }
  }

  /**
   * Load connector configs from storage
   */
  private loadConfigs(): void {
    const stored = localStorage.getItem('zenith-daw-connectors');
    if (stored) {
      try {
        const configs: ConnectorConfig[] = JSON.parse(stored);
        configs.forEach((config) => {
          this.configs.set(config.id, config);

          // Try to recreate connector instances
          try {
            const connector = this.createConnectorInstance(config);
            if (connector.isAuthenticated()) {
              this.connectors.set(config.id, connector);
            }
          } catch (error) {
            console.error(`Failed to load connector ${config.id}:`, error);
          }
        });
      } catch (error) {
        console.error('Failed to load connector configs:', error);
      }
    }
  }

  /**
   * Save connector configs to storage
   */
  private saveConfigs(): void {
    const configs = Array.from(this.configs.values());
    localStorage.setItem('zenith-daw-connectors', JSON.stringify(configs));
  }

  /**
   * Test connector connection
   */
  async testConnection(connectorId: string): Promise<boolean> {
    const connector = this.connectors.get(connectorId);
    if (!connector) {
      return false;
    }

    try {
      await connector.getStorageInfo();
      return true;
    } catch (error) {
      console.error(`Connection test failed for ${connectorId}:`, error);
      return false;
    }
  }

  /**
   * Refresh authentication for all connectors
   */
  async refreshAllAuth(): Promise<void> {
    const promises = Array.from(this.connectors.values()).map(async (connector) => {
      try {
        await connector.refreshAuth();
      } catch (error) {
        console.error(`Failed to refresh auth for ${connector.config.name}:`, error);
      }
    });

    await Promise.all(promises);
  }

  /**
   * Clear all connectors
   */
  async clearAll(): Promise<void> {
    for (const connector of this.connectors.values()) {
      await connector.disconnect();
    }

    this.connectors.clear();
    this.configs.clear();
    localStorage.removeItem('zenith-daw-connectors');

    console.log('All connectors cleared');
  }
}

export const connectorManager = ConnectorManager.getInstance();
