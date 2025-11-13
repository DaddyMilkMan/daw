/**
 * Save To Menu Component
 * Dropdown menu for saving to different cloud storage connectors
 *
 * Features:
 * - Shows when connectors are configured
 * - Lists enabled connectors
 * - Save to local or cloud
 * - Visual feedback during save
 */

import { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Save,
  ChevronRight,
  HardDrive,
  Cloud,
  CheckCircle,
  Loader,
} from 'lucide-react';
import { connectorManager } from '../services/connectorManager';
import { ConnectorConfig, CONNECTOR_PROVIDERS } from '../types/connectors';

interface SaveToMenuProps {
  onSaveLocal: () => void | Promise<void>;
  onSaveToConnector: (connectorId: string) => void | Promise<void>;
  projectName?: string;
}

export default function SaveToMenu({
  onSaveLocal,
  onSaveToConnector,
  projectName = 'Project',
}: SaveToMenuProps) {
  const [connectors, setConnectors] = useState<ConnectorConfig[]>([]);
  const [showSubmenu, setShowSubmenu] = useState(false);
  const [saving, setSaving] = useState<string | null>(null);

  useEffect(() => {
    loadConnectors();
  }, []);

  const loadConnectors = () => {
    const configs = connectorManager.getEnabledConnectors();
    setConnectors(configs.map((c) => c.config));
  };

  const handleSaveLocal = async () => {
    setSaving('local');
    try {
      await onSaveLocal();
      // Show success briefly
      setTimeout(() => setSaving(null), 1000);
    } catch (error) {
      console.error('Save failed:', error);
      setSaving(null);
      alert('Save failed');
    }
  };

  const handleSaveToConnector = async (connectorId: string) => {
    setSaving(connectorId);
    setShowSubmenu(false);
    try {
      await onSaveToConnector(connectorId);
      // Show success briefly
      setTimeout(() => setSaving(null), 1000);
    } catch (error) {
      console.error('Save to cloud failed:', error);
      setSaving(null);
      alert('Save to cloud failed');
    }
  };

  const hasConnectors = connectors.length > 0;

  return (
    <div className="relative">
      {/* Main Save Button */}
      {!hasConnectors ? (
        // Simple save when no connectors
        <button
          onClick={handleSaveLocal}
          disabled={!!saving}
          className="flex items-center gap-2 px-4 py-2 text-sm text-foreground hover:bg-muted rounded-md transition-colors disabled:opacity-50"
        >
          {saving === 'local' ? (
            <>
              <Loader className="h-4 w-4 animate-spin" />
              Saving...
            </>
          ) : (
            <>
              <Save className="h-4 w-4" />
              Save
            </>
          )}
        </button>
      ) : (
        // Save with submenu when connectors exist
        <div
          className="relative"
          onMouseEnter={() => setShowSubmenu(true)}
          onMouseLeave={() => setShowSubmenu(false)}
        >
          <button
            className="flex items-center gap-2 px-4 py-2 text-sm text-foreground hover:bg-muted rounded-md transition-colors w-full"
            disabled={!!saving}
          >
            {saving ? (
              <>
                <Loader className="h-4 w-4 animate-spin" />
                Saving...
              </>
            ) : (
              <>
                <Save className="h-4 w-4" />
                Save
                <ChevronRight className="h-4 w-4 ml-auto" />
              </>
            )}
          </button>

          {/* Submenu */}
          <AnimatePresence>
            {showSubmenu && !saving && (
              <motion.div
                initial={{ opacity: 0, x: -10 }}
                animate={{ opacity: 1, x: 0 }}
                exit={{ opacity: 0, x: -10 }}
                transition={{ duration: 0.15 }}
                className="absolute left-full top-0 ml-1 bg-card border border-border rounded-lg shadow-xl py-2 min-w-[200px] z-50"
                onClick={(e) => e.stopPropagation()}
              >
                {/* Save Locally */}
                <button
                  onClick={handleSaveLocal}
                  className="w-full flex items-center gap-3 px-4 py-2 text-sm text-foreground hover:bg-muted transition-colors"
                >
                  <HardDrive className="h-4 w-4" />
                  <span className="flex-1 text-left">Save Locally</span>
                  {saving === 'local' && <Loader className="h-4 w-4 animate-spin" />}
                </button>

                <div className="border-t border-border my-2" />

                {/* Cloud Connectors */}
                <div className="px-3 py-1">
                  <p className="text-xs font-semibold text-muted-foreground uppercase tracking-wide">
                    Cloud Storage
                  </p>
                </div>

                {connectors.map((connector) => {
                  const providerMeta = CONNECTOR_PROVIDERS[connector.provider];

                  return (
                    <button
                      key={connector.id}
                      onClick={() => handleSaveToConnector(connector.id)}
                      className="w-full flex items-center gap-3 px-4 py-2 text-sm text-foreground hover:bg-muted transition-colors"
                    >
                      <span className="text-base">{providerMeta.icon}</span>
                      <span className="flex-1 text-left">{connector.name}</span>
                      {saving === connector.id && (
                        <Loader className="h-4 w-4 animate-spin" />
                      )}
                    </button>
                  );
                })}

                {connectors.length === 0 && (
                  <div className="px-4 py-2 text-sm text-muted-foreground">
                    No connectors enabled
                  </div>
                )}
              </motion.div>
            )}
          </AnimatePresence>
        </div>
      )}

      {/* Success Indicator */}
      <AnimatePresence>
        {saving && (
          <motion.div
            initial={{ opacity: 0, scale: 0.8 }}
            animate={{ opacity: 1, scale: 1 }}
            exit={{ opacity: 0, scale: 0.8 }}
            className="absolute -right-8 top-1/2 -translate-y-1/2"
          >
            <CheckCircle className="h-5 w-5 text-green-500" />
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}

/**
 * Standalone Save To Button with Dropdown
 * For use in toolbars and quick access
 */
export function SaveToButton({
  onSaveLocal,
  onSaveToConnector,
  projectName,
}: SaveToMenuProps) {
  const [connectors, setConnectors] = useState<ConnectorConfig[]>([]);
  const [isOpen, setIsOpen] = useState(false);
  const [saving, setSaving] = useState<string | null>(null);

  useEffect(() => {
    const configs = connectorManager.getEnabledConnectors();
    setConnectors(configs.map((c) => c.config));
  }, []);

  const handleSaveLocal = async () => {
    setSaving('local');
    setIsOpen(false);
    try {
      await onSaveLocal();
      setTimeout(() => setSaving(null), 1000);
    } catch (error) {
      console.error('Save failed:', error);
      setSaving(null);
      alert('Save failed');
    }
  };

  const handleSaveToConnector = async (connectorId: string) => {
    setSaving(connectorId);
    setIsOpen(false);
    try {
      await onSaveToConnector(connectorId);
      setTimeout(() => setSaving(null), 1000);
    } catch (error) {
      console.error('Save to cloud failed:', error);
      setSaving(null);
      alert('Save to cloud failed');
    }
  };

  const hasConnectors = connectors.length > 0;

  return (
    <div className="relative">
      <button
        onClick={() => (hasConnectors ? setIsOpen(!isOpen) : handleSaveLocal())}
        disabled={!!saving}
        className="flex items-center gap-2 px-4 py-2 bg-primary text-primary-foreground rounded-md hover:bg-primary/90 transition-colors disabled:opacity-50"
      >
        {saving ? (
          <>
            <Loader className="h-4 w-4 animate-spin" />
            Saving...
          </>
        ) : (
          <>
            <Save className="h-4 w-4" />
            Save
            {hasConnectors && <ChevronRight className="h-3 w-3" />}
          </>
        )}
      </button>

      {/* Dropdown */}
      <AnimatePresence>
        {isOpen && hasConnectors && (
          <>
            {/* Backdrop */}
            <div
              className="fixed inset-0 z-40"
              onClick={() => setIsOpen(false)}
            />

            {/* Menu */}
            <motion.div
              initial={{ opacity: 0, y: -10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              transition={{ duration: 0.15 }}
              className="absolute top-full mt-2 right-0 bg-card border border-border rounded-lg shadow-xl py-2 min-w-[200px] z-50"
            >
              {/* Save Locally */}
              <button
                onClick={handleSaveLocal}
                className="w-full flex items-center gap-3 px-4 py-2 text-sm text-foreground hover:bg-muted transition-colors"
              >
                <HardDrive className="h-4 w-4" />
                <span className="flex-1 text-left">Save Locally</span>
              </button>

              {connectors.length > 0 && (
                <>
                  <div className="border-t border-border my-2" />

                  <div className="px-3 py-1">
                    <p className="text-xs font-semibold text-muted-foreground uppercase tracking-wide">
                      Cloud Storage
                    </p>
                  </div>

                  {connectors.map((connector) => {
                    const providerMeta = CONNECTOR_PROVIDERS[connector.provider];

                    return (
                      <button
                        key={connector.id}
                        onClick={() => handleSaveToConnector(connector.id)}
                        className="w-full flex items-center gap-3 px-4 py-2 text-sm text-foreground hover:bg-muted transition-colors"
                      >
                        <span className="text-base">{providerMeta.icon}</span>
                        <span className="flex-1 text-left">{connector.name}</span>
                      </button>
                    );
                  })}
                </>
              )}
            </motion.div>
          </>
        )}
      </AnimatePresence>

      {/* Success Indicator */}
      <AnimatePresence>
        {saving && (
          <motion.div
            initial={{ opacity: 0, scale: 0.8 }}
            animate={{ opacity: 1, scale: 1 }}
            exit={{ opacity: 0, scale: 0.8 }}
            className="absolute -right-8 top-1/2 -translate-y-1/2"
          >
            <CheckCircle className="h-5 w-5 text-green-500" />
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}
