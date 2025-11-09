import { motion } from 'framer-motion';

interface TimelineRulerProps {
  bars: number;
  beatsPerBar: number;
  tempo: number;
  pixelsPerBeat: number;
  timeSignature?: { numerator: number; denominator: number };
}

export default function TimelineRuler({
  bars,
  beatsPerBar,
  tempo,
  pixelsPerBeat,
  timeSignature = { numerator: 4, denominator: 4 },
}: TimelineRulerProps) {
  const totalBeats = bars * beatsPerBar;
  const width = totalBeats * pixelsPerBeat;

  return (
    <div className="relative h-12 bg-card/50 border-b border-border/50 overflow-hidden">
      {/* Ruler Background */}
      <div className="absolute inset-0 bg-gradient-to-b from-card to-card/80 backdrop-blur-sm" />

      {/* Bar and Beat Markers */}
      <svg width={width} height="100%" className="absolute top-0 left-0">
        {/* Beat lines */}
        {Array.from({ length: totalBeats }).map((_, beatIndex) => {
          const x = beatIndex * pixelsPerBeat;
          const barNumber = Math.floor(beatIndex / beatsPerBar);
          const beatInBar = beatIndex % beatsPerBar;
          const isBarStart = beatInBar === 0;

          return (
            <g key={beatIndex}>
              {/* Beat line */}
              <line
                x1={x}
                y1={isBarStart ? 0 : 24}
                x2={x}
                y2={48}
                stroke="currentColor"
                strokeWidth={isBarStart ? 2 : 1}
                className={isBarStart ? 'text-foreground/60' : 'text-foreground/20'}
              />

              {/* Bar number label */}
              {isBarStart && (
                <text
                  x={x + 4}
                  y={16}
                  className="text-xs font-mono font-semibold fill-current text-foreground"
                  style={{ userSelect: 'none' }}
                >
                  {barNumber + 1}
                </text>
              )}

              {/* Beat number label (for non-bar-start beats) */}
              {!isBarStart && (
                <text
                  x={x + 2}
                  y={42}
                  className="text-[10px] font-mono fill-current text-muted-foreground"
                  style={{ userSelect: 'none' }}
                >
                  {beatInBar + 1}
                </text>
              )}
            </g>
          );
        })}
      </svg>

      {/* Playhead */}
      <motion.div
        className="absolute top-0 bottom-0 w-0.5 bg-primary shadow-lg shadow-primary/50 pointer-events-none z-10"
        style={{ left: 0 }}
        animate={{
          left: [`0px`, `${width}px`],
        }}
        transition={{
          duration: (totalBeats / tempo) * 60,
          repeat: Infinity,
          ease: 'linear',
        }}
      >
        {/* Playhead triangle */}
        <div className="absolute top-0 left-1/2 -translate-x-1/2 w-0 h-0 border-l-[6px] border-r-[6px] border-t-[8px] border-l-transparent border-r-transparent border-t-primary" />
      </motion.div>

      {/* Time display (optional hover info) */}
      <div className="absolute top-1 right-2 px-2 py-0.5 bg-background/80 rounded text-[10px] font-mono text-muted-foreground backdrop-blur-sm">
        {tempo} BPM • {timeSignature.numerator}/{timeSignature.denominator}
      </div>
    </div>
  );
}
