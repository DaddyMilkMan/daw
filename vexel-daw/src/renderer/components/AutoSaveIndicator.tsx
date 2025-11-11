/**
 * Auto-Save Status Indicator
 *
 * Shows the current auto-save status in the UI with visual feedback:
 * - Idle: Hidden or dimmed
 * - Saving: Animated spinner
 * - Saved: Check mark with fade-out
 * - Error: Warning icon with error message
 */

import React, { useEffect, useState } from 'react';
import { autoSaveService, AutoSaveStatus } from '../services/autoSave';
import { Cloud, CloudOff, Check, AlertCircle, Loader2 } from 'lucide-react';

interface AutoSaveState {
  status: AutoSaveStatus;
  lastSaveTime: number | null;
  errorMessage: string | null;
  isDirty: boolean;
}

export function AutoSaveIndicator() {
  const [state, setState] = useState<AutoSaveState>(autoSaveService.getState());

  useEffect(() => {
    // Subscribe to auto-save status changes
    const unsubscribe = autoSaveService.onStatusChange(setState);
    return unsubscribe;
  }, []);

  // Don't show anything if idle and not dirty
  if (state.status === 'idle' && !state.isDirty) {
    return null;
  }

  // Format last save time
  const formatLastSave = (timestamp: number | null): string => {
    if (!timestamp) return '';

    const now = Date.now();
    const diff = now - timestamp;

    if (diff < 60000) {
      // Less than 1 minute
      return 'just now';
    } else if (diff < 3600000) {
      // Less than 1 hour
      const minutes = Math.floor(diff / 60000);
      return `${minutes}m ago`;
    } else {
      // Show time
      return new Date(timestamp).toLocaleTimeString([], {
        hour: '2-digit',
        minute: '2-digit',
      });
    }
  };

  return (
    <div className="fixed bottom-4 right-4 z-50">
      <div
        className={`
          flex items-center gap-2 px-3 py-2 rounded-lg
          text-sm font-medium transition-all duration-300
          ${
            state.status === 'saving'
              ? 'bg-blue-500/20 text-blue-400 border border-blue-500/30'
              : state.status === 'saved'
              ? 'bg-green-500/20 text-green-400 border border-green-500/30'
              : state.status === 'error'
              ? 'bg-red-500/20 text-red-400 border border-red-500/30'
              : 'bg-gray-500/10 text-gray-400 border border-gray-500/20'
          }
        `}
      >
        {/* Icon */}
        {state.status === 'saving' && (
          <Loader2 className="h-4 w-4 animate-spin" />
        )}
        {state.status === 'saved' && <Check className="h-4 w-4" />}
        {state.status === 'error' && <AlertCircle className="h-4 w-4" />}
        {state.status === 'idle' && state.isDirty && (
          <Cloud className="h-4 w-4" />
        )}

        {/* Text */}
        <span>
          {state.status === 'saving' && 'Saving...'}
          {state.status === 'saved' &&
            `Saved ${formatLastSave(state.lastSaveTime)}`}
          {state.status === 'error' && 'Save failed'}
          {state.status === 'idle' && state.isDirty && 'Unsaved changes'}
        </span>

        {/* Error details (if error) */}
        {state.status === 'error' && state.errorMessage && (
          <button
            className="ml-2 text-xs opacity-70 hover:opacity-100 transition-opacity"
            title={state.errorMessage}
            onClick={() => {
              alert(`Auto-save error:\n\n${state.errorMessage}`);
            }}
          >
            Details
          </button>
        )}

        {/* Manual save button (if dirty or error) */}
        {(state.isDirty || state.status === 'error') && (
          <button
            className="ml-2 px-2 py-0.5 bg-white/10 hover:bg-white/20 rounded text-xs transition-colors"
            onClick={async () => {
              try {
                await autoSaveService.forceSave();
              } catch (error) {
                console.error('Manual save failed:', error);
              }
            }}
          >
            Save Now
          </button>
        )}
      </div>
    </div>
  );
}

/**
 * Auto-Save Settings Component
 * Allows user to enable/disable auto-save and configure interval
 */
export function AutoSaveSettings() {
  const [isEnabled, setIsEnabled] = useState(
    autoSaveService.isAutoSaveEnabled()
  );
  const [interval, setInterval] = useState(5000);

  const handleToggle = () => {
    if (isEnabled) {
      autoSaveService.disable();
      setIsEnabled(false);
    } else {
      autoSaveService.enable(interval);
      setIsEnabled(true);
    }
  };

  const handleIntervalChange = (newInterval: number) => {
    setInterval(newInterval);
    if (isEnabled) {
      // Restart with new interval
      autoSaveService.disable();
      autoSaveService.enable(newInterval);
    }
  };

  return (
    <div className="space-y-4 p-4 bg-card border border-border rounded-lg">
      <div className="flex items-center justify-between">
        <div>
          <h3 className="text-sm font-semibold">Auto-Save</h3>
          <p className="text-xs text-muted-foreground">
            Automatically save your project in the background
          </p>
        </div>
        <button
          onClick={handleToggle}
          className={`
            relative inline-flex h-6 w-11 items-center rounded-full
            transition-colors
            ${isEnabled ? 'bg-blue-500' : 'bg-gray-600'}
          `}
        >
          <span
            className={`
              inline-block h-4 w-4 transform rounded-full bg-white
              transition-transform
              ${isEnabled ? 'translate-x-6' : 'translate-x-1'}
            `}
          />
        </button>
      </div>

      {isEnabled && (
        <div>
          <label className="text-xs text-muted-foreground">
            Save Interval: {interval / 1000}s
          </label>
          <input
            type="range"
            min="3000"
            max="30000"
            step="1000"
            value={interval}
            onChange={(e) => handleIntervalChange(Number(e.target.value))}
            className="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer"
          />
          <div className="flex justify-between text-xs text-muted-foreground mt-1">
            <span>3s</span>
            <span>15s</span>
            <span>30s</span>
          </div>
        </div>
      )}
    </div>
  );
}
