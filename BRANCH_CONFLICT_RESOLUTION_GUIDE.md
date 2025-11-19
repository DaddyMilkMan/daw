# Branch Conflict Resolution Guide

## Current Status Summary

### Branches Already Compatible ✅
These branches have NO conflicts with `claude/daw-features-comparison-011CUv5TyhjPCXSWeZJXTbs7`:
- `claude/zenith-audio-clip-editing-011CUx2oSfAJoTPh5os1NENs`
- `claude/full-implementation-011CUwyfsaHote8BFZdzD92s`
- `claude/ui-freeze-engine-adapter-011CUyqXTyp2NMA5M8qbPEZT`
- `claude/zenith-audio-midi-recording-011CUx2uD1Yd9qQ2sZt7ceqh`
- `claude/zenith-phase2-project-save-load-011CUx1z2XmdGFfXVcABUrkf`

**Action:** These can be merged directly via pull requests.

### Branches with Conflicts Resolved ✅

#### 1. claude/zenith-plugin-system-011CUx28AkcQynMgXKqa7VRv

**Conflicts:**
- `vexel-daw/src/renderer/types/plugin.ts`
- `vexel-daw/src/renderer/components/RightPanel.tsx`

**Resolution Applied:**
1. **plugin.ts**: Merged WAM 2.0 standard (main) with legacy plugin system (feature):
   - Kept WAM interfaces as primary system
   - Added legacy types under "Legacy/Simple Plugin System Types" section
   - Preserved BUILTIN_EFFECTS array for backward compatibility

2. **RightPanel.tsx**: Merged imports and types:
   - Combined all imports from both branches
   - Changed `Track[]` to `AudioTrack[]` for consistency
   - Kept plugin management handlers from feature branch
   - Integrated with engineClient

**Commit:** `515ab19 - Merge main: integrate WAM plugin system with legacy plugin support`

**Status:** ✅ Ready to merge (push failed due to session permissions, but resolution is complete)

---

### Branches with Conflicts Pending

#### 2. claude/zenith-tempo-metronome-011CUx2rKUdvrbdCZSo11TMy

**Conflicts:**
- `vexel-daw/src/renderer/App.tsx` (1 conflict)
- `vexel-daw/src/renderer/components/CenterPanel.tsx` (4 conflicts)
- `vexel-daw/src/renderer/components/TransportBar.tsx` (4 conflicts)

**Resolution Strategy:**

##### CenterPanel.tsx ✅ (Resolved)
```typescript
// Merge imports
import { Track, AudioState } from '@/types/audio';
import { AudioTrack } from '../audio/AudioEngine';

// Combined props
interface CenterPanelProps {
  tracks: AudioTrack[];
  bpm?: number;
  onOpenPianoRoll: (trackId: string, trackName: string) => void;
  audioState?: AudioState; // Optional for backward compatibility
}

// Function signature
export default function CenterPanel({ tracks, bpm = 128, onOpenPianoRoll, audioState }: CenterPanelProps)

// Pass both props to ArrangementViewEnhanced
<ArrangementViewEnhanced
  key="arrangement"
  tracks={tracks}
  bpm={bpm}
  onOpenPianoRoll={onOpenPianoRoll}
  audioState={audioState}  // Pass optional audioState
/>
```

##### App.tsx (Pending)
**Conflict Pattern:**
- Feature branch has project save/load UI (FileMenu, notifications)
- Main branch uses simplified architecture (no FileMenu)

**Resolution:**
Keep main branch's simplified architecture. The feature branch's save/load functionality should be handled through engineClient commands, not direct UI:

```typescript
// Keep main branch's clean imports (no FileMenu, no NotificationContainer)
import { useEffect, useState, useCallback, useRef } from 'react';
import { AnimatePresence } from 'framer-motion';
import TransportBar from './components/TransportBar';
import LeftPanel from './components/LeftPanel';
import CenterPanel from './components/CenterPanel';
import RightPanel from './components/RightPanel';
import WingmanSidebar from './components/WingmanSidebar';
import PianoRoll from './components/PianoRoll';
import { engineClient } from './lib/engineClient';
import { useStore, useProjectState, usePreferences } from './lib/store';
import './App.css';

// Keep main branch's keyboard shortcuts that use engineClient
// Remove handleNewProject, handleSaveProject, handleLoadProject functions
// These should be handled by engineClient, not UI layer
```

##### TransportBar.tsx (Pending)
**Conflict Pattern:**
- Feature branch adds metronome with saved settings, pre-count
- Main branch has simplified transport controls

**Resolution:**
Merge metronome features while using engineClient:

```typescript
// Combined imports
import { Play, Pause, Square, SkipBack, SkipForward, Circle, Sparkles, Repeat, Activity, Volume2 } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { Button } from './ui/button';
import { useState, useEffect } from 'react';
import { metronome } from '@/lib/metronome';
import { loadMetronomeSettings, saveMetronomeSettings, loadProjectSettings, saveProjectSettings } from '@/lib/storage';
import { engineClient } from '../lib/engineClient';

// Initialize with saved settings
const savedMetronome = loadMetronomeSettings();
const savedProject = loadProjectSettings();

const [tempo, setTempo] = useState(savedProject.tempo || audioState.tempo);
const [isMetronomeOn, setIsMetronomeOn] = useState(false);

// handlePlay with pre-count support
const handlePlay = () => {
  if (audioState.isPlaying) {
    engineClient.sendCommand('transport:pause');
  } else {
    if (preCountEnabled && isMetronomeOn && !audioState.isPlaying) {
      metronome.startWithPreCount(preCountBars, () => {
        engineClient.sendCommand('transport:play');
      });
    } else {
      engineClient.sendCommand('transport:play');
    }
  }
};

// All other handlers use engineClient
const handleTimeSignatureChange = (numerator: number, denominator: number) => {
  const newTimeSignature = { numerator, denominator };
  setTimeSignature(newTimeSignature);
  engineClient.sendCommand('transport:setTimeSignature', newTimeSignature);
};
```

**Status:** ⚠️ Partial (CenterPanel done, App.tsx and TransportBar.tsx pending)

---

#### 3. claude/zenith-tempo-metronome-011CUx2sYYUutsrYmrZxrKS5

**Conflicts:**
- `vexel-daw/src/renderer/App.tsx` (5 conflicts)
- `vexel-daw/src/renderer/components/TransportBar.tsx` (2 conflicts)

**Resolution Strategy:**
Same as branch #2 above - merge metronome features with engineClient architecture.

**Status:** ⚠️ Pending

---

## Resolution Principles

### 1. Architecture Priority
- **Main branch wins** for architecture decisions
- Use `engineClient` for all transport/project commands
- No FileMenu, no Notification UI components
- Keep UI layer thin, business logic in engine

### 2. Feature Preservation
- **Keep all features** from both branches
- Metronome with pre-count: ✅ Keep
- Saved settings: ✅ Keep
- Plugin system: ✅ Keep (both WAM and legacy)
- Time signature editor: ✅ Keep

### 3. Type Consistency
- Use `AudioTrack` everywhere (not `Track`)
- Make optional props explicit (`audioState?:`)
- Preserve all type definitions from both branches

### 4. Import Organization
```typescript
// 1. React imports
import { useEffect, useState } from 'react';

// 2. External libraries
import { motion } from 'framer-motion';

// 3. Internal components
import TransportBar from './components/TransportBar';

// 4. Services/utilities
import { engineClient } from './lib/engineClient';
import { metronome } from '@/lib/metronome';

// 5. Types
import { AudioTrack } from '../audio/AudioEngine';

// 6. Styles
import './App.css';
```

## Testing Checklist

After resolving conflicts, test:

### Plugin System
- [ ] Load WAM plugins
- [ ] Use built-in effects (EQ, Compressor, Reverb)
- [ ] Plugin parameters adjust correctly
- [ ] Plugin bypass works

### Tempo/Metronome
- [ ] Metronome plays at correct BPM
- [ ] Pre-count works before recording
- [ ] Saved metronome settings persist
- [ ] Time signature changes apply
- [ ] Tap tempo calculates correctly

### Transport
- [ ] Play/pause works
- [ ] Stop resets playhead
- [ ] Keyboard shortcuts work (Space, Enter)
- [ ] Loop mode toggles

### Integration
- [ ] No TypeScript errors
- [ ] npm run build succeeds
- [ ] No console errors at runtime

## Next Steps

1. **Complete Pending Resolutions:**
   - Finish App.tsx for tempo-metronome-r
   - Finish TransportBar.tsx for tempo-metronome-r
   - Apply same fixes to tempo-metronome-s

2. **Test Thoroughly:**
   - Run build: `cd vexel-daw && npm run build`
   - Check types: `npx tsc --noEmit`
   - Manual testing per checklist above

3. **Create Pull Requests:**
   - One PR per resolved branch
   - Include testing notes
   - Reference this guide in PR description

4. **Merge Order:**
   ```
   1. plugin-system (most self-contained)
   2. tempo-metronome-r (adds metronome features)
   3. tempo-metronome-s (similar to -r)
   ```

## Files Modified Summary

| Branch | Files | Lines Changed | Complexity |
|--------|-------|---------------|------------|
| plugin-system | 2 | ~215 | Medium |
| tempo-metronome-r | 3 | ~150 | Medium |
| tempo-metronome-s | 2 | ~100 | Low |

**Total:** 7 files, ~465 lines of conflict resolution

## Common Patterns

### Pattern 1: Import Merging
```typescript
// ❌ Conflict
// Version A
import { A } from './a';
// Version B
import { B } from './b';

// ✅ Resolution
import { A } from './a';
import { B } from './b';
```

### Pattern 2: Props Merging
```typescript
// ❌ Conflict
// Version A
interface Props { a: string; }
// Version B
interface Props { b: number; }

// ✅ Resolution
interface Props {
  a?: string;  // Optional if not always needed
  b?: number;  // Optional if not always needed
}
```

### Pattern 3: Function Handler Merging
```typescript
// ❌ Conflict
// Version A
const handler = () => { featureA(); };
// Version B
const handler = () => { featureB(); };

// ✅ Resolution
const handler = () => {
  if (conditionForA) {
    featureA();
  } else {
    featureB();
  }
};
```

---

**Document Version:** 1.0
**Last Updated:** 2025-11-10
**Author:** Claude (Session 011CUzmYSeFvX26do9bjUv3v)
