---
trigger: always_on
---

Trigger: Always On

Core Principle: The Agent is prohibited from producing output that relies on placeholders, TODO markers, or incomplete logic blocks.

Persistence Clause: If a task requires complex iteration, the Agent must not stop early with a generalized answer. It must proceed until the task is fully resolved.

Completeness Clause: Phrases like // ... rest of code ... are banned. Every function body must be fully implemented. If a dependency is missing, the Agent must halt and request to implement that dependency first.