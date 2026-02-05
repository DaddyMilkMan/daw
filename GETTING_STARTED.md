# Getting Started with Zenith DAW Development

Welcome to the Zenith DAW project! We're excited to have you join our community of developers building a next-generation digital audio workstation. This guide will help you get up and running quickly, even if you're new to open source development.

## 🎯 Welcome! Why Contribute to Zenith DAW?

Zenith DAW is an ambitious open-source project that combines cutting-edge audio technology with AI-powered creative tools. By contributing, you'll:

- **Learn professional audio software development** with real-time audio processing
- **Work with modern C++20 and JUCE framework**
- **Contribute to AI-integrated music production tools**
- **Join a welcoming community** of audio engineers and developers
- **Build impressive portfolio projects** in digital audio technology

## 🚀 Quick Start - First-Time Setup (5-10 minutes)

### Prerequisites

Before you begin, make sure you have:

1. **Git** - [Download Git](https://git-scm.com/downloads)
2. **CMake 3.15+** - [Download CMake](https://cmake.org/download/)
3. **C++ Compiler**:
   - **Windows**: Visual Studio 2022 (with C++ workload)
   - **macOS**: Xcode Command Line Tools
   - **Linux**: GCC 9+ or Clang 9+
4. **Python 3.8+** - [Download Python](https://www.python.org/downloads/)
5. **JUCE Framework** - Included in the repository

### Step-by-Step Setup

#### 1. Fork and Clone the Repository

```bash
# Fork the repository on GitHub first (click the "Fork" button on the repo page)

# Clone your fork locally
git clone https://github.com/YOUR_USERNAME/zenith-daw.git
cd zenith-daw

# Add the upstream repository
git remote add upstream https://github.com/zenith-daw/zenith.git
```

#### 2. Install Dependencies

**Windows (Visual Studio 2022):**
```powershell
# Install Visual Studio 2022 with "Desktop development with C++" workload
# No additional system dependencies needed - everything is included!
```

**macOS:**
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install dependencies via Homebrew
brew install cmake ninja jack
```

**Linux (Ubuntu/Debian):**
```bash
# Install system dependencies
sudo apt update
sudo apt install build-essential cmake ninja-build libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev libfreetype6-dev libx11-dev libxinerama-dev libxext-dev libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev
```

#### 3. Build the Project

```bash
# Create build directory
mkdir build && cd build

# Configure CMake
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Build the project (this may take 5-15 minutes)
cmake --build . --config Release -j$(nproc)
```

#### 4. Run Your First Build

```bash
# Navigate back to project root
cd ..

# Run the application
# Windows: ./build/Release/Zenith.exe
# macOS: ./build/Zenith.app/Contents/MacOS/Zenith
# Linux: ./build/Zenith
```

## 📁 Understanding the Project Structure

Zenith DAW follows a modular architecture to keep the codebase organized and maintainable:

```
zenith-daw/
├── apps/desktop/Source/              # Desktop application shell
│   ├── ai_client/                   # C++ AI service client
│   ├── tests/                       # Unit and integration tests
│   └── platform/                    # OS-specific code
├── modules/                         # Reusable C++ modules
│   ├── zenith_core/                 # Audio engine, DSP, instruments
│   ├── zenith_ui/                   # UI components with Skia rendering
│   ├── zenith_network/              # Network services
│   └── zenith_commands/             # Command API for AI
├── docs/                            # Documentation
├── Content/                         # Presets, samples, resources
└── external/                        # Third-party dependencies (JUCE, etc.)
```

### Key Components to Know

- **Audio Engine** (`modules/zenith_core/engine/`): Real-time audio processing
- **UI Framework** (`modules/zenith_ui/`): GPU-accelerated interface using Skia
- **Instruments** (`modules/zenith_core/instruments/`): Built-in synths and samplers
- **AI Integration** (`apps/desktop/Source/ai_client/`): Creative assistance features

## 🔧 Your First Contribution - Making a Simple Change

Let's make your first contribution! We'll start with something simple: improving documentation.

### Step 1: Find a "Good First Issue"

Go to [Zenith DAW Issues](https://github.com/zenith-daw/zenith/issues) and look for:
- Labels: `good first issue`, `help wanted`, `beginner`
- Issues with "Documentation" or "UI" in the title
- Small bugs that need fixing

### Step 2: Create a Feature Branch

```bash
# Switch to main and get latest changes
git checkout main
git pull upstream main

# Create your feature branch
git checkout -b feature/improve-documentation
```

### Step 3: Make Your Changes

For this example, let's improve a comment in the code:

1. **Find a file** to edit (we'll use a test file for safety):
   ```bash
   # Open a test file in your editor
   code apps/desktop/Source/tests/ZenithTestsUberStrings.cpp
   ```

2. **Make a simple change** (like fixing a typo or adding a comment):
   ```cpp
   // Find a comment that could be clearer and improve it
   // Example: Change "// Test strings" to "// Test string functionality for audio buffers"
   ```

### Step 4: Test Your Changes

```bash
# Build and test your changes
cd build
cmake --build . --config Release
# Test that the application still runs
```

### Step 5: Commit and Push

```bash
# Add your changes
git add apps/desktop/Source/tests/ZenithTestsUberStrings.cpp

# Commit with a clear message
git commit -m "Improve test documentation for clarity

- Added more descriptive comment for test function
- Makes the purpose of the test clearer to new developers
- Addresses #42 (example issue number)"
```

### Step 6: Create a Pull Request

1. Push your branch to your fork:
   ```bash
   git push origin feature/improve-documentation
   ```

2. Go to the [Zenith DAW GitHub page](https://github.com/zenith-daw/zenith)
3. Click "Compare & pull request"
4. Fill in the PR template with:
   - Clear description of what you changed
   - Why it's helpful
   - Steps to test your changes

## 🎵 Understanding the Audio Architecture

Zenith DAW is a real-time audio application, which means special care is needed for performance and timing:

### Threading Model

- **Audio Thread**: Processes audio in real-time (no allocations, no blocking!)
- **Message Thread**: Handles UI and user interactions
- **Background Threads**: File loading, plugin scanning, AI processing

### Audio Thread Safety Rules

**NEVER in audio thread:**
- ✗ Allocate/deallocate memory
- ✗ Use locks/mutexes
- ✗ Make system calls
- ✗ Log messages (use FIFO instead)

**ALWAYS OK in audio thread:**
- ✓ Process audio samples
- ✓ Use std::atomic for communication
- ✓ Read pre-allocated buffers
- ✓ Use SIMD optimizations

## 🤖 AI Integration

Zenith DAW includes AI-powered creative assistance through the Grok API:

- **Command API**: Natural language control over DAW features
- **Creative Suggestions**: AI-powered arrangement and mixing ideas
- **Real-time Processing**: AI effects and enhancements

The AI integration is modular - you can work on it independently of the core audio engine.

## 🧪 Testing Your Code

We take code quality seriously. Make sure to:

### Unit Tests

```bash
# Build with tests enabled
cmake .. -DBUILD_TESTS=ON
cmake --build . -j$(nproc)

# Run all tests
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests

# Run specific tests
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests --gtest_filter="*Audio*"
```

### Manual Testing

Before submitting:
1. Build both Debug and Release configurations
2. Test your changes manually in the application
3. Ensure no new compiler warnings
4. Check that existing features still work

## 📚 Learning Resources

### Documentation
- **[Developer Guide](docs/DEVELOPER.md)** - Detailed technical documentation
- **[Architecture Overview](docs/ARCHITECTURE.md)** - System design and patterns
- **[Coding Conventions](docs/CODING_CONVENTIONS.md)** - Code style guide

### Tutorials
- **[JUCE Tutorials](https://docs.juce.com/master/tutorial_index.html)** - Learn JUCE framework
- **[Audio Programming Book](https://www.bookofjoe.com/2010/06/the_audio_programming_book.html)** - Free online resource
- **[Real-time Audio Programming](https://github.com/mbrucher/rt-audio)** - Practical guide

### Community
- **GitHub Discussions**: [Join discussions](https://github.com/zenith-daw/zenith/discussions)
- **Discord Server**: [Link in GitHub Discussions]
- **Stack Overflow**: Use tags `[juce]` and `[audio-programming]`

## 🛠️ Development Tools and IDEs

### Recommended IDEs

1. **Visual Studio 2022** (Windows) - Best C++ support with great debugging
2. **CLion** (Cross-platform) - Excellent CMake integration
3. **VS Code** (Lightweight) - Good for quick edits

### VS Code Setup Extensions

```bash
# Install VS Code extensions
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
code --install-extension ms-vscode.makefile-tools
```

## 🐛 Getting Help When Stuck

### Common Issues and Solutions

**Build Failures:**
```bash
# Clean build and reconfigure
rm -rf build
mkdir build && cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
```

**Audio Device Issues:**
- Close other audio applications first
- Try different buffer sizes in settings
- Check system audio permissions

**Git Issues:**
```bash
# Sync with upstream
git fetch upstream
git pull upstream main
git push origin your-branch
```

### Where to Get Help

1. **GitHub Issues**: Search existing issues first, then create new ones
2. **Discussions**: General questions and brainstorming
3. **Discord**: Real-time chat with developers
4. **Documentation**: Check existing docs before asking

## 🎯 Next Steps After Setup

### Simple First Contributions
- Fix typos in documentation
- Add missing comments to code
- Create simple unit tests
- Improve UI button spacing
- Add keyboard shortcut hints

### More Advanced Projects
- Implement new audio effects
- Create new instrument presets
- Add UI animations
- Improve AI response quality
- Optimize audio processing performance

### Long-Term Goals
- Become a maintainer
- Lead feature development
- Help with community management
- Present at audio developer conferences

## 🏆 Recognition and Community

### Contributor Recognition
- Contributors are listed in `CONTRIBUTORS.md`
- Regular contributors get special roles in the community
- Outstanding work is featured in release notes

### Community Awards
- **Zenith Star**: Outstanding contributions
- **Innovation Award**: Technical breakthroughs
- **Helping Hand**: Exceptional community support
- **Documentation Hero**: Improved docs and guides

## 📝 Checklist for Successful Contributions

- [ ] Fork and clone the repository
- [ ] Set up development environment
- [ ] Build successfully (Debug and Release)
- [ ] Run existing tests to ensure nothing is broken
- [ ] Find a "good first issue" or create a small improvement
- [ ] Create descriptive feature branch
- [ ] Make focused, intentional changes
- [ ] Test your changes thoroughly
- [ ] Write clear commit messages
- [ ] Submit pull request with detailed description
- [ ] Be responsive to feedback and review comments

## 🌟 Final Words

Welcome to the Zenith DAW community! We're glad to have you here. Remember:

- **No question is too basic** - we all started somewhere
- **Small contributions matter** - fixing a typo helps the project
- **Be kind and respectful** - we're here to help each other learn
- **Have fun!** - Building audio software is creative and rewarding

The journey of a thousand miles begins with a single step. Your first commit to Zenith DAW could be the start of an amazing adventure in audio software development!

Happy coding! 🎵🚀

---

**Need help?** Join our [Discord server](https://discord.gg/zenith-daw) or ask in [GitHub Discussions](https://github.com/zenith-daw/zenith/discussions)