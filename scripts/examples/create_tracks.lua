--[[
    create_tracks.lua
    Example script: Batch create mixing template with organized tracks

    Creates a complete mixing template with:
    - Drum tracks (kick, snare, hi-hat, etc.)
    - Instrument tracks (bass, synth, pad)
    - Vocal tracks (lead, harmony, FX)
    - Return/send tracks for reverb and delay
]]

log("Creating professional mixing template...")

-- ============================================================================
-- Track Template Configuration
-- ============================================================================

local trackTemplate = {
    drums = {
        { name = "Kick", type = "audio", volume = 0.0, pan = 0.0 },
        { name = "Snare", type = "audio", volume = -3.0, pan = 0.0 },
        { name = "Hi-Hat", type = "audio", volume = -6.0, pan = 0.1 },
        { name = "Toms", type = "audio", volume = -3.0, pan = 0.0 },
        { name = "Overhead L", type = "audio", volume = -6.0, pan = -0.7 },
        { name = "Overhead R", type = "audio", volume = -6.0, pan = 0.7 },
        { name = "Room", type = "audio", volume = -12.0, pan = 0.0 }
    },

    instruments = {
        { name = "Bass", type = "audio", volume = -3.0, pan = 0.0 },
        { name = "Lead Synth", type = "midi", volume = -6.0, pan = 0.0 },
        { name = "Pad", type = "midi", volume = -9.0, pan = 0.0 },
        { name = "Pluck", type = "midi", volume = -6.0, pan = 0.2 },
        { name = "Keys", type = "midi", volume = -6.0, pan = -0.2 }
    },

    vocals = {
        { name = "Lead Vocal", type = "audio", volume = -3.0, pan = 0.0 },
        { name = "Vocal Harmony", type = "audio", volume = -9.0, pan = 0.3 },
        { name = "Vocal FX", type = "audio", volume = -12.0, pan = -0.3 }
    },

    fx = {
        { name = "Reverb Send", type = "audio", volume = -12.0, pan = 0.0 },
        { name = "Delay Send", type = "audio", volume = -12.0, pan = 0.0 },
        { name = "Master Bus", type = "audio", volume = 0.0, pan = 0.0 }
    }
}

-- ============================================================================
-- Track Creation Function
-- ============================================================================

function createTrackGroup(groupName, tracks)
    log("Creating " .. groupName .. " group...")

    for i, trackInfo in ipairs(tracks) do
        local trackId

        if trackInfo.type == "audio" then
            trackId = daw.addAudioTrack(trackInfo.name)
        else
            trackId = daw.addMidiTrack(trackInfo.name)
        end

        if trackId then
            -- Set initial volume and pan
            daw.setTrackVolume(trackId, trackInfo.volume)
            daw.setTrackPan(trackId, trackInfo.pan)

            log("  ✓ " .. trackInfo.name .. " (ID: " .. trackId ..
                ", Vol: " .. trackInfo.volume .. "dB, Pan: " .. trackInfo.pan .. ")")
        else
            log("  ✗ Failed to create " .. trackInfo.name)
        end
    end
end

-- ============================================================================
-- Execute Template Creation
-- ============================================================================

log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
log("VEXEL DAW - Mixing Template Generator")
log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

createTrackGroup("Drums", trackTemplate.drums)
createTrackGroup("Instruments", trackTemplate.instruments)
createTrackGroup("Vocals", trackTemplate.vocals)
createTrackGroup("FX/Sends", trackTemplate.fx)

log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
log("Template created successfully!")
log("Total tracks: " .. daw.getNumTracks())
log("Project tempo: " .. daw.getTempo() .. " BPM")
log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
