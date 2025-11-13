import { useState, useRef, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';

interface FileMenuProps {
  onNewProject: () => void;
  onSaveProject: () => void;
  onLoadProject: () => void;
}

export default function FileMenu({ onNewProject, onSaveProject, onLoadProject }: FileMenuProps) {
  const [isOpen, setIsOpen] = useState(false);
  const menuRef = useRef<HTMLDivElement>(null);

  // Close menu when clicking outside
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (menuRef.current && !menuRef.current.contains(event.target as Node)) {
        setIsOpen(false);
      }
    };

    if (isOpen) {
      document.addEventListener('mousedown', handleClickOutside);
    }

    return () => {
      document.removeEventListener('mousedown', handleClickOutside);
    };
  }, [isOpen]);

  const handleMenuItemClick = (action: () => void) => {
    action();
    setIsOpen(false);
  };

  const isMac = navigator.platform.toUpperCase().includes('MAC');
  const cmdKey = isMac ? '⌘' : 'Ctrl';

  return (
    <div className="relative" ref={menuRef}>
      {/* File Menu Button */}
      <button
        onClick={() => setIsOpen(!isOpen)}
        className={`px-3 py-1 text-sm rounded hover:bg-white/10 transition-colors ${
          isOpen ? 'bg-white/10' : ''
        }`}
      >
        File
      </button>

      {/* Dropdown Menu */}
      <AnimatePresence>
        {isOpen && (
          <motion.div
            initial={{ opacity: 0, y: -10 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -10 }}
            transition={{ duration: 0.15 }}
            className="absolute left-0 top-full mt-1 w-64 bg-[#2a2a2a] border border-border rounded-md shadow-xl overflow-hidden z-50"
          >
            <div className="py-1">
              {/* New Project */}
              <button
                onClick={() => handleMenuItemClick(onNewProject)}
                className="w-full px-4 py-2 text-left text-sm hover:bg-white/10 transition-colors flex items-center justify-between group"
              >
                <div className="flex items-center gap-3">
                  <span className="text-lg">🆕</span>
                  <span>New Project</span>
                </div>
                <span className="text-xs text-muted-foreground group-hover:text-foreground">
                  {cmdKey}+N
                </span>
              </button>

              {/* Divider */}
              <div className="h-px bg-border my-1" />

              {/* Open/Load Project */}
              <button
                onClick={() => handleMenuItemClick(onLoadProject)}
                className="w-full px-4 py-2 text-left text-sm hover:bg-white/10 transition-colors flex items-center justify-between group"
              >
                <div className="flex items-center gap-3">
                  <span className="text-lg">📂</span>
                  <span>Open Project...</span>
                </div>
                <span className="text-xs text-muted-foreground group-hover:text-foreground">
                  {cmdKey}+O
                </span>
              </button>

              {/* Divider */}
              <div className="h-px bg-border my-1" />

              {/* Save Project */}
              <button
                onClick={() => handleMenuItemClick(onSaveProject)}
                className="w-full px-4 py-2 text-left text-sm hover:bg-white/10 transition-colors flex items-center justify-between group"
              >
                <div className="flex items-center gap-3">
                  <span className="text-lg">💾</span>
                  <span>Save Project...</span>
                </div>
                <span className="text-xs text-muted-foreground group-hover:text-foreground">
                  {cmdKey}+S
                </span>
              </button>

              {/* Divider */}
              <div className="h-px bg-border my-1" />

              {/* Info Section */}
              <div className="px-4 py-2 text-xs text-muted-foreground">
                <p>Project files use .vxl extension</p>
              </div>
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}
