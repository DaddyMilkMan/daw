# Architectural Gates
1. **No ECS Re-entry**: The project is Standardized on ProjectState and Engine. Flecs integration is ABANDONED. Do not suggest or re-introduce ECS patterns.
2. **Code Structures**:
    *   Source/engine: Audio processing and internal state (Headless/Tests).
    *   Source/ui: Visual presentation (Skia/JUCE).
    *   Source/ai: Generative Logic and Agents.
3. **State Management**: The UI is a view of ProjectState. All mutations go through Engine or CommandAPI.
