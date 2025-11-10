/**
 * TrackManagement.tsx
 * Dialogs and utilities for track operations (rename, duplicate, delete, color picker)
 */

import { useState, useRef, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { X, Check, Trash2, AlertTriangle } from 'lucide-react';
import { HexColorPicker } from 'react-colorful';
import { Button } from './ui/button';

// Track Color Picker Dialog
interface ColorPickerDialogProps {
  isOpen: boolean;
  currentColor: string;
  trackName: string;
  onClose: () => void;
  onColorChange: (color: string) => void;
}

export function ColorPickerDialog({
  isOpen,
  currentColor,
  trackName,
  onClose,
  onColorChange,
}: ColorPickerDialogProps) {
  const [selectedColor, setSelectedColor] = useState(currentColor);

  useEffect(() => {
    setSelectedColor(currentColor);
  }, [currentColor]);

  const handleSave = () => {
    onColorChange(selectedColor);
    onClose();
  };

  if (!isOpen) return null;

  const presetColors = [
    '#ef4444', '#f97316', '#f59e0b', '#eab308',
    '#84cc16', '#22c55e', '#10b981', '#14b8a6',
    '#06b6d4', '#0ea5e9', '#3b82f6', '#6366f1',
    '#8b5cf6', '#a855f7', '#d946ef', '#ec4899',
  ];

  return (
    <AnimatePresence>
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        exit={{ opacity: 0 }}
        className="fixed inset-0 bg-black/50 backdrop-blur-sm flex items-center justify-center z-50"
        onClick={onClose}
      >
        <motion.div
          initial={{ scale: 0.9, opacity: 0 }}
          animate={{ scale: 1, opacity: 1 }}
          exit={{ scale: 0.9, opacity: 0 }}
          onClick={(e) => e.stopPropagation()}
          className="bg-card border border-border rounded-lg p-6 shadow-2xl max-w-sm w-full"
        >
          <div className="flex items-center justify-between mb-4">
            <h3 className="text-lg font-semibold">Change Track Color</h3>
            <button
              onClick={onClose}
              className="text-muted-foreground hover:text-foreground transition-colors"
            >
              <X className="h-5 w-5" />
            </button>
          </div>

          <div className="text-sm text-muted-foreground mb-4">
            Track: <span className="font-medium text-foreground">{trackName}</span>
          </div>

          {/* Color Picker */}
          <div className="mb-4">
            <HexColorPicker color={selectedColor} onChange={setSelectedColor} className="w-full" />
          </div>

          {/* Preset Colors */}
          <div className="mb-4">
            <div className="text-sm font-medium mb-2">Preset Colors</div>
            <div className="grid grid-cols-8 gap-2">
              {presetColors.map((color) => (
                <button
                  key={color}
                  onClick={() => setSelectedColor(color)}
                  className={`w-8 h-8 rounded border-2 transition-all ${
                    selectedColor === color ? 'border-foreground scale-110' : 'border-transparent'
                  }`}
                  style={{ backgroundColor: color }}
                />
              ))}
            </div>
          </div>

          {/* Color Preview */}
          <div className="flex items-center gap-3 mb-4 p-3 bg-background/50 rounded">
            <div
              className="w-12 h-12 rounded border border-border"
              style={{ backgroundColor: selectedColor }}
            />
            <div className="flex-1">
              <div className="text-xs text-muted-foreground">Selected Color</div>
              <div className="font-mono text-sm">{selectedColor}</div>
            </div>
          </div>

          {/* Actions */}
          <div className="flex gap-2">
            <Button variant="outline" onClick={onClose} className="flex-1">
              Cancel
            </Button>
            <Button onClick={handleSave} className="flex-1">
              <Check className="h-4 w-4 mr-2" />
              Apply
            </Button>
          </div>
        </motion.div>
      </motion.div>
    </AnimatePresence>
  );
}

// Track Rename Dialog
interface RenameDialogProps {
  isOpen: boolean;
  currentName: string;
  onClose: () => void;
  onRename: (newName: string) => void;
}

export function RenameDialog({ isOpen, currentName, onClose, onRename }: RenameDialogProps) {
  const [name, setName] = useState(currentName);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    setName(currentName);
    if (isOpen && inputRef.current) {
      inputRef.current.focus();
      inputRef.current.select();
    }
  }, [isOpen, currentName]);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (name.trim()) {
      onRename(name.trim());
      onClose();
    }
  };

  if (!isOpen) return null;

  return (
    <AnimatePresence>
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        exit={{ opacity: 0 }}
        className="fixed inset-0 bg-black/50 backdrop-blur-sm flex items-center justify-center z-50"
        onClick={onClose}
      >
        <motion.div
          initial={{ scale: 0.9, opacity: 0 }}
          animate={{ scale: 1, opacity: 1 }}
          exit={{ scale: 0.9, opacity: 0 }}
          onClick={(e) => e.stopPropagation()}
          className="bg-card border border-border rounded-lg p-6 shadow-2xl max-w-md w-full"
        >
          <div className="flex items-center justify-between mb-4">
            <h3 className="text-lg font-semibold">Rename Track</h3>
            <button
              onClick={onClose}
              className="text-muted-foreground hover:text-foreground transition-colors"
            >
              <X className="h-5 w-5" />
            </button>
          </div>

          <form onSubmit={handleSubmit}>
            <input
              ref={inputRef}
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              className="w-full px-3 py-2 bg-background border border-border rounded-md focus:outline-none focus:ring-2 focus:ring-primary"
              placeholder="Enter track name"
            />

            <div className="flex gap-2 mt-4">
              <Button type="button" variant="outline" onClick={onClose} className="flex-1">
                Cancel
              </Button>
              <Button type="submit" className="flex-1">
                <Check className="h-4 w-4 mr-2" />
                Rename
              </Button>
            </div>
          </form>
        </motion.div>
      </motion.div>
    </AnimatePresence>
  );
}

// Delete Confirmation Dialog
interface DeleteConfirmDialogProps {
  isOpen: boolean;
  trackName: string;
  onClose: () => void;
  onConfirm: () => void;
}

export function DeleteConfirmDialog({
  isOpen,
  trackName,
  onClose,
  onConfirm,
}: DeleteConfirmDialogProps) {
  if (!isOpen) return null;

  return (
    <AnimatePresence>
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        exit={{ opacity: 0 }}
        className="fixed inset-0 bg-black/50 backdrop-blur-sm flex items-center justify-center z-50"
        onClick={onClose}
      >
        <motion.div
          initial={{ scale: 0.9, opacity: 0 }}
          animate={{ scale: 1, opacity: 1 }}
          exit={{ scale: 0.9, opacity: 0 }}
          onClick={(e) => e.stopPropagation()}
          className="bg-card border border-border rounded-lg p-6 shadow-2xl max-w-md w-full"
        >
          <div className="flex items-start gap-3 mb-4">
            <div className="w-10 h-10 rounded-full bg-red-500/20 flex items-center justify-center flex-shrink-0">
              <AlertTriangle className="h-5 w-5 text-red-500" />
            </div>
            <div>
              <h3 className="text-lg font-semibold mb-1">Delete Track</h3>
              <p className="text-sm text-muted-foreground">
                Are you sure you want to delete <span className="font-medium text-foreground">"{trackName}"</span>?
              </p>
              <p className="text-sm text-muted-foreground mt-2">
                All clips and automation on this track will be permanently removed.
              </p>
            </div>
          </div>

          <div className="flex gap-2">
            <Button variant="outline" onClick={onClose} className="flex-1">
              Cancel
            </Button>
            <Button
              variant="destructive"
              onClick={() => {
                onConfirm();
                onClose();
              }}
              className="flex-1"
            >
              <Trash2 className="h-4 w-4 mr-2" />
              Delete
            </Button>
          </div>
        </motion.div>
      </motion.div>
    </AnimatePresence>
  );
}
