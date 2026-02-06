/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

namespace zenith {
namespace constants {
namespace strings {

// Track Types
static constexpr const char* kAudioTrackType = "audio";
static constexpr const char* kMidiTrackType = "midi";
static constexpr const char* kInstrumentTrackType = "instrument";
static constexpr const char* kBusTrackType = "bus";
static constexpr const char* kReturnTrackType = "return";
static constexpr const char* kMasterTrackType = "master";

// File Extensions
static constexpr const char* kProjectFileExtension = ".zenith";
static constexpr const char* kPresetFileExtension = ".zpreset";
static constexpr const char* kAudioWavExtension = ".wav";

// Properties (ProjectState)
static constexpr const char* kPropId = "id";
static constexpr const char* kPropName = "name";
static constexpr const char* kPropType = "type";
static constexpr const char* kPropMuted = "muted";
static constexpr const char* kPropSolo = "solo";
static constexpr const char* kPropArmed = "armed";
static constexpr const char* kPropVolume = "volume";
static constexpr const char* kPropPan = "pan";

// UI Strings
static constexpr const char* kUntitledProject = "Untitled Project";
static constexpr const char* kDefaultTrackName = "Track";

} // namespace strings
} // namespace constants
} // namespace zenith
