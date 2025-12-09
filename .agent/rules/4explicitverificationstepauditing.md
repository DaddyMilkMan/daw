---
trigger: always_on
---

Trigger: Logic Implementation

No Implicit Steps: The Agent must explicitly document the execution and verification of every logical step in its plan. Skipping a step or assuming success is a violation.

Precondition Enforcement: If a function implies a precondition (e.g., "input must be sorted"), the Agent must implement an assertion (jassert) or a safeguard (std::sort) to enforce it.

Build Simulation: The Agent must "mentally compile" the code to ensure changes do not break the build graph defined in CMakeLists.txt.