# 🎯 CRITICAL STUB FIX PLANS #5-8: SUMMARY
**Date**: 2025-11-30 20:55 PST

---

## PLAN #5: TRACK SYNCHRONIZER

**File**: `TrackStateSynchronizer.cpp`
**Lines**: 147, 168
**Complexity**: MEDIUM
**Risk**: MEDIUM

### Fix Strategy:
Similar to Clip Sync - bidirectional sync between Engine and ProjectState

### Code Changes:
```cpp
void TrackStateSynchronizer::onTrackAdded(const juce::ValueTree& trackNode)
{
    juce::String trackId = trackNode["id"];
    juce::String trackName = trackNode["name"];
    
    // Create in Engine
    auto* track = engine_->createTrack(trackId, trackName);
    
    // Sync properties
    track->setVolume(trackNode["volume"]);
    track->setPan(trackNode["pan"]);
    track->setMuted(trackNode["muted"]);
    track->setSoloed(trackNode["soloed"]);
}

void TrackStateSynchronizer::onTrackRemoved(const juce::ValueTree& trackNode)
{
    juce::String trackId = trackNode["id"];
    engine_->removeTrack(trackId);
}
```

### Dependencies:
- `Engine::createTrack(id, name)` must exist
- `Engine::removeTrack(id)` must exist

---

## PLAN #6: EXPORT ENGINE

**File**: `ExportEngine.cpp`
**Line**: 281
**Complexity**: MEDIUM-HIGH
**Risk**: MEDIUM

### Fix Strategy:
Add `Engine::renderOffline()` method for faster, more reliable exports

### Code Changes:

**In Engine.h**:
```cpp
void renderOffline(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
```

**In Engine.cpp**:
```cpp
void Engine::renderOffline(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    // Disable real-time mode
    setNonRealtime(true);
    
    // Render in chunks
    const int chunkSize = 512;
    int samplesRendered = 0;
    
    while (samplesRendered < numSamples)
    {
        int samplesToRender = std::min(chunkSize, numSamples - samplesRendered);
        
        juce::AudioBuffer<float> chunkBuffer(buffer.getNumChannels(), samplesToRender);
        
        // Call processBlock
        juce::MidiBuffer emptyMidi;
        processBlock(chunkBuffer, emptyMidi);
        
        // Copy to output buffer
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.copyFrom(ch, startSample + samplesRendered, 
                          chunkBuffer, ch, 0, samplesToRender);
        }
        
        samplesRendered += samplesToRender;
    }
    
    // Re-enable real-time mode
    setNonRealtime(false);
}
```

**In ExportEngine.cpp (line 281)**:
```cpp
// Use Engine's offline rendering
engine_->renderOffline(buffer, 0, totalSamples);
```

### Risk: MEDIUM
- Requires Engine modification
- Non-realtime mode might affect plugins

---

## PLAN #7: PIANO ROLL EDITOR

**File**: `PianoRollEditor.cpp`
**Lines**: 67, 112, 128, 139
**Complexity**: HIGH
**Risk**: HIGH

### Fix Strategy:
Use existing `PianoRollComponent` instead of stub rendering

### Approach A: Replace ContentComponent
**Risk**: HIGH - Major refactor

### Approach B: Delegate to PianoRollComponent
**Risk**: MEDIUM - Add component as child

### Recommended: Approach B

### Code Changes:

**In PianoRollEditor.h**:
```cpp
class ContentComponent : public juce::Component
{
public:
    ContentComponent(ProjectState& ps, const juce::String& tid, const juce::String& cid)
        : projectState(ps), trackId(tid), clipId(cid)
    {
        // Create real piano roll component
        pianoRoll_ = std::make_unique<PianoRollComponent>();
        addAndMakeVisible(pianoRoll_.get());
        
        // Load clip data
        loadClipData();
    }
    
    void paint(juce::Graphics& g) override
    {
        // Just background, component handles its own rendering
        g.fillAll(juce::Colour(0xff2a2a2a));
    }
    
    void resized() override
    {
        pianoRoll_->setBounds(getLocalBounds());
    }
    
private:
    void loadClipData()
    {
        auto clipNode = projectState.getClipNode(trackId, clipId);
        if (!clipNode.isValid()) return;
        
        auto notesNode = clipNode.getChildWithName("NOTES");
        std::vector<MidiNote> notes;
        
        for (int i = 0; i < notesNode.getNumChildren(); ++i)
        {
            auto noteNode = notesNode.getChild(i);
            MidiNote note;
            note.pitch = noteNode["pitch"];
            note.startBeat = noteNode["start"];
            note.duration = noteNode["duration"];
            note.velocity = noteNode["velocity"];
            notes.push_back(note);
        }
        
        pianoRoll_->setNotes(notes);
    }
    
    ProjectState& projectState;
    juce::String trackId;
    juce::String clipId;
    std::unique_ptr<PianoRollComponent> pianoRoll_;
};
```

### Risk: HIGH
- Requires `PianoRollComponent` to be fully functional
- Need to wire up note editing callbacks
- Complex integration

---

## PLAN #8: ARRANGER VIEW

**File**: `ArrangerView.cpp`
**Lines**: 77, 245, 251, 399, 414
**Complexity**: HIGH
**Risk**: HIGH

### Fix Strategy:
Similar to Piano Roll - use existing `ArrangementComponent`

### Code Changes:

**In ArrangerView.cpp**:
```cpp
void ArrangerView::paint(juce::Graphics& g)
{
    // Delegate to ArrangementComponent
    if (arrangementComponent_)
    {
        arrangementComponent_->paint(g);
    }
}

void ArrangerView::rebuildTrackComponents()
{
    trackComponents_.clear();
    
    for (auto* track : engine_->getTracks())
    {
        auto* trackComp = new TrackComponent(track);
        addAndMakeVisible(trackComp);
        trackComponents_.push_back(trackComp);
    }
    
    resized();
}

ClipComponent* ArrangerView::findClipAtPosition(int x, int y)
{
    for (auto* trackComp : trackComponents_)
    {
        auto* clip = trackComp->findClipAt(x, y);
        if (clip) return clip;
    }
    return nullptr;
}
```

### Risk: HIGH
- Requires `ArrangementComponent` to be complete
- Need track component hierarchy
- Complex UI integration

---

## 📊 OVERALL SUMMARY

| Plan | Complexity | Risk | Est. Time | Ready? |
|------|-----------|------|-----------|--------|
| #3 Preset Browser | LOW-MED | LOW | 30min | ✅ YES |
| #4 Clip Sync | MEDIUM | MEDIUM | 1hr | ⚠️ NEEDS REVIEW |
| #5 Track Sync | MEDIUM | MEDIUM | 45min | ⚠️ NEEDS REVIEW |
| #6 Export Engine | MED-HIGH | MEDIUM | 45min | ⚠️ NEEDS ENGINE MOD |
| #7 Piano Roll | HIGH | HIGH | 2hr | ❌ RISKY |
| #8 Arranger | HIGH | HIGH | 2hr | ❌ RISKY |

---

## 🎯 RECOMMENDED IMPLEMENTATION ORDER

1. **Preset Browser** (Plan #3) - Safest, immediate value
2. **Track Sync** (Plan #5) - Similar to Clip Sync
3. **Clip Sync** (Plan #4) - Enables recording
4. **Export Engine** (Plan #6) - Requires Engine mod
5. **Piano Roll** (Plan #7) - High risk, manual review needed
6. **Arranger** (Plan #8) - High risk, manual review needed

---

## ⚠️ BLOCKERS

### Before implementing ANY plan:
1. Verify Engine API methods exist
2. Verify ProjectState methods exist
3. Create unit tests for each component
4. Have rollback plan (git commits)

### For Piano Roll & Arranger:
1. Verify `PianoRollComponent` is complete
2. Verify `ArrangementComponent` is complete
3. Manual code review required
4. Incremental testing at each step

---

**All plans created**: YES
**Ready to implement**: Plans #3, #5 (with verification)
**Needs review**: Plans #4, #6
**High risk**: Plans #7, #8
