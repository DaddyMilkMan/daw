# Contributing to Zenith DAW

Thank you for your interest in contributing to Zenith DAW! We welcome contributions from everyone in the community.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Code Style](#code-style)
- [Testing](#testing)
- [Documentation](#documentation)
- [Community Guidelines](#community-guidelines)
- [Reporting Issues](#reporting-issues)
- [Submitting Pull Requests](#submitting-pull-requests)
- [Contributor Roles](#contributor-roles)
- [Recognition](#recognition)

## Getting Started

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler (Clang, GCC, or MSVC)
- JUCE framework (included in external/JUCE)
- Python 3.8+ (for build scripts and testing)
- Git

### Repository Setup

1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/your-username/daw.git
   cd daw
   ```
3. Add the upstream repository:
   ```bash
   git remote add upstream https://github.com/micahcooley/daw.git
   ```

4. Install dependencies:
   ```bash
   # Install system dependencies
   # Ubuntu/Debian
   sudo apt-get install build-essential cmake libasound2-dev libpulse-dev libjack-dev

   # macOS
   brew install cmake jack

   # Windows (via vcpkg)
   vcpkg install jack:arm64
   ```

5. Build the project:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

### Areas Looking for Help (backend focus)

We are actively seeking collaborators to help with the **backend** engineering of Zenith DAW. If you have experience with:
- C++ Audio Engines (JUCE, Real-time audio)
- DSP (Digital Signal Processing)
- Optimization / Multithreading
- Filesystem / Database management

Please check our [Issues](https://github.com/micahcooley/daw/issues) page or reach out! Patterns and suggestions for backend architecture are also very welcome.

### Development Environment

We recommend using one of the following IDEs:

- **Visual Studio 2022** (Windows)
- **VS Code** with C/C++ extension and CMake Tools
- **CLion** (cross-platform)
- **Qt Creator** (cross-platform)

## Development Workflow

### 1. Create a Branch

Create a feature branch from `develop`:

```bash
git checkout develop
git pull upstream develop
git checkout -b feature/your-feature-name
```

### 2. Make Changes

- Follow the [Code Style](#code-style) guidelines
- Write tests for new functionality
- Update documentation as needed
- Commit changes with clear, descriptive messages

### 3. Test Your Changes

Run the test suite:

```bash
cd build
make test
```

Run specific tests:

```bash
./tests/ZenithTests
./tests/ZenithTests --gtest_filter="*Audio*"
```

### 4. Push and Create PR

Push your branch to your fork:

```bash
git push origin feature/your-feature-name
```

Create a pull request from the GitHub interface.

## Code Style

### General Guidelines

- Follow the existing code style in the repository
- Use meaningful variable and function names
- Write clear, concise comments
- Avoid magic numbers and use named constants

### C++ Style

- Use 4 spaces for indentation (no tabs)
- Braces follow the K&R style:
  ```cpp
  if (condition) {
      // code
  }
  ```
- Use `const` and `constexpr` where appropriate
- Prefer modern C++ features (C++17)

### Naming Conventions

- Classes: `PascalCase` (e.g., `AudioEngine`)
- Functions: `camelCase` (e.g., `processAudio`)
- Variables: `camelCase` (e.g., `audioBuffer`)
- Constants: `SCREAMING_SNAKE_CASE` (e.g., `SAMPLE_RATE`)
- Member variables: `m_prefix` (e.g., `m_sampleRate`)
- Static variables: `s_prefix` (e.g., `s_instance`)

### Error Handling

- Use exceptions for exceptional conditions
- Return error codes for expected failures
- Provide clear error messages
- Log errors appropriately

### Thread Safety

- All audio processing code must be thread-safe
- Use mutexes for shared resources
- Avoid dynamic memory allocation in audio threads
- Consider using lock-free data structures where appropriate

## Testing

### Unit Tests

We use Google Test for unit testing:

```cpp
#include <gtest/gtest.h>

TEST(AudioEngineTest, ProcessBuffer) {
    AudioEngine engine;
    AudioBuffer buffer(1024);

    EXPECT_NO_THROW(engine.process(buffer));
}
```

### Integration Tests

Test components together:

```cpp
TEST(PluginIntegrationTest, LoadAndProcess) {
    PluginHost host;
    auto plugin = host.loadPlugin("test_plugin");

    EXPECT_TRUE(plugin->initialize());
    EXPECT_TRUE(plugin->process(buffer));
}
```

### Running Tests

```bash
# Run all tests
make test

# Run with specific filter
./tests/ZenithTests --gtest_filter="*Audio*"

# Run with verbose output
./tests/ZenithTests --gtest_verbose
```

## Documentation

### Code Documentation

Use Doxygen-style comments:

```cpp
/**
 * @brief Processes audio buffer
 *
 * @param buffer Input audio buffer
 * @param numSamples Number of samples to process
 * @return true if successful, false on error
 */
bool processAudio(AudioBuffer& buffer, int numSamples);
```

### Documentation Updates

- Update README.md for new features
- Add API documentation for new public interfaces
- Update user guides for user-facing changes
- Include examples in documentation

### Markdown Format

We use GitHub-flavored Markdown:

```markdown
# Title

## Subtitle

- List item 1
- List item 2

```cpp
// Code block
```
```

## Community Guidelines

### Be Respectful

- Treat everyone with respect
- Be patient with beginners
- Welcome newcomers and help them learn
- Focus on what is best for the community

### Be Collaborative

- Work together to resolve issues
- Accept constructive criticism
- Try to understand different viewpoints
- Help others when they ask

### Be Inclusive

- Welcome people of all backgrounds
- Use inclusive language
- Be respectful of different opinions
- Focus on constructive feedback

### Follow the Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md).

## Reporting Issues

### Bug Reports

Use the [bug report template](.github/ISSUE_TEMPLATE/bug_report.yml) and include:

- Steps to reproduce
- Expected vs actual behavior
- Environment information
- Error messages or logs
- Screenshots if applicable

### Feature Requests

Use the [feature request template](.github/ISSUE_TEMPLATE/feature_request.yml) and include:

- Problem statement
- Proposed solution
- Use cases
- Mockups or examples
- Implementation ideas

## Submitting Pull Requests

### PR Checklist

- [ ] Feature branch is up-to-date with `develop`
- [ ] All tests pass
- [ ] Code follows style guidelines
- [ ] Documentation is updated
- [ ] Changes are tested
- [ ] PR description is clear and detailed

### PR Template

Use the [pull request template](.github/pull_request_template.md) and include:

- Summary of changes
- Type of change
- Testing instructions
- Related issues
- Screenshots if applicable

### Review Process

1. Submit PR to `develop` branch
2. CI/CD pipeline runs tests
3. Code review by maintainers
4. Address feedback and comments
5. Merge after approval

## Contributor Roles

### Contributors

- Community members who submit PRs
- Help with documentation and testing
- Participate in discussions
- Support other users

### Community Leaders

- Active community members
- Help moderate discussions
- Review PRs and issues
- Organize community events

### Maintainers

- Core development team
- Review and merge PRs
- Set technical direction
- Manage releases and roadmap

## Recognition

### Contributor Recognition

- Contributors are listed in `CONTRIBUTORS.md`
- Regular contributors may be invited to join the team
- Outstanding contributions may be recognized in release notes

### Community Awards

- Zenith Star Award: Outstanding community contributions
- Innovation Award: Innovative technical solutions
- Helping Hand Award: Exceptional community support
- Documentation Award: Outstanding documentation contributions

## Getting Help

- [Documentation](https://github.com/your-repo/zenith-daw/docs)
- [Community Discussions](https://github.com/your-repo/zenith-daw/discussions)
- [Discord Server](https://discord.gg/zenith-daw)
- [Support Email](support@zenithdaw.com)

## Additional Resources

- [Zenith DAW Wiki](https://github.com/your-repo/zenith-daw/wiki)
- [Video Tutorials](https://youtube.com/zenithdaw)
- [API Reference](https://github.com/your-repo/zenith-daw/docs/api)
- [Troubleshooting Guide](https://github.com/your-repo/zenith-daw/blob/main/docs/TROUBLESHOOTING.md)

---

Thank you for contributing to Zenith DAW! Your efforts help make it better for everyone in the community.