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

#include "TestUtils.h"
#include "../../../../modules/zenith_core/engine/core/TrackManager.h"
#include "../../../../modules/zenith_core/engine/Track.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

/**
 * @class TrackManagerTests
 * @brief Tests for TrackManager class functionality
 */
class TrackManagerTests : public juce::UnitTest {
public:
    TrackManagerTests() : juce::UnitTest("TrackManager", "TrackManagers") {}

    void runTest() override {
        TrackManager manager;

        beginTest("Create Audio Track");
        {
            juce::String name = "My Audio Track";
            juce::String id = manager.createTrack(name, "audio");

            expect(id.isNotEmpty(), "Track ID should not be empty");
            expect(id.startsWith("track_"), "Track ID should start with 'track_'");

            auto* track = manager.getTrackById(id);
            expect(track != nullptr, "Track should be retrievable by ID");
            if (track) {
                expectEquals(track->getName(), name);
                expect(track->getType() == Track::Type::Audio, "Track type should be Audio");
                expectEquals(track->getTypeString(), juce::String("Audio"));
            }
        }

        beginTest("Create MIDI Track");
        {
            juce::String name = "My MIDI Track";
            juce::String id = manager.createTrack(name, "midi");

            expect(id.isNotEmpty(), "Track ID should not be empty");

            auto* track = manager.getTrackById(id);
            expect(track != nullptr, "Track should be retrievable by ID");
            if (track) {
                expectEquals(track->getName(), name);
                expect(track->getType() == Track::Type::MIDI, "Track type should be MIDI");
                expectEquals(track->getTypeString(), juce::String("MIDI"));
            }
        }

        beginTest("Create Unknown Track");
        {
            juce::String name = "Invalid Track";
            juce::String id = manager.createTrack(name, "unknown_type");

            expect(id.isEmpty(), "Track ID should be empty for unknown type");
            expect(manager.getNumTracks() == 2, "Number of tracks should remain unchanged (2)");
        }

        beginTest("Track Management");
        {
             expect(manager.getNumTracks() == 2, "Should have 2 tracks");

             // Verify tracks are in the snapshot
             auto snapshot = manager.getTracksSnapshot();
             expect(snapshot.size() == 2);

             // Test removal
             manager.clearAllTracks();
             expect(manager.getNumTracks() == 0, "Should have 0 tracks after clear");
        }
    }
};

static TrackManagerTests trackManagerTests;

} // namespace tests
} // namespace zenith
