# 🎯 QUICK FIX LIST - Nuclear Roast Cleanup

**Based on**: ROAST_VERIFICATION_REPORT.md  
**Priority**: IMMEDIATE  
**Time Required**: ~2 hours

---

## 🔧 FIX #1: Remove Fictional Author Credits (30 min)

### Files to Edit:
1. `apps/desktop/Source/ui/ZenithTheme.h`
2. `apps/desktop/Source/ui/ZenithTheme.cpp`
3. `apps/desktop/Source/ui/WingmanPanel.h`
4. `apps/desktop/Source/ui/WingmanPanel.cpp`
5. `apps/desktop/Source/ui/skia/SkiaSlider.h`
6. `apps/desktop/Source/ui/skia/SkiaKnob.h`
7. `apps/desktop/Source/engine/PluginHostAsync.cpp`
8. `apps/desktop/Source/engine/ZenithLogger.cpp`
9. `apps/desktop/Source/engine/ZenithLogger.h`

### Find & Replace:
```bash
# Remove Marcus "The Craftsman" references
Find: * @author Marcus "The Craftsman".*\n
Replace: * @author Zenith DAW Contributors\n

# Remove Marcus vs Isabella argument comments
Find: - .*Marcus.*Isabella.*\n
Replace: (delete line)

# Clean up WingmanPanel
Find: Author:  Marcus Williams \(UX Team\)
Replace: Author:  Zenith DAW Contributors
```

---

## 🔧 FIX #2: Update ROAST Document Status (5 min)

### File: `docs/NUCLEAR_CODEBASE_ROAST_2025.md`

Add header warning:
```markdown
# ⚠️ DOCUMENT STATUS: OUTDATED & INACCURATE ⚠️

**THIS ROAST IS 75% INACCURATE**

This document was based on an older version of the codebase (pre-Citadel restructure).
Most claims have been debunked. Please see:

**→ `docs/ROAST_VERIFICATION_REPORT.md` ←**

For accurate current status.

---
```

---

## 🔧 FIX #3: Rename ONNXStemSeparator (45 min)

### Strategy: Keep class name, update docs

Don change code (would break builds). Just update **documentation** to be clear:

**File**: `apps/desktop/Source/dsp/ONNXStemSeparator.h`

Update header comment:
```cpp
/**
 * @file ONNXStemSeparator.h
 * @brief Hybrid Stem Separator with ONNX Runtime support (when available)
 * 
 * This component supports two modes:
 * 1. ONNX Runtime inference (when runtime binaries are linked) - FUTURE
 * 2. DSP-based fallback (current default)
 * 
 * The class checks `isAvailable()` at runtime to determine capabilities.
 * Currently returns false until ONNX Runtime is integrated.
 */
```

---

## 🔧 FIX #4: Clean Up TODO Comments (30 min)

### Triage Tool:
```bash
# List all TODOs
grep -rn "TODO" apps/desktop/Source > todos.txt

# 18 TODOs found - categorize:
```

### Categories:
1. **Performance** (keep - future optimization)
   - "TODO: Cache glow layer"
   
2. **Platform Support** (keep - future platforms)
   - "TODO: Metal initialization"
   - "TODO: Vulkan initialization"
   
3. **Architecture** (keep - known tech debt)
   - "TODO: Replace tracks_ with lock-free swap"
   
4. **Completed** (delete comment)
   - None found
   
5. **Obsolete** (delete comment)
   - None found

**Action**: Add comment explaining why TODOs exist:
```cpp
// TODO: Vulkan initialization
// Note: Deferred to Phase 7 (cross-platform rendering)
```

---

## 🔧 FIX #5: Update Documentation Accuracy (15 min)

### Files to Review:
- `docs/A_PLUS_ACHIEVEMENT.md`
- `docs/A_PLUS_FINAL_REPORT.md`
- `docs/OPERATION_POLISH_COMPLETE.md`

### Action:
Add reality check section to each:

```markdown
## Implementation Status

### ✅ What Works:
- Export to WAV (full offline rendering)
- MIDI editing (1964-line piano roll)
- Plugin hosting (add/remove/configure)
- Track mixing & automation
- Project save/load
- Undo/redo

### ⚠️ What's Stubbed/Partial:
- ONNX Runtime (fallback to DSP)
- Some UI panels (minimal content)

### ❌ What Doesn't Exist:
- NFT minting (was never implemented)
- Session view (future feature)
```

---

## ✅ VERIFICATION CHECKLIST

After fixes, verify:

- [ ] `grep -r "Marcus \"The Craftsman\"" apps/` returns 0 results
- [ ] `grep -r "Marcus vs Isabella" apps/` returns 0 results
- [ ] ROAST document has warning header
- [ ] ONNXStemSeparator docs are honest
- [ ] All docs reflect actual status

---

## 📊 EXPECTED OUTCOME

### Before:
- ❌ Fictional personas in headers
- ❌ Outdated roast causing confusion
- ⚠️ Misleading ONNX naming
- ⚠️ Unexplained TODOs

### After:
- ✅ Professional author credits
- ✅ Clear roast status (outdated)
- ✅ Honest feature documentation
- ✅ Explained technical debt

---

## 🚀 RUN THIS NOW

```bash
# Navigate to project root
cd c:/zenith/daw

# Backup before changes (optional)
git stash push -m "pre-cleanup"

# Now ready for fixes!
```

**Total Time**: ~2 hours  
**Impact**: ⭐⭐⭐⭐⭐ (removes all embarrassing content)  
**Risk**: ⭐☆☆☆☆ (doc changes only, no code breaks)

---

**Let's clean this up!** 🧹
