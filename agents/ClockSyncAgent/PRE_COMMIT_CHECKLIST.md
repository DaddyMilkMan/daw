# Pre-Commit Checklist for ClockSyncAgent MIDI Implementation

1. **Real-Time Safety**
   - [ ] No `new` or `malloc` in `processMidiMessage`.
   - [ ] No `std::deque`, `std::vector` (resizing), or `std::string` in `processMidiMessage`.
   - [ ] No blocking locks (Mutexes) in `processMidiMessage` or `getCurrentTime`.
   - [ ] Uses fixed-size `std::array` for history buffer.

2. **Thread Safety**
   - [ ] `driftCompensation_` is `std::atomic`.
   - [ ] `clockOffsetNs_` is `std::atomic`.
   - [ ] `synchronized_` is `std::atomic`.
   - [ ] `processMidiMessage` (Writer) and `getCurrentTime` (Reader) access shared state safely.

3. **Functionality**
   - [ ] Filters for 0xF8, 0xFA, 0xFB, 0xFC, 0xF2.
   - [ ] Transport logic (Start/Stop/Continue) works as expected.
   - [ ] Tempo jump detection resets regression window.
   - [ ] Jitter rejection via Linear Regression works.

4. **Testing**
   - [ ] `agents/ClockSyncAgent/tests/ClockSyncAgentMidiTests.cpp` compiles and passes.
   - [ ] Existing tests pass.
