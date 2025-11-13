/**
 * @file ProjectPersistenceTests.cpp
 * @brief Tests for ProjectPersistence save/load functions
 */

#include <JuceHeader.h>
#include "../include/io/ProjectPersistence.h"
#include "../include/model/ProjectModel.h"

namespace zenith
{
    /**
     * @class ProjectPersistenceTests
     * @brief Unit tests for ProjectPersistence
     */
    class ProjectPersistenceTests : public juce::UnitTest
    {
    public:
        ProjectPersistenceTests()
            : juce::UnitTest("ProjectPersistence", "Model")
        {
        }

        void runTest() override
        {
            beginTest("Round-trip save and load");
            testRoundTrip();

            beginTest("Load from non-existent file");
            testLoadNonExistentFile();

            beginTest("Load from invalid XML file");
            testLoadInvalidXML();
        }

    private:
        /**
         * @brief Test round-trip save and load
         *
         * Creates a ProjectModel, saves it to a temp file, loads it back,
         * and verifies all data survived the round trip.
         */
        void testRoundTrip()
        {
            // Create a test project
            ProjectModel project;
            project.name = "Test Project";
            project.sampleRate = 48000.0;

            // Add Track 1
            TrackModel track1;
            track1.id = 1;
            track1.name = "Track 1";
            track1.gain = 0.8f;
            track1.pan = -0.5f;
            track1.muted = false;

            // Add Clip 1 to Track 1
            ClipModel clip1;
            clip1.id = 101;
            clip1.name = "Clip 1";
            clip1.file = juce::File("/path/to/audio1.wav");
            clip1.startSample = 0;
            clip1.lengthSamples = 48000;
            clip1.srcOffset = 0;
            clip1.gain = 1.0f;
            clip1.fadeInSamples = 1000;
            clip1.fadeOutSamples = 2000;
            clip1.muted = false;
            track1.clips.push_back(clip1);

            // Add Clip 2 to Track 1 (muted)
            ClipModel clip2;
            clip2.id = 102;
            clip2.name = "Clip 2";
            clip2.file = juce::File("/path/to/audio2.wav");
            clip2.startSample = 96000;
            clip2.lengthSamples = 24000;
            clip2.srcOffset = 1200;
            clip2.gain = 0.5f;
            clip2.fadeInSamples = 500;
            clip2.fadeOutSamples = 500;
            clip2.muted = true;
            track1.clips.push_back(clip2);

            project.tracks.push_back(track1);

            // Add Track 2 (muted)
            TrackModel track2;
            track2.id = 2;
            track2.name = "Track 2";
            track2.gain = 1.0f;
            track2.pan = 0.5f;
            track2.muted = true;
            project.tracks.push_back(track2);

            // Save to temp file
            juce::File tempFile = juce::File::createTempFile(".xml");
            auto saveResult = saveProjectToFile(project, tempFile);

            expect(saveResult.ok, "Save should succeed");
            expect(saveResult.errorMessage.isEmpty(), "Save should have no error message");
            expect(tempFile.existsAsFile(), "Temp file should exist after save");

            // Load from temp file
            auto loadResult = loadProjectFromFile(tempFile);

            expect(loadResult.ok, "Load should succeed");
            expect(loadResult.errorMessage.isEmpty(), "Load should have no error message");

            // Verify project properties
            expectEquals(loadResult.project.name, juce::String("Test Project"), "Project name");
            expectEquals(loadResult.project.sampleRate, 48000.0, "Project sample rate");
            expectEquals(static_cast<int>(loadResult.project.tracks.size()), 2, "Number of tracks");

            // Verify Track 1
            const auto& loadedTrack1 = loadResult.project.tracks[0];
            expectEquals(loadedTrack1.id, 1, "Track 1 ID");
            expectEquals(loadedTrack1.name, juce::String("Track 1"), "Track 1 name");
            expectEquals(loadedTrack1.gain, 0.8f, "Track 1 gain");
            expectEquals(loadedTrack1.pan, -0.5f, "Track 1 pan");
            expect(!loadedTrack1.muted, "Track 1 muted");
            expectEquals(static_cast<int>(loadedTrack1.clips.size()), 2, "Track 1 clip count");

            // Verify Clip 1
            const auto& loadedClip1 = loadedTrack1.clips[0];
            expectEquals(loadedClip1.id, 101, "Clip 1 ID");
            expectEquals(loadedClip1.name, juce::String("Clip 1"), "Clip 1 name");
            expectEquals(loadedClip1.file.getFullPathName(),
                         juce::String("/path/to/audio1.wav"), "Clip 1 file path");
            expectEquals(loadedClip1.startSample, (SamplePos)0, "Clip 1 startSample");
            expectEquals(loadedClip1.lengthSamples, (SamplePos)48000, "Clip 1 lengthSamples");
            expectEquals(loadedClip1.srcOffset, (SamplePos)0, "Clip 1 srcOffset");
            expectEquals(loadedClip1.gain, 1.0f, "Clip 1 gain");
            expectEquals(loadedClip1.fadeInSamples, 1000, "Clip 1 fadeInSamples");
            expectEquals(loadedClip1.fadeOutSamples, 2000, "Clip 1 fadeOutSamples");
            expect(!loadedClip1.muted, "Clip 1 muted");

            // Verify Clip 2 (muted)
            const auto& loadedClip2 = loadedTrack1.clips[1];
            expectEquals(loadedClip2.id, 102, "Clip 2 ID");
            expect(loadedClip2.muted, "Clip 2 should be muted");
            expectEquals(loadedClip2.gain, 0.5f, "Clip 2 gain");

            // Verify Track 2 (muted)
            const auto& loadedTrack2 = loadResult.project.tracks[1];
            expectEquals(loadedTrack2.id, 2, "Track 2 ID");
            expect(loadedTrack2.muted, "Track 2 should be muted");

            // Clean up
            tempFile.deleteFile();
        }

        /**
         * @brief Test loading from non-existent file
         */
        void testLoadNonExistentFile()
        {
            juce::File nonExistent = juce::File::getCurrentWorkingDirectory()
                .getChildFile("this-file-does-not-exist.xml");

            auto loadResult = loadProjectFromFile(nonExistent);

            expect(!loadResult.ok, "Load should fail for non-existent file");
            expect(loadResult.errorMessage.isNotEmpty(), "Should have error message");
        }

        /**
         * @brief Test loading from invalid XML file
         */
        void testLoadInvalidXML()
        {
            // Create a temp file with invalid XML
            juce::File tempFile = juce::File::createTempFile(".xml");

            {
                juce::FileOutputStream stream(tempFile);
                if (stream.openedOk())
                {
                    stream.writeText("This is not valid XML { [ < >", false, false, nullptr);
                }
            }

            auto loadResult = loadProjectFromFile(tempFile);

            expect(!loadResult.ok, "Load should fail for invalid XML");
            expect(loadResult.errorMessage.isNotEmpty(), "Should have error message");

            // Clean up
            tempFile.deleteFile();
        }
    };

    // Register the test
    static ProjectPersistenceTests projectPersistenceTests;
}
