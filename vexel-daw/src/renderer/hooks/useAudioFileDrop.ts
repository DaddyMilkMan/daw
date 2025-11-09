// Hook for handling drag-and-drop of audio files

import { useState, useCallback, DragEvent } from 'react';

export interface DropZoneState {
  isDragging: boolean;
  isOver: boolean;
}

export function useAudioFileDrop(
  onFilesDropped: (files: File[], x: number, y: number) => void
) {
  const [state, setState] = useState<DropZoneState>({
    isDragging: false,
    isOver: false,
  });

  const isAudioFile = (file: File): boolean => {
    return (
      file.type.startsWith('audio/') ||
      /\.(mp3|wav|ogg|flac|m4a|aac|wma|aiff)$/i.test(file.name)
    );
  };

  const handleDragEnter = useCallback((e: DragEvent) => {
    e.preventDefault();
    e.stopPropagation();

    // Check if dragging contains files
    if (e.dataTransfer.items && e.dataTransfer.items.length > 0) {
      setState({ isDragging: true, isOver: true });
    }
  }, []);

  const handleDragLeave = useCallback((e: DragEvent) => {
    e.preventDefault();
    e.stopPropagation();

    // Only set isOver to false if leaving the main container
    if (e.currentTarget === e.target) {
      setState((prev) => ({ ...prev, isOver: false }));
    }
  }, []);

  const handleDragOver = useCallback((e: DragEvent) => {
    e.preventDefault();
    e.stopPropagation();

    // Set dropEffect to copy
    e.dataTransfer.dropEffect = 'copy';
  }, []);

  const handleDrop = useCallback(
    (e: DragEvent) => {
      e.preventDefault();
      e.stopPropagation();

      setState({ isDragging: false, isOver: false });

      const files: File[] = [];

      // Get files from dataTransfer
      if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
        for (let i = 0; i < e.dataTransfer.files.length; i++) {
          const file = e.dataTransfer.files[i];
          if (isAudioFile(file)) {
            files.push(file);
          }
        }
      }

      if (files.length > 0) {
        // Get drop position relative to the container
        const rect = e.currentTarget.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        onFilesDropped(files, x, y);
      }
    },
    [onFilesDropped]
  );

  const handleDragEnd = useCallback(() => {
    setState({ isDragging: false, isOver: false });
  }, []);

  return {
    state,
    handlers: {
      onDragEnter: handleDragEnter,
      onDragLeave: handleDragLeave,
      onDragOver: handleDragOver,
      onDrop: handleDrop,
      onDragEnd: handleDragEnd,
    },
  };
}
