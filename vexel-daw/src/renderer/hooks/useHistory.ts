import { useState, useCallback, useRef } from 'react';

export interface HistoryAction<T> {
  type: string;
  timestamp: Date;
  undo: () => void;
  redo: () => void;
  state: T;
}

interface UseHistoryOptions {
  maxSize?: number;
}

export function useHistory<T>(initialState: T, options: UseHistoryOptions = {}) {
  const { maxSize = 100 } = options;

  const [state, setState] = useState<T>(initialState);
  const [history, setHistory] = useState<HistoryAction<T>[]>([]);
  const [currentIndex, setCurrentIndex] = useState(-1);
  const isUndoRedoRef = useRef(false);

  const canUndo = currentIndex >= 0;
  const canRedo = currentIndex < history.length - 1;

  const pushHistory = useCallback(
    (action: Omit<HistoryAction<T>, 'timestamp'>) => {
      if (isUndoRedoRef.current) return;

      setHistory((prev) => {
        // Remove any history after current index
        const newHistory = prev.slice(0, currentIndex + 1);

        // Add new action
        newHistory.push({
          ...action,
          timestamp: new Date(),
        });

        // Limit history size
        if (newHistory.length > maxSize) {
          newHistory.shift();
          setCurrentIndex((idx) => idx - 1);
        } else {
          setCurrentIndex((idx) => idx + 1);
        }

        return newHistory;
      });
    },
    [currentIndex, maxSize]
  );

  const undo = useCallback(() => {
    if (!canUndo) return;

    isUndoRedoRef.current = true;
    const action = history[currentIndex];
    action.undo();
    setState(action.state);
    setCurrentIndex((idx) => idx - 1);
    isUndoRedoRef.current = false;

    console.log(`↩️ Undo: ${action.type}`);
  }, [canUndo, currentIndex, history]);

  const redo = useCallback(() => {
    if (!canRedo) return;

    isUndoRedoRef.current = true;
    const action = history[currentIndex + 1];
    action.redo();
    setState(action.state);
    setCurrentIndex((idx) => idx + 1);
    isUndoRedoRef.current = false;

    console.log(`↪️ Redo: ${action.type}`);
  }, [canRedo, currentIndex, history]);

  const clearHistory = useCallback(() => {
    setHistory([]);
    setCurrentIndex(-1);
    console.log('🗑️ History cleared');
  }, []);

  const updateState = useCallback(
    (newState: T, actionType: string, undoFn: () => void, redoFn: () => void) => {
      setState(newState);
      pushHistory({
        type: actionType,
        state: newState,
        undo: undoFn,
        redo: redoFn,
      });
    },
    [pushHistory]
  );

  return {
    state,
    setState: updateState,
    undo,
    redo,
    canUndo,
    canRedo,
    history,
    currentIndex,
    clearHistory,
  };
}
