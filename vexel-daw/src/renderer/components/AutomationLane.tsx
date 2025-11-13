import { motion, AnimatePresence } from 'framer-motion';
import { useState } from 'react';
import { Volume2, Sliders, Circle, Minus, Plus } from 'lucide-react';

export type AutomationMode = 'off' | 'read' | 'write' | 'touch' | 'latch' | 'trim';

export interface AutomationPoint {
  time: number; // in beats
  value: number; // 0-1
}

export interface AutomationLane {
  id: string;
  trackId: string;
  parameter: 'volume' | 'pan' | 'send1' | 'send2' | 'plugin';
  parameterName: string;
  points: AutomationPoint[];
  mode: AutomationMode;
  color: string;
}

interface AutomationLaneComponentProps {
  lane: AutomationLane;
  width: number;
  height: number;
  onPointAdd: (time: number, value: number) => void;
  onPointMove: (index: number, time: number, value: number) => void;
  onPointDelete: (index: number) => void;
  onModeChange: (mode: AutomationMode) => void;
}

export default function AutomationLaneComponent({
  lane,
  width,
  height,
  onPointAdd,
  onPointMove,
  onPointDelete,
  onModeChange,
}: AutomationLaneComponentProps) {
  const [selectedPoint, setSelectedPoint] = useState<number | null>(null);
  const [isDragging, setIsDragging] = useState(false);

  // Convert automation points to SVG path
  const generatePath = (): string => {
    if (lane.points.length === 0) return '';

    const points = [...lane.points].sort((a, b) => a.time - b.time);

    let path = `M 0 ${height - points[0].value * height}`;

    for (let i = 0; i < points.length; i++) {
      const x = (points[i].time / 32) * width; // Assuming 32 bars
      const y = height - points[i].value * height;
      path += ` L ${x} ${y}`;
    }

    return path;
  };

  const handleCanvasClick = (e: React.MouseEvent<SVGSVGElement>) => {
    if (lane.mode === 'off' || lane.mode === 'read') return;

    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    const time = (x / width) * 32; // Convert to beats (32 bars)
    const value = 1 - y / height; // Invert Y axis

    onPointAdd(time, Math.max(0, Math.min(1, value)));
  };

  const handlePointDrag = (index: number, e: React.MouseEvent<SVGCircleElement>) => {
    if (!isDragging) return;

    const svg = e.currentTarget.ownerSVGElement;
    if (!svg) return;

    const rect = svg.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    const time = (x / width) * 32;
    const value = 1 - y / height;

    onPointMove(index, Math.max(0, time), Math.max(0, Math.min(1, value)));
  };

  return (
    <div className="bg-background/30 border-t border-border/30 backdrop-blur-sm">
      {/* Header */}
      <div className="h-8 flex items-center justify-between px-3 bg-card/50 border-b border-border/30">
        <div className="flex items-center gap-2">
          {lane.parameter === 'volume' && <Volume2 className="h-3 w-3 text-muted-foreground" />}
          {lane.parameter === 'pan' && <Sliders className="h-3 w-3 text-muted-foreground" />}
          <span className="text-xs font-medium">{lane.parameterName}</span>
        </div>

        {/* Automation Mode Selector */}
        <div className="flex items-center gap-1">
          {(['off', 'read', 'write', 'touch', 'latch'] as AutomationMode[]).map((mode) => (
            <motion.button
              key={mode}
              whileHover={{ scale: 1.05 }}
              whileTap={{ scale: 0.95 }}
              onClick={() => onModeChange(mode)}
              className={`px-2 py-0.5 rounded text-[10px] font-medium transition-all ${
                lane.mode === mode
                  ? 'bg-primary text-primary-foreground'
                  : 'bg-muted hover:bg-muted/80 text-muted-foreground'
              }`}
              title={`${mode.charAt(0).toUpperCase() + mode.slice(1)} mode`}
            >
              {mode.charAt(0).toUpperCase()}
            </motion.button>
          ))}
        </div>
      </div>

      {/* Automation Envelope */}
      <div className="relative" style={{ height: `${height}px` }}>
        <svg
          width={width}
          height={height}
          className="absolute inset-0 cursor-crosshair"
          onClick={handleCanvasClick}
        >
          {/* Grid lines */}
          <g stroke="currentColor" strokeWidth="0.5" className="text-border/20">
            {/* Horizontal grid (value) */}
            {[0, 0.25, 0.5, 0.75, 1].map((value) => (
              <line
                key={value}
                x1={0}
                y1={height - value * height}
                x2={width}
                y2={height - value * height}
              />
            ))}
            {/* Vertical grid (time - every 4 beats) */}
            {Array.from({ length: 33 }, (_, i) => i).map((bar) => (
              <line
                key={bar}
                x1={(bar / 32) * width}
                y1={0}
                x2={(bar / 32) * width}
                y2={height}
                strokeDasharray={bar % 4 === 0 ? '' : '2,2'}
              />
            ))}
          </g>

          {/* Automation curve */}
          {lane.points.length > 0 && (
            <motion.path
              d={generatePath()}
              fill="none"
              stroke={lane.color}
              strokeWidth={2}
              strokeLinecap="round"
              strokeLinejoin="round"
              initial={{ pathLength: 0, opacity: 0 }}
              animate={{ pathLength: 1, opacity: 1 }}
              transition={{ duration: 0.3 }}
            />
          )}

          {/* Automation points */}
          <AnimatePresence>
            {lane.points.map((point, index) => {
              const x = (point.time / 32) * width;
              const y = height - point.value * height;

              return (
                <g key={index}>
                  <motion.circle
                    cx={x}
                    cy={y}
                    r={selectedPoint === index ? 6 : 4}
                    fill={lane.color}
                    stroke="white"
                    strokeWidth={2}
                    className="cursor-move"
                    initial={{ scale: 0 }}
                    animate={{ scale: 1 }}
                    exit={{ scale: 0 }}
                    whileHover={{ scale: 1.2 }}
                    onMouseDown={(e) => {
                      e.stopPropagation();
                      setSelectedPoint(index);
                      setIsDragging(true);
                    }}
                    onMouseMove={(e) => handlePointDrag(index, e)}
                    onMouseUp={() => setIsDragging(false)}
                    onDoubleClick={(e) => {
                      e.stopPropagation();
                      onPointDelete(index);
                    }}
                  />
                </g>
              );
            })}
          </AnimatePresence>
        </svg>

        {/* Value labels */}
        <div className="absolute left-0 top-0 bottom-0 w-12 flex flex-col justify-between py-1 text-[9px] text-muted-foreground font-mono pointer-events-none">
          <div>1.0</div>
          <div>0.5</div>
          <div>0.0</div>
        </div>
      </div>
    </div>
  );
}

// Helper component for automation mode tooltip
export function AutomationModeInfo({ mode }: { mode: AutomationMode }) {
  const descriptions = {
    off: 'No automation playback or recording',
    read: 'Playback existing automation only',
    write: 'Record automation (overwrites existing)',
    touch: 'Write while touching, revert when released',
    latch: 'Write and latch to last value',
    trim: 'Offset existing automation up/down',
  };

  return (
    <div className="text-xs">
      <div className="font-semibold mb-1">{mode.toUpperCase()} Mode</div>
      <div className="text-muted-foreground">{descriptions[mode]}</div>
    </div>
  );
}
