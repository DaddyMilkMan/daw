/**
 * Metronome Visual Countdown
 * Shows big numbers in the middle of the screen during pre-count
 * "1 2 3 GO" or "1 2 3 4 5 6 7 GO" - fun and professional!
 */

import { motion, AnimatePresence } from 'framer-motion';
import { useEffect, useState } from 'react';
import { MetronomeState } from '../types/metronome';

interface MetronomeCountdownProps {
  state: MetronomeState;
  totalPreCountBeats: number;
  timeSignature: { numerator: number; denominator: number };
}

export default function MetronomeCountdown({
  state,
  totalPreCountBeats,
  timeSignature,
}: MetronomeCountdownProps) {
  const [displayText, setDisplayText] = useState<string>('');
  const [isAccent, setIsAccent] = useState<boolean>(false);

  useEffect(() => {
    if (!state.isInPreCount) {
      setDisplayText('');
      return;
    }

    const currentBeat = state.preCountBeat + 1; // Convert from 0-based to 1-based
    const isDownbeat = (currentBeat - 1) % timeSignature.numerator === 0;

    setIsAccent(isDownbeat);

    // Check if this is the last beat of pre-count
    if (currentBeat >= totalPreCountBeats) {
      setDisplayText('GO');
    } else {
      setDisplayText(currentBeat.toString());
    }
  }, [state, totalPreCountBeats, timeSignature]);

  if (!state.isInPreCount || !displayText) {
    return null;
  }

  return (
    <AnimatePresence mode="wait">
      <motion.div
        key={state.preCountBeat}
        initial={{ scale: 0.5, opacity: 0 }}
        animate={{ scale: 1, opacity: 1 }}
        exit={{ scale: 1.5, opacity: 0 }}
        transition={{
          duration: 0.2,
          ease: 'easeOut',
        }}
        className="fixed inset-0 pointer-events-none flex items-center justify-center z-50"
      >
        <motion.div
          animate={{
            scale: isAccent ? [1, 1.1, 1] : 1,
          }}
          transition={{
            duration: 0.3,
            ease: 'easeInOut',
          }}
          className={`
            text-[20rem] font-black tracking-tighter select-none
            ${displayText === 'GO'
              ? 'text-primary'
              : isAccent
                ? 'text-foreground dark:text-white'
                : 'text-foreground/70 dark:text-white/70'
            }
            ${displayText === 'GO' ? 'drop-shadow-[0_0_50px_rgba(var(--primary-rgb),0.8)]' : ''}
          `}
          style={{
            textShadow: displayText === 'GO'
              ? '0 0 80px rgba(var(--primary-rgb), 0.6)'
              : isAccent
                ? '0 0 40px rgba(0, 0, 0, 0.3)'
                : 'none',
          }}
        >
          {displayText}
        </motion.div>

        {/* Pulsing ring for extra emphasis on GO */}
        {displayText === 'GO' && (
          <motion.div
            initial={{ scale: 1, opacity: 0.8 }}
            animate={{ scale: 3, opacity: 0 }}
            transition={{ duration: 0.8, ease: 'easeOut' }}
            className="absolute inset-0 flex items-center justify-center"
          >
            <div className="w-96 h-96 border-8 border-primary rounded-full" />
          </motion.div>
        )}
      </motion.div>
    </AnimatePresence>
  );
}
