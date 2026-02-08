/*
    This file is part of Zenith DAW
    Unit tests for TrackManager::createTrack
*/

#include <juce_core/juce_core.h>
// Include the correct TrackManager header from engine/core
#include "../../../../modules/zenith_core/engine/core/TrackManager.h"
// Include Track header from engine
#include "../../../../modules/zenith_core/engine/Track.h"

namespace zenith {
namespace tests {

class TrackManagerCreateTest : public juce::UnitTest {
public:
    TrackManagerCreateTest() : juce::UnitTest("TrackManager Creation", "TrackManagers") {}

    void runTest() override {
        // TrackManager constructor might require dependencies or be standalone.
        // Looking at Turn 21, it has a default constructor.
        TrackManager manager;

        beginTest("Create Audio Track");
        {
            auto id = manager.createTrack("Audio 1", "audio");
            expect(id.isNotEmpty(), "Track ID should not be empty");
            expect(id.startsWith("track_"), "Track ID should start with prefix");

            auto* track = manager.getTrackById(id);
            expect(track != nullptr, "Track should be retrievable by ID");
            if (track) {
                expectEquals(track->getName(), juce::String("Audio 1"));
                expect(track->getType() == Track::Type::Audio, "Track type should be Audio");
            }
        }

        beginTest("Create MIDI Track");
        {
            auto id = manager.createTrack("MIDI 1", "midi");
            expect(id.isNotEmpty(), "Track ID should not be empty");

            auto* track = manager.getTrackById(id);
            expect(track != nullptr, "Track should be retrievable by ID");
            if (track) {
                expectEquals(track->getName(), juce::String("MIDI 1"));
                expect(track->getType() == Track::Type::MIDI, "Track type should be MIDI");
            }
        }

        beginTest("Create Track Case Insensitive");
        {
            auto id = manager.createTrack("Audio CAPS", "AUDIO");
            expect(id.isNotEmpty(), "Track ID should not be empty");

            auto* track = manager.getTrackById(id);
            expect(track != nullptr);
            if (track) {
                expect(track->getType() == Track::Type::Audio);
            }
        }

        beginTest("Create Unknown Type");
        {
            auto id = manager.createTrack("Invalid", "invalid_type");
            expect(id.isEmpty(), "Track ID should be empty for invalid type");
            // Count tracks to ensure it wasn't added
            // We created 3 valid tracks before.
            expectEquals(manager.getNumTracks(), 3);
        }
    }
};

static TrackManagerCreateTest trackManagerCreateTest;

} // namespace tests
} // namespace zenith
