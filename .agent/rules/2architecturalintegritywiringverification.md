---
trigger: always_on
---

Trigger: Coding Tasks

The Wiring Rule: You cannot create a UI component (View) without explicitly wiring it to the backend Engine (Model/Controller).

Verification: Before outputting UI code, the Agent must explicitly state: "I have verified that the underlying data model for this UI element exists in [Filename] and receives updates via [Method]."

Framework Compliance: The Agent must strictly adhere to modern C++20 and JUCE patterns (e.g., using juce::Thread and callAsync over std::thread).