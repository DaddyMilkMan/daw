import { useState, useRef, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Send, Sparkles, X, Mic, Loader2, Settings, ChevronLeft, ChevronRight } from 'lucide-react';
import { Button } from './ui/button';

interface Message {
  id: string;
  role: 'user' | 'assistant';
  content: string;
  timestamp: number;
  actions?: Array<{ label: string; onClick: () => void }>;
}

interface WingmanSidebarProps {
  isOpen: boolean;
  onClose: () => void;
  position: 'left' | 'right';
  onPositionChange: (position: 'left' | 'right') => void;
}

export default function WingmanSidebar({ isOpen, onClose, position, onPositionChange }: WingmanSidebarProps) {
  const [messages, setMessages] = useState<Message[]>([
    {
      id: '1',
      role: 'assistant',
      content: "Hey! I'm Wingman, your AI music assistant. I can help you create beats, generate MIDI, suggest chords, and control your DAW. What would you like to make?",
      timestamp: Date.now(),
    },
  ]);
  const [input, setInput] = useState('');
  const [isProcessing, setIsProcessing] = useState(false);
  const [showSettings, setShowSettings] = useState(false);
  const messagesEndRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    if (isOpen && inputRef.current) {
      inputRef.current.focus();
    }
  }, [isOpen]);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  const handleSend = async () => {
    if (!input.trim() || isProcessing) return;

    const userMessage: Message = {
      id: Date.now().toString(),
      role: 'user',
      content: input,
      timestamp: Date.now(),
    };

    setMessages((prev) => [...prev, userMessage]);
    setInput('');
    setIsProcessing(true);

    // Simulate AI response with actions
    setTimeout(() => {
      const response = generateMockResponse(input);
      const aiMessage: Message = {
        id: (Date.now() + 1).toString(),
        role: 'assistant',
        content: response.message,
        timestamp: Date.now(),
        actions: response.actions,
      };
      setMessages((prev) => [...prev, aiMessage]);
      setIsProcessing(false);

      // Execute mock DAW actions
      if (response.dawActions) {
        response.dawActions.forEach((action: any) => {
          if (action.type === 'setTempo') {
            window.electron.setTempo(action.value);
          } else if (action.type === 'createTrack') {
            window.electron.createTrack(action.name, action.trackType);
          }
        });
      }
    }, 800);
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  const sidebarVariants = {
    open: {
      x: 0,
      transition: {
        type: 'spring',
        stiffness: 300,
        damping: 30,
      },
    },
    closed: {
      x: position === 'left' ? -400 : 400,
      transition: {
        type: 'spring',
        stiffness: 300,
        damping: 30,
      },
    },
  };

  return (
    <>
      {/* Backdrop */}
      <AnimatePresence>
        {isOpen && (
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            transition={{ duration: 0.2 }}
            className="fixed inset-0 bg-black/40 backdrop-blur-sm z-40"
            onClick={onClose}
          />
        )}
      </AnimatePresence>

      {/* Sidebar */}
      <motion.div
        initial="closed"
        animate={isOpen ? 'open' : 'closed'}
        variants={sidebarVariants}
        className={`fixed top-0 ${position === 'left' ? 'left-0' : 'right-0'} h-full w-[400px] bg-gradient-to-b from-card to-background border-${position === 'left' ? 'r' : 'l'} border-border shadow-2xl z-50 flex flex-col`}
      >
        {/* Header */}
        <div className="h-16 border-b border-border/50 flex items-center justify-between px-6 bg-gradient-to-r from-primary/5 to-transparent backdrop-blur-xl">
          <div className="flex items-center gap-3">
            <motion.div
              className="w-10 h-10 rounded-xl bg-gradient-to-br from-primary to-primary/70 flex items-center justify-center shadow-lg"
              animate={{
                boxShadow: isProcessing
                  ? '0 0 20px rgba(59, 130, 246, 0.5)'
                  : '0 0 0px rgba(59, 130, 246, 0)',
              }}
              transition={{ duration: 0.3 }}
            >
              <Sparkles className="h-5 w-5 text-white" />
            </motion.div>
            <div>
              <h3 className="font-semibold text-base">Wingman AI</h3>
              <motion.p
                className="text-xs text-muted-foreground"
                animate={{ opacity: isProcessing ? [1, 0.5, 1] : 1 }}
                transition={{ repeat: isProcessing ? Infinity : 0, duration: 1.5 }}
              >
                {isProcessing ? 'Thinking...' : 'Ready to help'}
              </motion.p>
            </div>
          </div>
          <div className="flex items-center gap-1">
            <Button
              size="icon"
              variant="ghost"
              onClick={() => setShowSettings(!showSettings)}
              title="Settings"
              className="hover:bg-white/5"
            >
              <Settings className="h-4 w-4" />
            </Button>
            <Button
              size="icon"
              variant="ghost"
              onClick={onClose}
              title="Close"
              className="hover:bg-white/5"
            >
              <X className="h-4 w-4" />
            </Button>
          </div>
        </div>

        {/* Settings Panel */}
        <AnimatePresence>
          {showSettings && (
            <motion.div
              initial={{ height: 0, opacity: 0 }}
              animate={{ height: 'auto', opacity: 1 }}
              exit={{ height: 0, opacity: 0 }}
              transition={{ duration: 0.2 }}
              className="border-b border-border/50 overflow-hidden bg-secondary/20"
            >
              <div className="p-4">
                <label className="text-sm font-medium mb-2 block">Sidebar Position</label>
                <div className="flex gap-2">
                  <Button
                    size="sm"
                    variant={position === 'left' ? 'default' : 'outline'}
                    onClick={() => onPositionChange('left')}
                    className="flex-1"
                  >
                    <ChevronLeft className="h-4 w-4 mr-1" />
                    Left
                  </Button>
                  <Button
                    size="sm"
                    variant={position === 'right' ? 'default' : 'outline'}
                    onClick={() => onPositionChange('right')}
                    className="flex-1"
                  >
                    Right
                    <ChevronRight className="h-4 w-4 ml-1" />
                  </Button>
                </div>
              </div>
            </motion.div>
          )}
        </AnimatePresence>

        {/* Messages */}
        <div className="flex-1 overflow-y-auto p-6 space-y-4">
          <AnimatePresence initial={false}>
            {messages.map((message, index) => (
              <motion.div
                key={message.id}
                initial={{ opacity: 0, y: 20, scale: 0.95 }}
                animate={{ opacity: 1, y: 0, scale: 1 }}
                exit={{ opacity: 0, scale: 0.95 }}
                transition={{
                  type: 'spring',
                  stiffness: 300,
                  damping: 25,
                  delay: index * 0.05,
                }}
                className={`flex ${message.role === 'user' ? 'justify-end' : 'justify-start'}`}
              >
                <div
                  className={`max-w-[85%] rounded-2xl px-4 py-3 ${
                    message.role === 'user'
                      ? 'bg-gradient-to-br from-primary to-primary/80 text-primary-foreground shadow-lg shadow-primary/20'
                      : 'bg-secondary/60 text-secondary-foreground backdrop-blur-xl'
                  }`}
                >
                  <p className="text-sm leading-relaxed whitespace-pre-wrap">{message.content}</p>

                  {/* Action Buttons */}
                  {message.actions && message.actions.length > 0 && (
                    <div className="mt-3 flex flex-wrap gap-2">
                      {message.actions.map((action, i) => (
                        <button
                          key={i}
                          onClick={action.onClick}
                          className="px-3 py-1.5 text-xs font-medium bg-primary/20 hover:bg-primary/30 rounded-lg transition-all hover:scale-105"
                        >
                          {action.label}
                        </button>
                      ))}
                    </div>
                  )}

                  <span className="text-[10px] opacity-60 mt-2 block">
                    {new Date(message.timestamp).toLocaleTimeString([], {
                      hour: '2-digit',
                      minute: '2-digit',
                    })}
                  </span>
                </div>
              </motion.div>
            ))}
          </AnimatePresence>

          {isProcessing && (
            <motion.div
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              className="flex justify-start"
            >
              <div className="bg-secondary/60 backdrop-blur-xl rounded-2xl px-4 py-3">
                <Loader2 className="h-4 w-4 animate-spin text-primary" />
              </div>
            </motion.div>
          )}
          <div ref={messagesEndRef} />
        </div>

        {/* Quick Actions */}
        <div className="border-t border-border/50 p-4 bg-secondary/10 backdrop-blur-xl">
          <div className="flex flex-wrap gap-2">
            <QuickActionButton
              icon="🥁"
              label="Drums"
              onClick={() => setInput('Create a trap beat')}
            />
            <QuickActionButton
              icon="🎸"
              label="Bass"
              onClick={() => setInput('Add a bass line')}
            />
            <QuickActionButton
              icon="🎹"
              label="Chords"
              onClick={() => setInput('Suggest chords for me')}
            />
            <QuickActionButton
              icon="🎚️"
              label="Mix"
              onClick={() => setInput('Auto-mix this track')}
            />
          </div>
        </div>

        {/* Input */}
        <div className="border-t border-border/50 p-4 bg-background/95 backdrop-blur-xl">
          <div className="flex gap-2">
            <Button
              size="icon"
              variant="ghost"
              title="Voice input (coming soon)"
              className="hover:bg-white/5"
            >
              <Mic className="h-4 w-4" />
            </Button>
            <input
              ref={inputRef}
              type="text"
              placeholder="Ask Wingman anything..."
              value={input}
              onChange={(e) => setInput(e.target.value)}
              onKeyPress={handleKeyPress}
              className="flex-1 px-4 py-2.5 bg-secondary/40 border border-border/50 rounded-xl text-sm focus:outline-none focus:ring-2 focus:ring-primary/50 focus:border-primary/50 transition-all placeholder:text-muted-foreground/50 backdrop-blur-xl"
              disabled={isProcessing}
            />
            <Button
              onClick={handleSend}
              disabled={!input.trim() || isProcessing}
              className="bg-gradient-to-r from-primary to-primary/80 hover:from-primary/90 hover:to-primary/70 shadow-lg shadow-primary/20"
            >
              <Send className="h-4 w-4" />
            </Button>
          </div>
        </div>
      </motion.div>
    </>
  );
}

interface QuickActionButtonProps {
  icon: string;
  label: string;
  onClick: () => void;
}

function QuickActionButton({ icon, label, onClick }: QuickActionButtonProps) {
  return (
    <motion.button
      onClick={onClick}
      className="flex items-center gap-2 px-3 py-2 text-sm font-medium bg-secondary/40 hover:bg-secondary/60 rounded-lg transition-all backdrop-blur-xl border border-border/30"
      whileHover={{ scale: 1.05 }}
      whileTap={{ scale: 0.95 }}
    >
      <span>{icon}</span>
      <span>{label}</span>
    </motion.button>
  );
}

function generateMockResponse(input: string): {
  message: string;
  actions?: Array<{ label: string; onClick: () => void }>;
  dawActions?: Array<any>;
} {
  const lowerInput = input.toLowerCase();

  if (lowerInput.includes('drum') || lowerInput.includes('beat')) {
    return {
      message: "I'll create a trap beat for you!\n\n✓ Set tempo to 140 BPM\n✓ Created drum track\n✓ Generated kick pattern (beats 1 & 3)\n✓ Added snare (beats 2 & 4)\n✓ Hi-hat rolls with variations\n✓ 808 bass slides\n\nCheck Track 1 in the arrangement!",
      actions: [
        { label: 'View in Piano Roll', onClick: () => console.log('Open piano roll') },
        { label: 'Adjust Pattern', onClick: () => console.log('Adjust') },
      ],
      dawActions: [
        { type: 'setTempo', value: 140 },
        { type: 'createTrack', name: 'Trap Drums', trackType: 'midi' },
      ],
    };
  }

  if (lowerInput.includes('bass')) {
    return {
      message: "Adding a bass line in C minor!\n\n✓ Created bass track\n✓ Root notes on strong beats\n✓ Octave jumps for movement\n✓ Slides between key notes\n✓ Follows your chord progression\n\nPlaced on Track 2. Ready to edit!",
      actions: [
        { label: 'Edit Bass Line', onClick: () => console.log('Edit bass') },
      ],
      dawActions: [
        { type: 'createTrack', name: 'Bass', trackType: 'midi' },
      ],
    };
  }

  if (lowerInput.includes('chord')) {
    return {
      message: "For a chill vibe, try this progression:\n\nCm → Ab → Eb → Bb\n(i - VI - III - VII in C minor)\n\nThis progression is perfect for:\n• Lo-fi hip hop\n• R&B\n• Neo-soul\n• Chill beats\n\nWant me to generate it?",
      actions: [
        { label: 'Generate Chords', onClick: () => console.log('Generate chords') },
        { label: 'Try Different Key', onClick: () => console.log('Different key') },
      ],
    };
  }

  if (lowerInput.includes('mix')) {
    return {
      message: "Analyzing your mix...\n\n✓ Balanced levels for clarity\n✓ Applied EQ (cut mud, boost presence)\n✓ Compression for consistency\n✓ Stereo panning for width\n✓ Headroom at -6dB\n\nYour mix is ready! Check the mixer for details.",
      actions: [
        { label: 'View Mixer', onClick: () => console.log('View mixer') },
      ],
    };
  }

  return {
    message: "I can help you with:\n\n🥁 Create drum patterns\n🎹 Generate MIDI melodies\n🎸 Suggest chord progressions\n🎚️ Auto-mix your tracks\n⚡ Control transport & tempo\n\nTry: \"Create drums\" or \"Suggest chords\"",
  };
}
