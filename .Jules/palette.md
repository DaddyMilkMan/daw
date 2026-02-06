## 2025-05-20 - Keyboard Accessibility in Custom Components
**Learning:** Custom UI components (like ZenithButton) must manually handle Space/Enter keys for accessibility, as they don't inherit native button behaviors.
**Action:** Always implement `keyPressed` to map `returnKey` and `spaceKey` to `onClick` for interactive custom components.
