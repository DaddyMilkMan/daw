#include <gtest/gtest.h>
#include "../engine/TakeFolder.h"
#include "../engine/Clip.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

class TakeFolderTests : public juce::UnitTest {
public:
    TakeFolderTests() : juce::UnitTest("TakeFolder Logic", "AudioEngine") {}

    void runTest() override {
        beginTest("Flatten to Disk");
        {
            // Setup
            juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                     .getChildFile("zenith_take_test_" + juce::Uuid().toString());
            tempDir.createDirectory();

            TakeFolder folder;
            folder.setName("TestFolder");
            folder.setStartPosition(0);
            
            // Create a dummy take (silent, but valid)
            auto take1 = std::make_shared<Clip>();
            take1->setType(Clip::Type::Audio);
            take1->setStartPosition(0);
            take1->setLength(44100); // 1 sec
            
            // Create dummy audio file for the take so it can produce audio?
            // Actually, an empty clip produces silence, which is fine for "writing something".
            // Ideally we'd fill it with data, but Clip logic requires valid file/buffer.
            // Let's use internal buffer for the test take.
            juce::AudioBuffer<float> buffer(2, 44100);
            buffer.clear();
            // Fill with DC offset to verify we wrote data and not just silence
            for (int i=0; i<44100; ++i) buffer.setSample(0, i, 0.5f);
            take1->setAudioBuffer(buffer);
            
            folder.addTake(take1);
            
            // Comp region setup
            folder.setCompRegion(0, 44100, 0);

            // Execute Flatten
            auto flatClip = folder.flatten(44100.0, tempDir);

            // Verify
            expect(flatClip != nullptr, "Flatten returned null");
            if (flatClip) {
                juce::File writtenFile = flatClip->getAudioFile();
                expect(writtenFile.existsAsFile(), "Flattened file does not exist");
                expect(writtenFile.getSize() > 0, "Flattened file is empty");
                expect(flatClip->getLength() == 44100, "Flattened clip length mismatch");
            
                // Clean up file
                writtenFile.deleteFile();
            }

            // Teardown
            tempDir.deleteRecursively();
        }
    }
};

static TakeFolderTests takeFolderTests;

} // namespace tests
} // namespace zenith
