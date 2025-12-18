#include <gtest/gtest.h>
#include "../engine/AudioConstants.h"
#include "../engine/MixerChannel.h"

TEST(AudioConstantsTest, ConstantsAreDefined) {
    EXPECT_EQ(zenith::constants::kDefaultSampleRate, 44100.0f);
    EXPECT_EQ(zenith::constants::kDefaultBufferSize, 512);
    EXPECT_EQ(zenith::constants::kNumEQBands, 4);
}

TEST(AudioConstantsTest, MixerChannelUsesConstants) {
    // This test ensures that MixerChannel compiles and links with the constants
    // If constants were missing, this would fail to link
    zenith::MixerChannel channel; 
    // Just instantiation is enough to verify linkage of default params
    SUCCEED();
}

TEST(AudioConstantsTest, CompressorDefaults) {
    EXPECT_NEAR(zenith::constants::kDefaultCompThresholdDb, -10.0f, 0.001f);
    EXPECT_NEAR(zenith::constants::kDefaultCompRatio, 2.0f, 0.001f);
}
