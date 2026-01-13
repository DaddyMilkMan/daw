# TestingAgent

## Purpose
Automates comprehensive testing workflows including unit tests, integration tests, audio-specific tests, and real-time safety validation. Coordinates test execution and result reporting.

## Triggers
- Code changes committed
- Pull request reviews
- Manual test execution requests
- Scheduled test runs

## Outputs
- Test execution results
- Code coverage reports
- Real-time safety validation reports
- Test failure diagnostics

## Acceptance Criteria
- [ ] Python module imports successfully
- [ ] Follows Zenith DAW Python coding conventions
- [ ] Executes both C++ and Python tests
- [ ] Validates real-time safety constraints
- [ ] Generates comprehensive test reports
- [ ] Includes unit tests for test orchestration

## TODO Checklist

### Phase 1: Basic Structure
- [ ] Define test execution interface
- [ ] Implement test discovery mechanism
- [ ] Add test result collection

### Phase 2: Core Functionality
- [ ] Implement C++ test execution (ZenithDAWTests)
- [ ] Add Python test execution (pytest)
- [ ] Implement audio-specific test runners
- [ ] Add real-time safety validation

### Phase 3: Advanced Features
- [ ] Implement code coverage collection
- [ ] Add parallel test execution
- [ ] Implement test result analysis
- [ ] Add failure reproduction helpers

### Phase 4: Testing & Documentation
- [ ] Write unit tests for test orchestration
- [ ] Add integration tests
- [ ] Document testing strategies
- [ ] Add examples for custom test types

## Dependencies
- pytest (Python testing)
- gcov/lcov (C++ coverage)
- JUCE testing framework

## Related Documentation
- [Developer Guide](../../docs/DEVELOPER.md)
- [Testing Documentation](../../apps/desktop/Source/tests/)
- [RT Safety Checklist](../../docs/RT_SAFETY_CODE_REVIEW_CHECKLIST.md)
