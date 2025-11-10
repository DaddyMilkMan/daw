/**
 * Connectors Settings Component
 * Manage cloud storage connectors
 *
 * Features:
 * - Add new connectors
 * - Configure credentials
 * - Test connections
 * - Enable/disable connectors
 * - View storage info
 */

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  X,
  Plus,
  Settings,
  CheckCircle,
  AlertCircle,
  Trash2,
  RefreshCw,
  Power,
  HardDrive,
} from 'lucide-react';
import { Button } from './ui/button';
import { connectorManager } from '../services/connectorManager';
import {
  ConnectorConfig,
  ConnectorProvider,
  ConnectorStatus,
  CONNECTOR_PROVIDERS,
} from '../types/connectors';

interface ConnectorsSettingsProps {
  open: boolean;
  onClose: () => void;
}

export default function ConnectorsSettings({ open, onClose }: ConnectorsSettingsProps) {
  const [connectors, setConnectors] = useState<ConnectorConfig[]>([]);
  const [statuses, setStatuses] = useState<Map<string, ConnectorStatus>>(new Map());
  const [showAddDialog, setShowAddDialog] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  useEffect(() => {
    if (open) {
      loadConnectors();
    }
  }, [open]);

  const loadConnectors = async () => {
    setIsLoading(true);

    try {
      const configs = connectorManager.getAllConfigs();
      setConnectors(configs);

      // Load statuses
      const statusMap = new Map<string, ConnectorStatus>();
      for (const config of configs) {
        const status = await connectorManager.getStatus(config.id);
        if (status) {
          statusMap.set(config.id, status);
        }
      }
      setStatuses(statusMap);
    } catch (error) {
      console.error('Failed to load connectors:', error);
    } finally {
      setIsLoading(false);
    }
  };

  const handleRemoveConnector = async (connectorId: string) => {
    if (!confirm('Are you sure you want to remove this connector?')) {
      return;
    }

    try {
      await connectorManager.removeConnector(connectorId);
      await loadConnectors();
    } catch (error) {
      console.error('Failed to remove connector:', error);
      alert('Failed to remove connector');
    }
  };

  const handleToggleEnabled = async (connectorId: string, enabled: boolean) => {
    try {
      connectorManager.updateConfig(connectorId, { enabled });
      await loadConnectors();
    } catch (error) {
      console.error('Failed to toggle connector:', error);
    }
  };

  const handleTestConnection = async (connectorId: string) => {
    try {
      const success = await connectorManager.testConnection(connectorId);
      alert(success ? 'Connection successful!' : 'Connection failed');
      await loadConnectors();
    } catch (error) {
      console.error('Connection test failed:', error);
      alert('Connection test failed');
    }
  };

  const formatBytes = (bytes: number): string => {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return `${(bytes / Math.pow(k, i)).toFixed(2)} ${sizes[i]}`;
  };

  if (!open) return null;

  return (
    <AnimatePresence>
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        exit={{ opacity: 0 }}
        className="fixed inset-0 bg-black/50 backdrop-blur-sm z-50 flex items-center justify-center p-4"
        onClick={onClose}
      >
        <motion.div
          initial={{ scale: 0.95, opacity: 0 }}
          animate={{ scale: 1, opacity: 1 }}
          exit={{ scale: 0.95, opacity: 0 }}
          onClick={(e) => e.stopPropagation()}
          className="bg-card border border-border rounded-lg shadow-2xl w-full max-w-4xl max-h-[85vh] overflow-hidden flex flex-col"
        >
          {/* Header */}
          <div className="flex items-center justify-between p-6 border-b border-border">
            <div>
              <h2 className="text-2xl font-bold text-foreground flex items-center gap-2">
                <Settings className="h-6 w-6 text-primary" />
                Cloud Connectors
              </h2>
              <p className="text-sm text-muted-foreground mt-1">
                Connect your cloud storage accounts
              </p>
            </div>
            <div className="flex items-center gap-2">
              <Button
                onClick={() => setShowAddDialog(true)}
                className="bg-primary hover:bg-primary/90"
              >
                <Plus className="h-4 w-4 mr-2" />
                Add Connector
              </Button>
              <Button size="icon" variant="ghost" onClick={onClose}>
                <X className="h-5 w-5" />
              </Button>
            </div>
          </div>

          {/* Connector List */}
          <div className="flex-1 overflow-y-auto p-6">
            {isLoading ? (
              <div className="flex items-center justify-center py-12">
                <RefreshCw className="h-8 w-8 animate-spin text-primary" />
              </div>
            ) : connectors.length === 0 ? (
              <div className="text-center py-12">
                <HardDrive className="h-16 w-16 text-muted-foreground mx-auto mb-4" />
                <p className="text-muted-foreground text-lg">No connectors configured</p>
                <p className="text-sm text-muted-foreground mt-2">
                  Click "Add Connector" to get started
                </p>
              </div>
            ) : (
              <div className="space-y-4">
                {connectors.map((connector) => {
                  const status = statuses.get(connector.id);
                  const providerMeta = CONNECTOR_PROVIDERS[connector.provider];

                  return (
                    <motion.div
                      key={connector.id}
                      layout
                      initial={{ opacity: 0, y: 20 }}
                      animate={{ opacity: 1, y: 0 }}
                      className="bg-muted/50 border border-border rounded-lg p-4"
                    >
                      <div className="flex items-start gap-4">
                        {/* Icon */}
                        <div
                          className="flex-shrink-0 w-12 h-12 rounded-lg flex items-center justify-center text-2xl"
                          style={{ backgroundColor: `${providerMeta.color}20` }}
                        >
                          {providerMeta.icon}
                        </div>

                        {/* Info */}
                        <div className="flex-1 min-w-0">
                          <div className="flex items-center gap-2">
                            <h3 className="font-semibold text-foreground">{connector.name}</h3>
                            {status?.connected ? (
                              <CheckCircle className="h-4 w-4 text-green-500" />
                            ) : (
                              <AlertCircle className="h-4 w-4 text-red-500" />
                            )}
                          </div>
                          <p className="text-sm text-muted-foreground">
                            {providerMeta.name}
                          </p>

                          {status && (
                            <div className="mt-2 space-y-1">
                              {status.storageUsed !== undefined && (
                                <div className="text-xs text-muted-foreground">
                                  Storage: {formatBytes(status.storageUsed)} /{' '}
                                  {formatBytes(status.storageTotal || 0)}
                                </div>
                              )}
                              {status.lastSync && (
                                <div className="text-xs text-muted-foreground">
                                  Last sync:{' '}
                                  {new Date(status.lastSync).toLocaleString()}
                                </div>
                              )}
                              {status.lastError && (
                                <div className="text-xs text-red-500">
                                  Error: {status.lastError}
                                </div>
                              )}
                            </div>
                          )}
                        </div>

                        {/* Actions */}
                        <div className="flex items-center gap-2">
                          <Button
                            size="icon"
                            variant="ghost"
                            onClick={() =>
                              handleToggleEnabled(connector.id, !connector.enabled)
                            }
                            title={connector.enabled ? 'Disable' : 'Enable'}
                          >
                            <Power
                              className={`h-4 w-4 ${
                                connector.enabled ? 'text-green-500' : 'text-gray-400'
                              }`}
                            />
                          </Button>
                          <Button
                            size="icon"
                            variant="ghost"
                            onClick={() => handleTestConnection(connector.id)}
                            title="Test connection"
                          >
                            <RefreshCw className="h-4 w-4" />
                          </Button>
                          <Button
                            size="icon"
                            variant="ghost"
                            onClick={() => handleRemoveConnector(connector.id)}
                            className="text-red-500 hover:text-red-600 hover:bg-red-500/10"
                            title="Remove connector"
                          >
                            <Trash2 className="h-4 w-4" />
                          </Button>
                        </div>
                      </div>
                    </motion.div>
                  );
                })}
              </div>
            )}
          </div>

          {/* Footer */}
          <div className="p-6 border-t border-border bg-muted/30">
            <div className="flex items-center justify-between text-sm">
              <div className="text-muted-foreground">
                {connectors.filter((c) => c.enabled).length} of {connectors.length}{' '}
                connectors enabled
              </div>
              <div className="text-xs text-muted-foreground">
                Changes are saved automatically
              </div>
            </div>
          </div>
        </motion.div>
      </motion.div>

      {/* Add Connector Dialog */}
      {showAddDialog && (
        <AddConnectorDialog
          onClose={() => setShowAddDialog(false)}
          onAdd={async () => {
            setShowAddDialog(false);
            await loadConnectors();
          }}
        />
      )}
    </AnimatePresence>
  );
}

/**
 * Add Connector Dialog
 */
interface AddConnectorDialogProps {
  onClose: () => void;
  onAdd: () => void;
}

function AddConnectorDialog({ onClose, onAdd }: AddConnectorDialogProps) {
  const [provider, setProvider] = useState<ConnectorProvider>('google-drive');
  const [name, setName] = useState('');
  const [clientId, setClientId] = useState('');
  const [clientSecret, setClientSecret] = useState('');
  const [isAdding, setIsAdding] = useState(false);

  const handleAdd = async () => {
    if (!name.trim()) {
      alert('Please enter a connector name');
      return;
    }

    const providerMeta = CONNECTOR_PROVIDERS[provider];
    if (providerMeta.authType === 'oauth2' && !clientId.trim()) {
      alert('Please enter Client ID');
      return;
    }

    setIsAdding(true);

    try {
      const config: ConnectorConfig = {
        id: `connector-${Date.now()}`,
        name: name.trim(),
        provider,
        enabled: true,
        credentials: {
          clientId,
          clientSecret: clientSecret || undefined,
        },
        settings: {
          autoSync: false,
          syncInterval: 60000,
          defaultFolder: '',
          uploadQuality: 'original',
          conflictResolution: 'ask',
          cacheEnabled: true,
          maxCacheSize: 1024,
        },
        createdAt: Date.now(),
      };

      await connectorManager.addConnector(config);
      onAdd();
    } catch (error) {
      console.error('Failed to add connector:', error);
      alert(`Failed to add connector: ${error instanceof Error ? error.message : 'Unknown error'}`);
    } finally {
      setIsAdding(false);
    }
  };

  const providerMeta = CONNECTOR_PROVIDERS[provider];

  return (
    <motion.div
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      className="fixed inset-0 bg-black/70 z-[60] flex items-center justify-center p-4"
      onClick={onClose}
    >
      <motion.div
        initial={{ scale: 0.95, opacity: 0 }}
        animate={{ scale: 1, opacity: 1 }}
        exit={{ scale: 0.95, opacity: 0 }}
        onClick={(e) => e.stopPropagation()}
        className="bg-card border border-border rounded-lg shadow-2xl w-full max-w-md p-6"
      >
        <h3 className="text-xl font-bold text-foreground mb-4">Add Cloud Connector</h3>

        <div className="space-y-4">
          {/* Provider Selection */}
          <div>
            <label className="text-sm font-medium text-foreground mb-2 block">
              Provider
            </label>
            <select
              value={provider}
              onChange={(e) => setProvider(e.target.value as ConnectorProvider)}
              className="w-full px-3 py-2 bg-background border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary text-foreground"
            >
              {Object.values(CONNECTOR_PROVIDERS).map((p) => (
                <option key={p.id} value={p.id}>
                  {p.icon} {p.name}
                </option>
              ))}
            </select>
          </div>

          {/* Connector Name */}
          <div>
            <label className="text-sm font-medium text-foreground mb-2 block">
              Connector Name
            </label>
            <input
              type="text"
              placeholder="My Google Drive"
              value={name}
              onChange={(e) => setName(e.target.value)}
              className="w-full px-3 py-2 bg-background border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary text-foreground"
            />
          </div>

          {/* OAuth 2.0 Credentials */}
          {providerMeta.authType === 'oauth2' && (
            <>
              <div>
                <label className="text-sm font-medium text-foreground mb-2 block">
                  Client ID
                </label>
                <input
                  type="text"
                  placeholder="Your OAuth 2.0 Client ID"
                  value={clientId}
                  onChange={(e) => setClientId(e.target.value)}
                  className="w-full px-3 py-2 bg-background border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary text-foreground font-mono text-sm"
                />
              </div>
              <div>
                <label className="text-sm font-medium text-foreground mb-2 block">
                  Client Secret (Optional)
                </label>
                <input
                  type="password"
                  placeholder="Your OAuth 2.0 Client Secret"
                  value={clientSecret}
                  onChange={(e) => setClientSecret(e.target.value)}
                  className="w-full px-3 py-2 bg-background border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary text-foreground font-mono text-sm"
                />
              </div>
            </>
          )}

          {/* Info */}
          <div className="bg-blue-500/10 border border-blue-500/30 rounded-md p-3">
            <p className="text-xs text-blue-600 dark:text-blue-400">
              <strong>Note:</strong> You'll need to create OAuth credentials at{' '}
              <a
                href={providerMeta.website}
                target="_blank"
                rel="noopener noreferrer"
                className="underline"
              >
                {providerMeta.name}
              </a>
              . After adding the connector, you'll be prompted to authenticate.
            </p>
          </div>
        </div>

        {/* Actions */}
        <div className="flex justify-end gap-3 mt-6">
          <Button variant="ghost" onClick={onClose} disabled={isAdding}>
            Cancel
          </Button>
          <Button
            onClick={handleAdd}
            disabled={isAdding}
            className="bg-primary hover:bg-primary/90"
          >
            {isAdding ? (
              <>
                <RefreshCw className="h-4 w-4 mr-2 animate-spin" />
                Adding...
              </>
            ) : (
              <>
                <Plus className="h-4 w-4 mr-2" />
                Add Connector
              </>
            )}
          </Button>
        </div>
      </motion.div>
    </motion.div>
  );
}
