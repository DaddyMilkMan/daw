# 🎯 CRITICAL STUB FIX PLAN #4: CLIP SYNCHRONIZER
**Date**: 2025-11-30 20:52 PST
**Complexity**: MEDIUM-HIGH
**Risk**: MEDIUM
**Est. Time**: 1 hour

---

## 📋 CURRENT STATE

**File**: `ClipSynchronizer.cpp`
**Stubs**:
- Line 43: `createClip()` - Stub comment
- Line 101: `syncFromEngine()` - Stub comment

**Problem**: Clips created in Engine (during recording) don't appear in ProjectState (UI doesn't see them)

---

## 🏗️ ARCHITECTURE DECISION NEEDED

### Option A: Poll-Based Sync
- Timer checks Engine for new clips every 100ms
- Simple but inefficient
- **Risk**: LOW

### Option B: Event-Based Sync
- Engine fires callback when clip created
- Efficient but requires Engine modification
- **Risk**: MEDIUM

### Option C: Hybrid
- Poll during recording, event-based otherwise
- Best of both worlds
- **Risk**: MEDIUM

**Recommendation**: Start with Option A (poll-based), migrate to Option C later

---

## 🔧 FIX #1: createClip() (Line 43-50)

### Current Code:
```cpp
void ClipSynchronizer::createClip(const juce::String& trackId, double startBeat, double endBeat)
{
    // Integration stub: Would create clip in both Engine and ProjectState
    // TODO (when U3 recording branch is merged):
    // 1. Create clip in Engine track
    // 2. Create corresponding CLIP node in ProjectState
    // 3. Link them via clip ID
}
```

### New Code:
```cpp
void ClipSynchronizer::createClip(const juce::String& trackId, double startBeat, double endBeat)
{
    DBG("ClipSynchronizer: Creating clip for track " + trackId);
    
    // 1. Create clip in Engine
    auto* engineTrack = engine_->getTrackById(trackId);
    if (!engineTrack)
    {
        DBG("ClipSynchronizer: ERROR - Track not found in Engine: " + trackId);
        return;
    }
    
    auto* engineClip = engineTrack->createMidiClip(startBeat, endBeat);
    if (!engineClip)
    {
        DBG("ClipSynchronizer: ERROR - Failed to create clip in Engine");
        return;
    }
    
    juce::String clipId = engineClip->getId();
    
    // 2. Create corresponding node in ProjectState
    auto trackNode = projectState_->getTrackNode(trackId);
    if (!trackNode.isValid())
    {
        DBG("ClipSynchronizer: ERROR - Track not found in ProjectState: " + trackId);
        return;
    }
    
    auto clipsNode = trackNode.getOrCreateChildWithName("CLIPS", nullptr);
    
    juce::ValueTree clipNode("CLIP");
    clipNode.setProperty("id", clipId, nullptr);
    clipNode.setProperty("start", startBeat, nullptr);
    clipNode.setProperty("end", endBeat, nullptr);
    clipNode.setProperty("type", "midi", nullptr);
    
    // Create NOTES container
    juce::ValueTree notesNode("NOTES");
    clipNode.appendChild(notesNode, nullptr);
    
    clipsNode.appendChild(clipNode, nullptr);
    
    DBG("ClipSynchronizer: Created clip " + clipId + " in both Engine and ProjectState");
}
```

### Dependencies:
- `Engine::getTrackById()` must exist
- `Track::createMidiClip()` must exist
- `ProjectState::getTrackNode()` must exist

---

## 🔧 FIX #2: syncFromEngine() (Line 101-107)

### Current Code:
```cpp
void ClipSynchronizer::syncFromEngine()
{
    // Integration stub: Would check for new clips in Engine tracks
    // TODO (when U3 recording branch is merged):
    // 1. Iterate Engine tracks
    // 2. Check for clips not in ProjectState
    // 3. Add missing clips to ProjectState
}
```

### New Code:
```cpp
void ClipSynchronizer::syncFromEngine()
{
    DBG("ClipSynchronizer: Syncing from Engine...");
    
    int clipsAdded = 0;
    
    // Iterate all Engine tracks
    for (auto* engineTrack : engine_->getTracks())
    {
        juce::String trackId = engineTrack->getId();
        
        // Get corresponding ProjectState track
        auto trackNode = projectState_->getTrackNode(trackId);
        if (!trackNode.isValid())
        {
            DBG("ClipSynchronizer: WARNING - Track " + trackId + " not in ProjectState");
            continue;
        }
        
        auto clipsNode = trackNode.getOrCreateChildWithName("CLIPS", nullptr);
        
        // Check each clip in Engine
        for (auto* engineClip : engineTrack->getClips())
        {
            juce::String clipId = engineClip->getId();
            
            // Check if clip exists in ProjectState
            bool foundInProjectState = false;
            for (int i = 0; i < clipsNode.getNumChildren(); ++i)
            {
                auto clipNode = clipsNode.getChild(i);
                if (clipNode["id"].toString() == clipId)
                {
                    foundInProjectState = true;
                    break;
                }
            }
            
            // If not found, add it
            if (!foundInProjectState)
            {
                DBG("ClipSynchronizer: Found new clip in Engine: " + clipId);
                
                juce::ValueTree newClipNode("CLIP");
                newClipNode.setProperty("id", clipId, nullptr);
                newClipNode.setProperty("start", engineClip->getStartBeat(), nullptr);
                newClipNode.setProperty("end", engineClip->getEndBeat(), nullptr);
                newClipNode.setProperty("type", "midi", nullptr);
                
                // Create NOTES container
                juce::ValueTree notesNode("NOTES");
                
                // Copy MIDI notes from Engine clip
                auto midiData = engineClip->getMidiData();
                for (int i = 0; i < midiData.getNumEvents(); ++i)
                {
                    auto event = midiData.getEventPointer(i);
                    if (event->message.isNoteOn())
                    {
                        juce::ValueTree noteNode("NOTE");
                        noteNode.setProperty("id", juce::Uuid().toString(), nullptr);
                        noteNode.setProperty("pitch", event->message.getNoteNumber(), nullptr);
                        noteNode.setProperty("velocity", event->message.getVelocity(), nullptr);
                        noteNode.setProperty("start", event->message.getTimeStamp() / 960.0, nullptr);
                        
                        // Find corresponding note-off for duration
                        auto noteOff = midiData.getNextEventPointer(i, true);
                        if (noteOff && noteOff->message.isNoteOff())
                        {
                            double duration = (noteOff->message.getTimeStamp() - event->message.getTimeStamp()) / 960.0;
                            noteNode.setProperty("duration", duration, nullptr);
                        }
                        
                        notesNode.appendChild(noteNode, nullptr);
                    }
                }
                
                newClipNode.appendChild(notesNode, nullptr);
                clipsNode.appendChild(newClipNode, nullptr);
                
                clipsAdded++;
            }
        }
    }
    
    if (clipsAdded > 0)
    {
        DBG("ClipSynchronizer: Added " + juce::String(clipsAdded) + " clips to ProjectState");
    }
}
```

### Dependencies:
- `Engine::getTracks()` must exist
- `Track::getId()`, `Track::getClips()` must exist
- `Clip::getId()`, `Clip::getStartBeat()`, `Clip::getEndBeat()`, `Clip::getMidiData()` must exist

---

## 🔧 FIX #3: Add Timer for Auto-Sync

### New Member Variable (in header):
```cpp
class ClipSynchronizer : public juce::Timer
{
    // ... existing code ...
    
    void timerCallback() override
    {
        syncFromEngine();
    }
};
```

### In Constructor:
```cpp
ClipSynchronizer::ClipSynchronizer(Engine& engine, ProjectState& projectState)
    : engine_(&engine), projectState_(&projectState)
{
    // Start timer to sync every 100ms during recording
    startTimer(100);
}
```

---

## ✅ TESTING PLAN

### Test 1: Manual Clip Creation
1. Call `createClip("track1", 0.0, 4.0)`
2. Verify clip appears in Engine
3. Verify clip appears in ProjectState
4. Verify clip has NOTES container

### Test 2: Recording Sync
1. Start recording on a track
2. Play some MIDI notes
3. Stop recording
4. Verify clip appears in ProjectState within 100ms
5. Verify MIDI notes are copied correctly

### Test 3: Multiple Clips
1. Record multiple clips on different tracks
2. Verify all clips sync correctly
3. Verify no duplicates

---

## 📊 RISK ASSESSMENT

| Component | Risk | Mitigation |
|-----------|------|------------|
| createClip | MEDIUM | Error checking at each step |
| syncFromEngine | MEDIUM | Check for duplicates |
| Timer | LOW | Standard JUCE pattern |
| MIDI note copy | HIGH | Careful timestamp conversion |

**Overall Risk**: MEDIUM

---

## ⚠️ POTENTIAL ISSUES

1. **Timestamp Conversion**: MIDI timestamps vs. beat positions
2. **Note-Off Matching**: Finding correct note-off for duration
3. **Performance**: Syncing every 100ms might be expensive
4. **Race Conditions**: Engine and ProjectState on different threads

---

## 🎯 IMPLEMENTATION ORDER

1. Add Timer inheritance to header
2. Implement `createClip()` with full error checking
3. Implement `syncFromEngine()` without MIDI note copying
4. Test basic clip sync
5. Add MIDI note copying
6. Test with recording

---

**Ready to implement**: NEEDS REVIEW
**Blockers**: Need to verify Engine API methods exist
**Alternative**: Use event-based sync if Engine supports callbacks
