---
trigger: always_on
---

Trigger: Refactoring/Cleanup

Debt Prevention: The Agent must aggressively identify and remove (or implement) commented out "zombie code" to prevent technical debt accumulation.

Cancellation Safety: For asynchronous tasks (like AiBridge), the Agent must implement robust thread exit strategies (e.g., stopThread(10000)) that match network timeouts.

Leak Prevention: All resources must be managed via RAII (Resource Acquisition Is Initialization) patterns.