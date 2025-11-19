--[[
    automate_volume.lua
    Example script: Automate track volume with fade-in effect

    Usage: Load this script in Zenith DAW and run to create automated volume fade
]]

log("Starting volume automation script")

-- Configuration
local trackId = 1  -- Track to automate
local fadeStartDb = -60  -- Start volume (dB)
local fadeEndDb = 0  -- End volume (dB)
local fadeDuration = 5.0  -- Duration in seconds
local steps = 100  -- Number of automation points

-- Check if track exists
local numTracks = daw.getNumTracks()
if numTracks < trackId then
    log("Error: Track " .. trackId .. " does not exist")
    return
end

log("Creating volume fade-in on track " .. trackId)
log("Fade: " .. fadeStartDb .. "dB -> " .. fadeEndDb .. "dB over " .. fadeDuration .. " seconds")

-- Create fade-in automation
for step = 0, steps do
    local progress = step / steps

    -- Linear fade in dB (sounds exponential due to logarithmic perception)
    local currentVolume = fadeStartDb + (fadeEndDb - fadeStartDb) * progress

    -- Set track volume
    daw.setTrackVolume(trackId, currentVolume)

    -- Wait between steps (non-real-time, for demo purposes)
    sleep(fadeDuration / steps)

    if step % 10 == 0 then
        log(string.format("Fade progress: %d%% (%.1fdB)", progress * 100, currentVolume))
    end
end

log("Volume automation complete!")
