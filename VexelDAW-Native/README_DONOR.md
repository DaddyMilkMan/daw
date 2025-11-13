# 📦 DONOR REFERENCE CODE – DO NOT EXTEND

This directory contains the **VexelDAW-Native donor implementation**.

## Status

- ✅ Served as reference for Zenith Phase 0-1 implementation
- ❌ **NOT COMPILED** (CMake flag: `ZENITH_ENABLE_VEXEL_DONOR=OFF`)
- ❌ **NOT MAINTAINED** going forward
- 📚 Kept as reference documentation only

## Canonical Implementations

All active code is in: **`/zenith-core/Source/engine/`**

| Component | Donor (Reference) | Canonical (Active) |
|-----------|-------------------|-------------------|
| Audio Engine | `Source/Audio/AudioEngine.*` | `zenith-core/include/Engine.h` |
| Clip Rendering | `Source/Audio/Clip.*` | `zenith-core/Source/engine/Clip.*` |
| Track Processing | `Source/Audio/Track.*` | `zenith-core/Source/engine/Track.*` |
| Audio File Pool | *(None)* | `zenith-core/Source/engine/AudioFilePool.*` |

## Key Design Differences

The canonical Zenith implementation differs from this donor code:

### **1. Clip Timing**
- **Donor:** Uses internal `transportPosition` member (stateful, requires updates)
- **Canonical:** Playhead passed as parameter (stateless, explicit dependency)

### **2. Audio File Loading**
- **Donor:** No centralized file pool
- **Canonical:** AudioFilePool for RT-safe pre-loaded buffers

### **3. JUCE Version**
- **Donor:** JUCE 7.x
- **Canonical:** JUCE 8.0.9

### **4. RT-Safety**
- **Donor:** Some allocations on audio thread
- **Canonical:** Strict RT-safety (pre-allocated buffers, atomics, shared_ptr)

---

**Do not modify files in this directory.**
**For new features, work in `/zenith-core/`.**

---

**Last Updated:** 2025-11-13 (Phase 1.4 complete)
