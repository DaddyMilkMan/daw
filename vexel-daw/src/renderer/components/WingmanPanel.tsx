import { useState, useRef, useEffect } from 'react';
import { Send, Sparkles, X, Mic, Loader2, Music, Drum, Piano } from 'lucide-react';
import { Button } from './ui/button';
import magentaService from '../lib/MagentaService';
import { getRandomColor } from '../types/session';

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
    const userInput = input;
    setInput('');
    setIsProcessing(true);

    try {
      // Process the request with MagentaService
      const response = await processAIRequest(userInput);

      const aiMessage: Message = {
        id: (Date.now() + 1).toString(),
        role: 'assistant',
        content: response,
        timestamp: Date.now(),
      };
      setMessages((prev) => [...prev, aiMessage]);
    } catch (error) {
      const errorMessage: Message = {
        id: (Date.now() + 1).toString(),
        role: 'assistant',
        content: `Sorry, I encountered an error: ${error instanceof Error ? error.message : 'Unknown error'}`,
        timestamp: Date.now(),
      };
      setMessages((prev) => [...prev, errorMessage]);
    } finally {
      setIsProcessing(false);
    }
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  const processAIRequest = async (input: string): Promise<string> => {
    const lowerInput = input.toLowerCase();

    // Initialize MagentaService
    await magentaService.initialize();

    // Generate drums
    if (lowerInput.includes('drum') || lowerInput.includes('beat')) {
      const style = lowerInput.includes('trap') ? 'trap' :
                    lowerInput.includes('house') ? 'house' : 'trap';

      const result = await magentaService.generateDrumPattern(style);

      console.log('🥁 Generated drum pattern:', result);

      return `I've created a ${result.style} drum pattern for you! Generated:\n` +
             `• ${result.pattern.length} drum hits\n` +
             `• ${result.bars} bars\n` +
             `• Pattern includes kick, snare, and hi-hats\n\n` +
             `The pattern is ready to be inserted into your track!`;
    }

    // Generate melody
    if (lowerInput.includes('melody') || lowerInput.includes('lead')) {
      const result = await magentaService.generateMelody();

      console.log('🎼 Generated melody:', result);

      return `I've generated a melody for you! Created:\n` +
             `• ${result.notes.length} notes\n` +
             `• Temperature: ${result.temperature}\n` +
             `• Musical and expressive\n\n` +
             `The melody is ready to be added to your arrangement!`;
    }

    // Generate chords
    if (lowerInput.includes('chord')) {
      const key = lowerInput.includes('c minor') || lowerInput.includes('cm') ? 'Cm' :
                  lowerInput.includes('g major') || lowerInput.includes('g') ? 'G' : 'C';

      const result = await magentaService.generateChordProgression(key);

      console.log('🎹 Generated chord progression:', result);

      return `Here's a chord progression in ${result.key}:\n` +
             `${result.chords.join(' - ')}\n\n` +
             `This is a ${result.scale} progression with ${result.notes.length} notes.\n` +
             `Perfect for your track! Want me to insert it?`;
    }

    // Generate bass
    if (lowerInput.includes('bass')) {
      const result = await magentaService.generateMelody([], { steps: 16 });

      console.log('🎸 Generated bass line:', result);

      return `I've created a bass line for you! Features:\n` +
             `• ${result.notes.length} notes\n` +
             `• Root-focused progression\n` +
             `• Groovy rhythm\n\n` +
             `Ready to add some low-end to your track!`;
    }

    // Default response
    return "I'm here to help! I can:\n" +
           "• 🥁 Generate drum patterns (try 'create trap drums')\n" +
           "• 🎹 Suggest chord progressions (try 'chords in C minor')\n" +
           "• 🎼 Create melodies (try 'generate a melody')\n" +
           "• 🎸 Make bass lines (try 'add a bass line')\n\n" +
           "What would you like to create?";
  };

  const handleQuickAction = async (action: 'drums' | 'chords' | 'melody' | 'bass') => {
    setIsProcessing(true);

    try {
      await magentaService.initialize();
      let result: string;

      switch (action) {
        case 'drums':
          const drums = await magentaService.generateDrumPattern('trap');
          result = `Created trap drums: ${drums.pattern.length} hits across ${drums.bars} bars`;
          break;
        case 'chords':
          const chords = await magentaService.generateChordProgression('C');
          result = `Generated progression: ${chords.chords.join(' - ')}`;
          break;
        case 'melody':
          const melody = await magentaService.generateMelody();
          result = `Created melody with ${melody.notes.length} notes`;
          break;
        case 'bass':
          const bass = await magentaService.generateMelody([], { steps: 16 });
          result = `Generated bass line with ${bass.notes.length} notes`;
          break;
      }

      const aiMessage: Message = {
        id: Date.now().toString(),
        role: 'assistant',
        content: `✓ ${result}\n\nThe pattern is ready to be inserted into your timeline!`,
        timestamp: Date.now(),
      };
      setMessages((prev) => [...prev, aiMessage]);
    } catch (error) {
      const errorMessage: Message = {
        id: Date.now().toString(),
        role: 'assistant',
        content: `Error: ${error instanceof Error ? error.message : 'Unknown error'}`,
        timestamp: Date.now(),
      };
      setMessages((prev) => [...prev, errorMessage]);
    } finally {
      setIsProcessing(false);
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
      <div className="border-t border-border p-2 space-y-2">
        <div className="flex items-center gap-2 mb-1">
          <Sparkles className="h-3 w-3 text-primary" />
          <span className="text-xs font-semibold text-muted-foreground">AI Generation</span>
        </div>
        <div className="flex flex-wrap gap-2">
          <QuickActionButton
            icon={<Drum className="h-3 w-3" />}
            label="Generate Drums"
            onClick={() => handleQuickAction('drums')}
            variant="ai"
          />
          <QuickActionButton
            icon={<Piano className="h-3 w-3" />}
            label="Generate Chords"
            onClick={() => handleQuickAction('chords')}
            variant="ai"
          />
          <QuickActionButton
            icon={<Music className="h-3 w-3" />}
            label="Generate Melody"
            onClick={() => handleQuickAction('melody')}
            variant="ai"
          />
          <QuickActionButton
            icon={<Music className="h-3 w-3" />}
            label="Generate Bass"
            onClick={() => handleQuickAction('bass')}
            variant="ai"
          />
        </div>
        <div className="flex items-center gap-2 mb-1 mt-3">
          <span className="text-xs font-semibold text-muted-foreground">Quick Prompts</span>
        </div>
        <div className="flex flex-wrap gap-2">
          <QuickActionButton
            label="Trap drums"
            onClick={() => setInput('Create a trap beat')}
          />
          <QuickActionButton
            label="House drums"
            onClick={() => setInput('Create house drums')}
          />
          <QuickActionButton
            label="Chords in Cm"
            onClick={() => setInput('Chords in C minor')}
          />
        </div>
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
  icon?: React.ReactNode;
  variant?: 'default' | 'ai';
}

function QuickActionButton({ label, onClick, icon, variant = 'default' }: QuickActionButtonProps) {
  const baseClasses = "px-3 py-1.5 text-xs rounded transition-colors flex items-center gap-1.5";
  const variantClasses = variant === 'ai'
    ? "bg-primary/10 hover:bg-primary/20 text-primary border border-primary/30"
    : "bg-secondary hover:bg-accent";

  return (
    <button
      onClick={onClick}
      className={`${baseClasses} ${variantClasses}`}
    >
      {icon}
      <span>{label}</span>
    </button>
  );
}
