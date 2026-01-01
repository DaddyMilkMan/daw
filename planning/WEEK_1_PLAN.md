# 📅 ZENITH DAW - Week 1 Development Plan

**Week Start:** January 1, 2026  
**Focus:** High-Priority Foundation Tasks

---

## Priority 1: Wire Up Real Grok API (4 hours)
**Files:** `apps/desktop/Source/network/AIBridgeClient.cpp`, `ui/WingmanPanel.cpp`

### Tasks
- [ ] Add API Key Configuration (Settings → AI → Grok API Key)
- [ ] Replace MockAIProvider with real GrokAPIClient
- [ ] Implement error handling (timeout, invalid key, rate limiting)
- [ ] Test with live API key

### Success Criteria
- Real Grok API responses in Wingman panel
- Graceful error messages when network unavailable
- API key persistently configurable

---

## Priority 2: Plugin Crash Recovery (8 hours)
**Files:** `apps/desktop/Source/engine/PluginHost.cpp`

### Tasks
- [ ] Create out-of-process plugin scanner (PluginScanner.exe)
- [ ] Implement subprocess communication with timeout
- [ ] Add plugin blacklist system
- [ ] Add progress UI for scanning

### Success Criteria
- Plugin scan never hangs app
- Crashed plugins auto-blacklisted
- Can scan 1000+ plugins safely

---

## Priority 3: Thread Safety Audit (16 hours)
**Files:** Multiple engine files

### Tasks
- [ ] Audit audio thread for CriticalSection usage
- [ ] Replace mutexes with lock-free alternatives (AbstractFifo)
- [ ] Add thread safety assertions
- [ ] Enable TSAN on Linux builds

### Success Criteria
- No mutexes in audio callback
- No heap allocations in audio callback
- TSAN reports no data races

---

## Agent Assignments

| Agent | Primary Task | Secondary Task |
|-------|--------------|----------------|
| EngineAgent | Thread Safety | - |
| NetworkAgent | Grok API | - |
| PluginAgent | Crash Recovery | - |
| UIAgent | API Key Settings | Progress UI |
| TestAgent | TSAN integration | Regression tests |

---

## Daily Checkpoints

| Day | Expected Progress |
|-----|-------------------|
| Mon | Grok API skeleton, Scanner subprocess created |
| Tue | Grok API complete, Blacklist system |
| Wed | Plugin scan UI, Thread audit started |
| Thu | Audio thread fixes, Lock-free migration |
| Fri | TSAN validation, Integration testing |

---

## Quality Gates
- [ ] All 38 existing tests pass
- [ ] No new warnings introduced
- [ ] Build time < 5 minutes (incremental)
- [ ] Documentation updated for new features
