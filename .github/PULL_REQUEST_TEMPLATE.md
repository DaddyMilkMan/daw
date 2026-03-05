## Description

<!-- Briefly describe what this PR changes and why. Link to any related issues. -->

Closes #<!-- issue number -->

## Type of Change

- [ ] Bug fix (non-breaking)
- [ ] New feature (non-breaking)
- [ ] Breaking change
- [ ] Refactor / code quality
- [ ] Documentation only
- [ ] CI / build / tooling

---

## ⚠️ Real-Time (RT) Path Declaration

> **All contributors must complete this section.** Zenith DAW has strict real-time audio thread constraints (see [`docs/tech-briefs/06-audio-thread-safety-policy.md`](../docs/tech-briefs/06-audio-thread-safety-policy.md) and [`docs/THREADING_MODEL.md`](../docs/THREADING_MODEL.md)).

### Does this PR touch any real-time (audio/MIDI callback) code paths?

- [ ] **No** — this PR does not modify any code that runs on the audio thread or within `processBlock` / `getNextAudioBlock` / MIDI callbacks.
- [ ] **Yes** — this PR modifies RT paths (complete the sub-sections below).

#### RT Change Summary
<!-- If "Yes" above: describe exactly which RT paths are changed and what the change does. -->

#### Probable Performance Impact
<!-- Estimate the worst-case per-buffer overhead added or removed (e.g., "< 100 ns per 128-sample block on a 3 GHz core"). Explain your reasoning or measurements. -->

#### RT Safety Analysis
<!-- Confirm the following (check all that apply or explain any exception): -->
- [ ] No heap allocation/deallocation on the audio thread
- [ ] No blocking system calls (mutex lock, file I/O, network, `sleep`) on the audio thread
- [ ] No calls into non-RT-safe third-party code on the audio thread
- [ ] Lock-free data structures used where shared state crosses threads
- [ ] WCET budget not exceeded (see `ZENITH_ENABLE_WCET` in CMake options)

#### Test Procedures for RT Changes
<!-- Describe how reviewers can verify the RT-path change is safe and correct:
     - Which unit/integration tests exercise the changed path?
     - Were TSan or ASan builds run locally? If so, what were the results?
     - Were WCET measurements taken? Attach results if available. -->

---

## Testing

- [ ] All existing tests pass (`ctest --output-on-failure`)
- [ ] New tests added for this change (if applicable)
- [ ] Manually verified on: <!-- list platforms tested, e.g. Linux Ubuntu 22.04, macOS 13 -->

## Checklist

- [ ] Code follows the [Coding Conventions](../docs/CODING_CONVENTIONS.md)
- [ ] Self-review performed
- [ ] Documentation updated (if applicable)
- [ ] No new compiler warnings introduced
