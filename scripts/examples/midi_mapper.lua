--[[
    midi_mapper.lua
    Example script: MIDI controller mapping utility

    Maps MIDI CC controllers to DAW parameters with custom curves
]]

log("MIDI Mapper Script Initialized")

-- ============================================================================
-- Configuration
-- ============================================================================

local midiMappings = {
    -- CC1 (Mod Wheel) -> Master Volume
    {
        cc = 1,
        parameter = "master_volume",
        min = -60,
        max = 0,
        curve = "linear"
    },

    -- CC7 (Volume) -> Track 1 Volume
    {
        cc = 7,
        parameter = "track_volume",
        trackId = 1,
        min = -60,
        max = 6,
        curve = "logarithmic"
    },

    -- CC10 (Pan) -> Track 1 Pan
    {
        cc = 10,
        parameter = "track_pan",
        trackId = 1,
        min = -1,
        max = 1,
        curve = "linear"
    },

    -- CC74 (Filter Cutoff) -> Custom parameter
    {
        cc = 74,
        parameter = "filter_cutoff",
        min = 20,
        max = 20000,
        curve = "exponential"
    }
}

-- ============================================================================
-- Curve Functions
-- ============================================================================

function applyCurve(value, curvetype, min, max)
    -- value is 0-1 from MIDI CC (0-127 -> 0-1)
    local mapped

    if curvetype == "linear" then
        mapped = value
    elseif curvetype == "logarithmic" then
        -- Logarithmic curve for volume
        mapped = math.log(1 + value * 9) / math.log(10)
    elseif curvetype == "exponential" then
        -- Exponential curve for frequency
        mapped = (math.exp(value * 5) - 1) / (math.exp(5) - 1)
    else
        mapped = value
    end

    return min + mapped * (max - min)
end

-- ============================================================================
-- MIDI Processing Function
-- ============================================================================

function processMidiCC(ccNumber, ccValue)
    -- Find mapping for this CC
    for _, mapping in ipairs(midiMappings) do
        if mapping.cc == ccNumber then
            -- Normalize MIDI value (0-127 -> 0-1)
            local normalizedValue = ccValue / 127.0

            -- Apply curve
            local mappedValue = applyCurve(
                normalizedValue,
                mapping.curve,
                mapping.min,
                mapping.max
            )

            -- Route to parameter
            if mapping.parameter == "master_volume" then
                daw.setMasterVolume(mappedValue)
                log(string.format("Master Volume: %.1fdB (CC%d=%d)",
                    mappedValue, ccNumber, ccValue))

            elseif mapping.parameter == "track_volume" then
                daw.setTrackVolume(mapping.trackId, mappedValue)
                log(string.format("Track %d Volume: %.1fdB (CC%d=%d)",
                    mapping.trackId, mappedValue, ccNumber, ccValue))

            elseif mapping.parameter == "track_pan" then
                daw.setTrackPan(mapping.trackId, mappedValue)
                log(string.format("Track %d Pan: %.2f (CC%d=%d)",
                    mapping.trackId, mappedValue, ccNumber, ccValue))
            end

            break
        end
    end
end

-- ============================================================================
-- Main Loop (Demo)
-- ============================================================================

log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
log("MIDI Mapper Active")
log("Mappings configured:")

for _, mapping in ipairs(midiMappings) do
    log(string.format("  CC%d -> %s [%.1f to %.1f] (%s curve)",
        mapping.cc,
        mapping.parameter,
        mapping.min,
        mapping.max,
        mapping.curve))
end

log("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

-- Demo: Simulate some MIDI CC input
log("\nDemo: Simulating MIDI CC input...")

processMidiCC(1, 64)   -- Mod wheel to 50%
processMidiCC(7, 100)  -- Volume CC to ~78%
processMidiCC(10, 64)  -- Pan to center
processMidiCC(74, 85)  -- Filter cutoff

log("\nMIDI Mapper ready. Connect your MIDI controller!")
