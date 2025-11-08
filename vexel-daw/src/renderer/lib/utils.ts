import { type ClassValue, clsx } from 'clsx';
import { twMerge } from 'tailwind-merge';

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

export function formatTime(bars: number, beatsPerBar: number = 4): string {
  const bar = Math.floor(bars) + 1;
  const beat = Math.floor((bars % 1) * beatsPerBar) + 1;
  return `${bar}.${beat}`;
}

export function bpmToMs(bpm: number): number {
  return (60 / bpm) * 1000;
}
