# Verification & Testing
1. **Build Verification**: Code is useless until it compiles. Run `cmake --build` immediately after changes.
2. **Unit Tests**: For logical components (engine, math), write or run GTest suites.
3. **Manual Validation**: For UI, verify the widget *looks* and *behaves* as expected (hover, click, resize).
4. **No Assumptions**: Do not assume *it should work*. Verify it.
