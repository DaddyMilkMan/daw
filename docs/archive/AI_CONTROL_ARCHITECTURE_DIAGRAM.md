# AI Control Architecture Diagram

## Current State: "The Remote Control"
The AI is external to the system, poking at surface-level parameters.

```mermaid
graph TD
    AI[AI Agent] -->|JSON Commands| API[CommandAPI]
    
    subgraph "Zenith DAW Engine"
        API -->|setTrackVolume| Mixer[Mixer Strip]
        API -->|setTrackPan| Mixer
        API -->|setPluginParam| VST[VST3 Plugin]
        API -->|split/move| Clip[Audio Clip]
        
        Mixer -.->|Hidden| Sends[Aux Sends]
        Mixer -.->|Hidden| EQ[4-Band EQ]
        Mixer -.->|Hidden| Comp[Compressor]
        
        VST -.->|Hidden| State[State Chunk]
        VST -.->|Hidden| Names[Param Names]
        
        Clip -.->|Hidden| Fades[Fades]
        Clip -.->|Hidden| Stretch[Time Stretch]
    end
    
    style AI fill:#f9f,stroke:#333,stroke-width:2px
    style API fill:#ff9,stroke:#333,stroke-width:2px
    style Sends fill:#ccc,stroke:#333,stroke-dasharray: 5 5
    style EQ fill:#ccc,stroke:#333,stroke-dasharray: 5 5
    style Comp fill:#ccc,stroke:#333,stroke-dasharray: 5 5
    style State fill:#ccc,stroke:#333,stroke-dasharray: 5 5
    style Names fill:#ccc,stroke:#333,stroke-dasharray: 5 5
    style Fades fill:#ccc,stroke:#333,stroke-dasharray: 5 5
    style Stretch fill:#ccc,stroke:#333,stroke-dasharray: 5 5
```

---

## Desired State: "The Brain Interface"
The AI has deep access to the signal graph, plugin states, and DSP pipeline.

```mermaid
graph TD
    AI[AI Agent] -->|JSON Commands| API[CommandAPI]
    
    subgraph "Zenith DAW Engine"
        API -->|Routing Graph API| Graph[Signal Routing Graph]
        
        Graph -->|Connect| Nodes[Audio Nodes]
        Nodes -->|Route| Bus[Aux Buses]
        Nodes -->|Sidechain| SC[Sidechain Inputs]
        
        API -->|State Management| Plugins[Plugin Host]
        Plugins -->|Base64 Chunks| VST[VST3 Plugin]
        Plugins -->|Named Params| VST
        
        API -->|DSP Control| Clips[Clip Engine]
        Clips -->|Curves| Fades[Crossfades]
        Clips -->|Rubberband| Stretch[Time/Pitch]
        
        API -->|Analysis| Feedback[Visual/Audio Feedback]
        Feedback -->|PNG| Waveform[Waveform View]
        Feedback -->|FFT| Spectrum[Spectrum Analyzer]
    end
    
    style AI fill:#f9f,stroke:#333,stroke-width:4px
    style API fill:#9f9,stroke:#333,stroke-width:4px
    style Graph fill:#bbf,stroke:#333,stroke-width:2px
    style Plugins fill:#bbf,stroke:#333,stroke-width:2px
    style Clips fill:#bbf,stroke:#333,stroke-width:2px
    style Feedback fill:#bbf,stroke:#333,stroke-width:2px
```

## Implementation Roadmap

```mermaid
gantt
    title AI Control Architecture Fix
    dateFormat  YYYY-MM-DD
    section Quick Wins
    Expose Sends/EQ/Comp       :active, a1, 2025-12-03, 1d
    section Phase 1: Routing
    Routing Graph Architecture :p1, after a1, 5d
    Bus Creation & Routing     :after p1, 2d
    section Phase 2: Plugins
    Param Name Resolution      :p2, after p1, 3d
    State Chunk Handling       :after p2, 2d
    section Phase 3: Clips
    Clip DSP Structures        :p3, after p2, 3d
    Fade/Stretch Processing    :after p3, 4d
    section Phase 4: Feedback
    UI State Capture           :p4, after p3, 2d
    Audio Analysis Export      :after p4, 3d
```
