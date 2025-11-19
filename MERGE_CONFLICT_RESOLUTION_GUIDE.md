# Merge Conflict Resolution Guide

## Summary

This guide documents the merge conflicts found across 7 branches when attempting to merge them into the main branch (`claude/daw-features-comparison-011CUv5TyhjPCXSWeZJXTbs7`).

**Main Branch:** `claude/daw-features-comparison-011CUv5TyhjPCXSWeZJXTbs7` (commit: a35e450)

## Conflict Analysis

### Branch 1: `claude/ui-freeze-engine-adapter-011CUyqXTyp2NMA5M8qbPEZT`
**Status:** 5 files with conflicts

**Conflicting Files:**
1. `zenith-daw/src/renderer/App.tsx`
2. `zenith-daw/src/renderer/components/CenterPanel.tsx`
3. `zenith-daw/src/renderer/components/RightPanel.tsx`
4. `zenith-daw/src/renderer/components/TransportBar.tsx`
5. `zenith-daw/src/renderer/components/WingmanSidebar.tsx`

**Conflict Nature:**
- Branch uses old `useAudioStore` architecture
- Main uses newer `engineClient` architecture
- Need to migrate all audio store calls to engineClient commands

**Resolution Strategy:**
```typescript
// OLD (branch):
const { play, pause } = useAudioStore();
play();

// NEW (main - use engineClient):
import { engineClient } from '@/lib/engineClient';
engineClient.sendCommand('transport:play');

// For state, use Zustand store:
const projectState = useProjectState();
projectState.isPlaying // instead of transport.isPlaying
```

**Detailed Resolutions:**

**App.tsx:**
- Remove `useAudioStore` import
- Use `engineClient` for all transport commands
- Use `projectState` from Zustand instead of audio store state
- Keep `engineClient.connect()` initialization
- Use `wingmanOpen` from preferences instead of local state

**RightPanel.tsx:**
- Keep `track.volume`, `track.pan`, `track.muted`, `track.soloed` for reading values
- Replace handler functions with `engineClient.sendCommand()` calls:
  ```typescript
  onChange={(e) => {
    const newVolume = parseFloat(e.target.value);
    engineClient.sendCommand('track:setVolume', { trackId: track.id, volume: newVolume });
  }}
  ```

**TransportBar.tsx:**
- Change prop from `transport: TransportState` to `audioState: any`
- Replace all `transport.` references with `audioState.`
- Replace all audio store actions with engineClient commands
- Remove `useAudioStore` import

**CenterPanel.tsx:**
- Keep both `engineClient` and `SessionView` imports
- Use `engineClient.sendCommand('track:create', { name, type, color })` for track creation
- Remove `useAudioStore` usage

---

### Branch 2: `claude/zenith-audio-clip-editing-011CUx2oSfAJoTPh5os1NENs`
**Status:** 2 files with conflicts

**Conflicting Files:**
1. `zenith-daw/src/renderer/App.tsx`
2. `zenith-daw/src/renderer/components/CenterPanel.tsx`

**Conflict Nature:**
- Similar to Branch 1 - old architecture vs new
- Adds clip editing features that need to integrate with engineClient

**Resolution Strategy:**
- Apply same engineClient migration as Branch 1
- Ensure clip editing commands go through engineClient

---

### Branch 3: `claude/zenith-audio-midi-recording-011CUx2uD1Yd9qQ2sZt7ceqh`
**Status:** 1 file with conflicts

**Conflicting Files:**
1. `zenith-daw/package.json`

**Conflict Nature:**
- Dependency version conflicts
- Different packages added on each branch

**Resolution Strategy:**
- Manually merge package.json
- Keep all unique dependencies
- Use newer versions where conflicts exist
- Run `npm install` after resolution

---

### Branch 4: `claude/zenith-phase2-project-save-load-011CUx1z2XmdGFfXVcABUrkf`
**Status:** 4 files with conflicts

**Conflicting Files:**
1. `zenith-daw/src/main/index.js`
2. `zenith-daw/src/main/preload.js`
3. `zenith-daw/src/renderer/App.tsx`
4. `zenith-daw/src/renderer/types/electron.d.ts`

**Conflict Nature:**
- Adds Electron IPC handlers for save/load
- Main branch may have different IPC structure

**Resolution Strategy:**
- Keep both sets of IPC handlers
- Merge electron.d.ts type definitions
- Ensure no duplicate handler names
- Update App.tsx to use engineClient architecture

---

### Branch 5: `claude/zenith-plugin-system-011CUx28AkcQynMgXKqa7VRv`
**Status:** 1 file with conflicts

**Conflicting Files:**
1. `zenith-daw/src/renderer/components/RightPanel.tsx`

**Conflict Nature:**
- Adds plugin UI to RightPanel
- Conflicts with engineClient migration

**Resolution Strategy:**
- Apply same RightPanel resolution as Branch 1
- Keep plugin UI additions
- Ensure plugin actions use engineClient

---

### Branch 6: `claude/zenith-tempo-metronome-011CUx2rKUdvrbdCZSo11TMy`
**Status:** 3 files with conflicts

**Conflicting Files:**
1. `zenith-daw/src/renderer/App.tsx`
2. `zenith-daw/src/renderer/components/CenterPanel.tsx`
3. `zenith-daw/src/renderer/components/TransportBar.tsx`

**Conflict Nature:**
- Adds tempo/metronome features
- Same architecture migration needed

**Resolution Strategy:**
- Apply same resolutions as Branch 1
- Ensure tempo changes use `engineClient.sendCommand('transport:setTempo', { tempo })`
- Keep metronome UI additions

---

### Branch 7: `claude/zenith-tempo-metronome-011CUx2sYYUutsrYmrZxrKS5`
**Status:** 2 files with conflicts

**Conflicting Files:**
1. `zenith-daw/src/renderer/App.tsx`
2. `zenith-daw/src/renderer/components/TransportBar.tsx`

**Conflict Nature:**
- Duplicate of Branch 6 (different session)
- Same conflicts and resolutions

**Resolution Strategy:**
- Apply same resolutions as Branch 6

---

## Step-by-Step Resolution Process

### For Each Branch:

1. **Checkout the branch**
   ```bash
   git checkout <branch-name>
   ```

2. **Merge main into the branch**
   ```bash
   git merge origin/claude/daw-features-comparison-011CUv5TyhjPCXSWeZJXTbs7
   ```

3. **Resolve conflicts** following the strategies above

4. **Test the resolution** (if possible)

5. **Commit the merge**
   ```bash
   git add .
   git commit -m "Merge main branch and resolve conflicts"
   ```

6. **Push the branch**
   ```bash
   git push origin <branch-name>
   ```

---

## Common Conflict Patterns

### Pattern 1: Audio Store → Engine Client Migration

**Old Code:**
```typescript
const { play, pause, stop } = useAudioStore();
play();
```

**New Code:**
```typescript
engineClient.sendCommand('transport:play');
```

### Pattern 2: State Access

**Old Code:**
```typescript
const { transport } = useAudioStore();
if (transport.isPlaying) { ... }
```

**New Code:**
```typescript
const projectState = useProjectState();
if (projectState.isPlaying) { ... }
```

### Pattern 3: Track Mutations

**Old Code:**
```typescript
setTrackVolume(trackId, volume);
```

**New Code:**
```typescript
engineClient.sendCommand('track:setVolume', { trackId, volume });
```

---

## Architecture Changes Summary

The main branch has migrated from a direct Web Audio API integration (`useAudioStore`) to an adapter pattern (`engineClient`) that allows swapping between web and native audio engines.

**Key Changes:**
1. All audio operations now go through `engineClient.sendCommand()`
2. State is managed by Zustand store instead of audio store
3. Engine events are subscribed via `engineClient.onEvent()`
4. Enables future native C++/JUCE engine integration

---

## Notes

- The architecture change is a significant refactor
- All branches created before this refactor will have conflicts
- Future branches should use the engineClient pattern from the start
- Consider rebasing very old branches instead of merging if they have extensive changes

---

## Quick Reference: Common Commands

### Transport Commands
```typescript
engineClient.sendCommand('transport:play');
engineClient.sendCommand('transport:pause');
engineClient.sendCommand('transport:stop');
engineClient.sendCommand('transport:setTempo', { tempo: 120 });
engineClient.sendCommand('transport:toggleLoop');
```

### Track Commands
```typescript
engineClient.sendCommand('track:create', { name, type, color });
engineClient.sendCommand('track:setVolume', { trackId, volume });
engineClient.sendCommand('track:setPan', { trackId, pan });
engineClient.sendCommand('track:setMute', { trackId, muted });
engineClient.sendCommand('track:setSolo', { trackId, solo });
```

---

Generated: 2025-11-10
Main Branch: claude/daw-features-comparison-011CUv5TyhjPCXSWeZJXTbs7 (a35e450)
