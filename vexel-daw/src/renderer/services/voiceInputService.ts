/**
 * Voice Input Service
 *
 * Provides voice recognition capabilities using the Web Speech API.
 * Integrates with Wingman AI for voice-controlled DAW operations.
 *
 * Browser Support: Chrome, Edge (Chromium-based browsers)
 * Note: Requires user permission for microphone access
 */

export interface VoiceCommand {
  transcript: string;
  confidence: number;
  isFinal: boolean;
  timestamp: number;
}

export type VoiceCommandCallback = (command: VoiceCommand) => void;
export type VoiceErrorCallback = (error: string) => void;

class VoiceInputService {
  private recognition: any = null;
  private isListening = false;
  private commandCallbacks: Set<VoiceCommandCallback> = new Set();
  private errorCallbacks: Set<VoiceErrorCallback> = new Set();
  private continuous = true;
  private interimResults = true;
  private language = 'en-US';

  constructor() {
    this.initializeRecognition();
  }

  /**
   * Initialize Web Speech API recognition
   */
  private initializeRecognition(): boolean {
    // Check for Web Speech API support
    const SpeechRecognition = (window as any).SpeechRecognition ||
                             (window as any).webkitSpeechRecognition;

    if (!SpeechRecognition) {
      console.warn('Web Speech API not supported in this browser');
      console.warn('Voice input requires Chrome or Edge (Chromium-based)');
      return false;
    }

    try {
      this.recognition = new SpeechRecognition();

      // Configure recognition
      this.recognition.continuous = this.continuous;
      this.recognition.interimResults = this.interimResults;
      this.recognition.lang = this.language;
      this.recognition.maxAlternatives = 1;

      // Set up event listeners
      this.recognition.onresult = this.handleResult.bind(this);
      this.recognition.onerror = this.handleError.bind(this);
      this.recognition.onend = this.handleEnd.bind(this);
      this.recognition.onstart = this.handleStart.bind(this);

      console.log('✅ Web Speech API initialized');
      return true;
    } catch (error) {
      console.error('Failed to initialize Web Speech API:', error);
      return false;
    }
  }

  /**
   * Start listening for voice input
   */
  async startListening(): Promise<boolean> {
    if (!this.recognition) {
      const initialized = this.initializeRecognition();
      if (!initialized) {
        this.notifyError('Web Speech API not available');
        return false;
      }
    }

    if (this.isListening) {
      console.warn('Already listening');
      return true;
    }

    try {
      this.recognition.start();
      console.log('🎤 Voice input started');
      return true;
    } catch (error) {
      console.error('Failed to start voice recognition:', error);
      this.notifyError(`Failed to start: ${error instanceof Error ? error.message : 'Unknown error'}`);
      return false;
    }
  }

  /**
   * Stop listening for voice input
   */
  stopListening(): void {
    if (!this.recognition || !this.isListening) {
      return;
    }

    try {
      this.recognition.stop();
      console.log('🎤 Voice input stopped');
    } catch (error) {
      console.error('Failed to stop voice recognition:', error);
    }
  }

  /**
   * Toggle listening state
   */
  async toggleListening(): Promise<boolean> {
    if (this.isListening) {
      this.stopListening();
      return false;
    } else {
      return await this.startListening();
    }
  }

  /**
   * Check if currently listening
   */
  isActive(): boolean {
    return this.isListening;
  }

  /**
   * Check if Web Speech API is available
   */
  isAvailable(): boolean {
    return !!(
      (window as any).SpeechRecognition ||
      (window as any).webkitSpeechRecognition
    );
  }

  /**
   * Set recognition language
   */
  setLanguage(lang: string): void {
    this.language = lang;
    if (this.recognition) {
      this.recognition.lang = lang;
    }
  }

  /**
   * Set continuous mode
   */
  setContinuous(continuous: boolean): void {
    this.continuous = continuous;
    if (this.recognition) {
      this.recognition.continuous = continuous;
    }
  }

  /**
   * Set interim results
   */
  setInterimResults(enabled: boolean): void {
    this.interimResults = enabled;
    if (this.recognition) {
      this.recognition.interimResults = enabled;
    }
  }

  /**
   * Subscribe to voice commands
   */
  onCommand(callback: VoiceCommandCallback): () => void {
    this.commandCallbacks.add(callback);
    return () => {
      this.commandCallbacks.delete(callback);
    };
  }

  /**
   * Subscribe to errors
   */
  onError(callback: VoiceErrorCallback): () => void {
    this.errorCallbacks.add(callback);
    return () => {
      this.errorCallbacks.delete(callback);
    };
  }

  /**
   * Handle recognition result
   */
  private handleResult(event: any): void {
    for (let i = event.resultIndex; i < event.results.length; i++) {
      const result = event.results[i];
      const transcript = result[0].transcript;
      const confidence = result[0].confidence;
      const isFinal = result.isFinal;

      const command: VoiceCommand = {
        transcript: transcript.trim(),
        confidence,
        isFinal,
        timestamp: Date.now(),
      };

      console.log(`🎤 ${isFinal ? 'Final' : 'Interim'}: "${command.transcript}" (${Math.round(confidence * 100)}%)`);

      this.notifyCommand(command);
    }
  }

  /**
   * Handle recognition error
   */
  private handleError(event: any): void {
    let errorMessage = 'Unknown error';

    switch (event.error) {
      case 'no-speech':
        errorMessage = 'No speech detected';
        break;
      case 'audio-capture':
        errorMessage = 'No microphone found or permission denied';
        break;
      case 'not-allowed':
        errorMessage = 'Microphone permission denied';
        break;
      case 'network':
        errorMessage = 'Network error occurred';
        break;
      case 'aborted':
        errorMessage = 'Recognition aborted';
        break;
      default:
        errorMessage = `Error: ${event.error}`;
    }

    console.error('Voice recognition error:', errorMessage);
    this.notifyError(errorMessage);
  }

  /**
   * Handle recognition end
   */
  private handleEnd(): void {
    this.isListening = false;
    console.log('🎤 Voice recognition ended');

    // Auto-restart if continuous mode is enabled
    if (this.continuous && this.recognition) {
      setTimeout(() => {
        if (!this.isListening && this.continuous) {
          this.startListening();
        }
      }, 100);
    }
  }

  /**
   * Handle recognition start
   */
  private handleStart(): void {
    this.isListening = true;
    console.log('🎤 Voice recognition started');
  }

  /**
   * Notify command callbacks
   */
  private notifyCommand(command: VoiceCommand): void {
    this.commandCallbacks.forEach(callback => {
      try {
        callback(command);
      } catch (error) {
        console.error('Error in command callback:', error);
      }
    });
  }

  /**
   * Notify error callbacks
   */
  private notifyError(error: string): void {
    this.errorCallbacks.forEach(callback => {
      try {
        callback(error);
      } catch (err) {
        console.error('Error in error callback:', err);
      }
    });
  }

  /**
   * Get supported languages (browser-dependent)
   */
  getSupportedLanguages(): string[] {
    // Common supported languages by Chrome/Edge
    return [
      'en-US', 'en-GB', 'es-ES', 'es-MX', 'fr-FR', 'de-DE',
      'it-IT', 'ja-JP', 'ko-KR', 'pt-BR', 'pt-PT', 'ru-RU',
      'zh-CN', 'zh-TW', 'ar-SA', 'hi-IN', 'nl-NL', 'pl-PL',
      'sv-SE', 'tr-TR', 'vi-VN', 'th-TH',
    ];
  }
}

// Export singleton instance
export const voiceInputService = new VoiceInputService();
export default voiceInputService;
