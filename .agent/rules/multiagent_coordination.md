# Multi-Agent Coordination Rules

The user frequently runs multiple AI agents in parallel on this codebase. This causes resource contention.

## File Lock Handling
1. **Expect Locks**: If you encounter a file lock (e.g., index.lock, Unable to create lock file), assume another agent is actively working. Do NOT panic or treat this as an error.
2. **Wait and Retry**: Wait 5-10 seconds, then retry the operation. Repeat up to 3 times before reporting to the user.
3. **Productive Waiting**: While waiting for a lock to release:
   - Re-analyze your planned changes for correctness.
   - Review the files you've already modified for errors or improvements.
   - Check if your changes align with workspace rules (audio safety, no stubs, etc.).
   - Prepare the next step of your plan.

## Git Coordination
1. **Stale Locks**: If a lock persists after 30+ seconds, it may be stale from a crashed agent. Inform the user and suggest manual removal.
2. **Branch Awareness**: Before modifying files, run git status to see if uncommitted changes exist from another agent's work. Do not blindly overwrite.
3. **Atomic Commits**: Make small, focused commits. This reduces merge conflicts with other agents.

## File Modification Conflicts
1. **Check Modification Time**: Before editing a file, note its last modification time. If it changed while you were working, re-read it before applying edits.
2. **Avoid Parallel Edits**: If you see evidence another agent is editing the same file (e.g., partial changes, lock files), skip that file and move to the next task. Inform the user.
3. **Notify, Don't Fight**: If you cannot proceed due to contention, clearly tell the user which resource is blocked and what you accomplished so far.

## Build System
1. **Build Lock**: Only one agent should run cmake --build at a time. If a build is already running, wait for it to complete or ask the user.
2. **Parallel Safety**: Read operations (iew_file, grep_search) are always safe. Write operations require lock awareness.
