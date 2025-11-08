# Zenith DAW

AI-Native Digital Audio Workstation with Wingman Integration

## Quick Start

### Development

```bash
# Install dependencies
npm install

# Run in development mode (starts Vite dev server + Electron)
npm run dev

# Or run separately:
npm run dev:vite    # Start Vite dev server
npm run dev:electron # Start Electron app
```

### Production Build

```bash
# Build frontend
npm run build

# Build application
npm run build:app

# Platform-specific builds
npm run dist:win    # Windows installer
npm run dist:mac    # macOS DMG
npm run dist:linux  # Linux AppImage/deb
```

## Project Structure

```
zenith-daw/
├── src/
│   ├── main/           # Electron main process
│   │   ├── index.js    # Main process entry
│   │   └── preload.js  # Preload script (IPC bridge)
│   └── renderer/       # React frontend
│       ├── components/ # UI components
│       ├── hooks/      # React hooks
│       ├── lib/        # Utilities
│       ├── store/      # State management
│       ├── types/      # TypeScript types
│       ├── App.tsx     # Main app component
│       └── main.tsx    # React entry point
├── public/             # Static assets
├── dist/               # Vite build output
└── dist-electron/      # Electron build output
```

## Features Implemented

### ✅ Core UI
- Tri-pane layout (Browser / Arrangement / Mixer)
- Transport controls (Play, Pause, Stop, Record)
- Tempo and time signature display
- Session View / Arrangement View toggle
- Track creation and management
- Basic mixer with volume/pan controls

### 🚧 In Progress
- Piano Roll editor
- Audio editor with comp lanes
- Wingman AI integration
- JUCE audio engine
- Plugin hosting
- MIDI generation

## Architecture

### Frontend (Renderer Process)
- **Framework**: React 18 + TypeScript
- **Build Tool**: Vite 5
- **Styling**: TailwindCSS + Radix UI
- **State**: Zustand (planned)

### Backend (Main Process)
- **Runtime**: Electron 28
- **Audio Engine**: JUCE (C++) - planned
- **AI Integration**: Wingman bridge - planned

### Communication
- **IPC**: Electron IPC (renderer ↔ main)
- **UDP**: Wingman protocol (ports 12000/12001) - planned
- **WebSocket**: Real-time updates (port 8123) - planned

## Development Roadmap

### Phase 1: Core UI (Weeks 1-4) ✅
- [x] Project setup
- [x] Tri-pane layout
- [x] Transport bar
- [x] Basic track management
- [ ] Piano roll (basic)
- [ ] Audio editor (basic)

### Phase 2: Audio Engine (Weeks 5-8)
- [ ] JUCE audio engine setup
- [ ] Audio I/O configuration
- [ ] MIDI playback
- [ ] Audio playback
- [ ] Plugin hosting (VST3)

### Phase 3: Wingman Integration (Weeks 9-12)
- [ ] UDP bridge
- [ ] AI command parsing
- [ ] MIDI generation
- [ ] Music prompt refinement
- [ ] Context-aware suggestions

### Phase 4: Advanced Features (Weeks 13-16)
- [ ] Session View (clip launcher)
- [ ] Automation
- [ ] Modular routing
- [ ] Audio analysis (Essentia.js)

### Phase 5: Polish & Beta (Weeks 17-20)
- [ ] Performance optimization
- [ ] UI/UX refinements
- [ ] Bug fixes
- [ ] Documentation
- [ ] Beta testing

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Space` | Play/Pause |
| `Enter` | Stop |
| `Tab` | Toggle Session ↔ Arrangement |
| `Cmd+T` | Create Track |
| `Cmd+B` | Toggle Browser |
| `Cmd+M` | Toggle Mixer |

## Tech Stack

- **Electron** - Cross-platform desktop framework
- **React** - UI framework
- **TypeScript** - Type safety
- **Vite** - Fast build tool
- **TailwindCSS** - Utility-first CSS
- **Radix UI** - Accessible component primitives
- **Lucide** - Icon library
- **Zustand** - State management
- **Framer Motion** - Animations (planned)

## License

MIT

## Author

DaddyMilkMan
