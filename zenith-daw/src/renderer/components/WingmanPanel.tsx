import { useState, useRef, useEffect } from 'react';
import { Send, Sparkles, X, Mic, Loader2 } from 'lucide-react';
import { Button } from './ui/button';

interface Message {
  id: string;
  role: 'user' | 'assistant';
  content: string;
  timestamp: number;
}

interface WingmanPanelProps {
  isOpen: boolean;
  onClose: () => void;
}

export default function WingmanPanel({ isOpen, onClose }: WingmanPanelProps) {
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
  const messagesEndRef = useRef<HTMLDivElement>(null);

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

    // Simulate AI response (will connect to Wingman later)
    setTimeout(() => {
      const aiMessage: Message = {
        id: (Date.now() + 1).toString(),
        role: 'assistant',
        content: generateMockResponse(input),
        timestamp: Date.now(),
      };
      setMessages((prev) => [...prev, aiMessage]);
      setIsProcessing(false);
    }, 1000);
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  if (!isOpen) return null;

  return (
    <div className="absolute top-20 right-4 w-96 h-[600px] bg-card border border-border rounded-lg shadow-2xl flex flex-col z-50 animate-in slide-in-from-right">
      {/* Header */}
      <div className="h-14 border-b border-border flex items-center justify-between px-4 bg-gradient-to-r from-primary/10 to-transparent">
        <div className="flex items-center gap-2">
          <div className="w-8 h-8 rounded-full bg-primary/20 flex items-center justify-center">
            <Sparkles className="h-4 w-4 text-primary" />
          </div>
          <div>
            <h3 className="font-semibold text-sm">Wingman AI</h3>
            <p className="text-xs text-muted-foreground">
              {isProcessing ? 'Thinking...' : 'Ready to help'}
            </p>
          </div>
        </div>
        <Button size="icon" variant="ghost" onClick={onClose}>
          <X className="h-4 w-4" />
        </Button>
      </div>

      {/* Messages */}
      <div className="flex-1 overflow-y-auto p-4 space-y-4">
        {messages.map((message) => (
          <div
            key={message.id}
            className={`flex ${message.role === 'user' ? 'justify-end' : 'justify-start'}`}
          >
            <div
              className={`max-w-[80%] rounded-lg px-4 py-2 ${
                message.role === 'user'
                  ? 'bg-primary text-primary-foreground'
                  : 'bg-secondary text-secondary-foreground'
              }`}
            >
              <p className="text-sm whitespace-pre-wrap">{message.content}</p>
              <span className="text-xs opacity-70 mt-1 block">
                {new Date(message.timestamp).toLocaleTimeString([], {
                  hour: '2-digit',
                  minute: '2-digit',
                })}
              </span>
            </div>
          </div>
        ))}
        {isProcessing && (
          <div className="flex justify-start">
            <div className="bg-secondary text-secondary-foreground rounded-lg px-4 py-2">
              <Loader2 className="h-4 w-4 animate-spin" />
            </div>
          </div>
        )}
        <div ref={messagesEndRef} />
      </div>

      {/* Quick Actions */}
      <div className="border-t border-border p-2 flex flex-wrap gap-2">
        <QuickActionButton
          label="Create drums"
          onClick={() => setInput('Create a trap beat')}
        />
        <QuickActionButton
          label="Add bass"
          onClick={() => setInput('Add a bass line')}
        />
        <QuickActionButton
          label="Suggest chords"
          onClick={() => setInput('What chords should I use?')}
        />
        <QuickActionButton
          label="Auto-mix"
          onClick={() => setInput('Mix this track for me')}
        />
      </div>

      {/* Input */}
      <div className="border-t border-border p-4 flex gap-2">
        <Button size="icon" variant="ghost" title="Voice input (coming soon)">
          <Mic className="h-4 w-4" />
        </Button>
        <input
          type="text"
          placeholder="Ask Wingman anything..."
          value={input}
          onChange={(e) => setInput(e.target.value)}
          onKeyPress={handleKeyPress}
          className="flex-1 px-3 py-2 bg-background border border-input rounded text-sm focus:outline-none focus:ring-2 focus:ring-primary"
          disabled={isProcessing}
        />
        <Button onClick={handleSend} disabled={!input.trim() || isProcessing}>
          <Send className="h-4 w-4" />
        </Button>
      </div>
    </div>
  );
}

interface QuickActionButtonProps {
  label: string;
  onClick: () => void;
}

function QuickActionButton({ label, onClick }: QuickActionButtonProps) {
  return (
    <button
      onClick={onClick}
      className="px-3 py-1 text-xs bg-secondary hover:bg-accent rounded transition-colors"
    >
      {label}
    </button>
  );
}

function generateMockResponse(input: string): string {
  const lowerInput = input.toLowerCase();

  if (lowerInput.includes('drum') || lowerInput.includes('beat')) {
    return "I'll create a trap beat for you! Creating:\n• Kick pattern on beats 1 and 3\n• Snare on beats 2 and 4\n• Hi-hat rolls\n• 808 bass slides\n\nSet tempo to 140 BPM. Check the arrangement view!";
  }

  if (lowerInput.includes('bass')) {
    return "Adding a bass line in the key of C minor. I'm creating:\n• Root notes on strong beats\n• Octave jumps for interest\n• Slides between notes\n\nPlaced on Track 2. You can edit it in the piano roll!";
  }

  if (lowerInput.includes('chord')) {
    return "For a chill vibe, try this progression:\nCm - Ab - Eb - Bb\n\nThis is a i-VI-III-VII progression in C minor. It's used in lots of lo-fi and R&B tracks. Want me to generate it?";
  }

  if (lowerInput.includes('mix')) {
    return "Analyzing your tracks...\n✓ Adjusted levels for clarity\n✓ Applied EQ to drums\n✓ Added compression to vocals\n✓ Set proper panning\n\nYour mix is balanced! Check the mixer panel for details.";
  }

  return "I'm still learning to understand that! Here's what I can do:\n• Create drum patterns\n• Generate MIDI melodies\n• Suggest chord progressions\n• Auto-mix tracks\n• Control transport (play, stop, tempo)\n\nTry asking me to create drums or suggest chords!";
}
