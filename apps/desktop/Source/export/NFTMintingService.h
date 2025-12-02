/*
  ==============================================================================

    NFTMintingService.h
    Created: 2025-11-29
    Author:  Zenith DAW - Feature Creep Team

    Handles cryptographic hashing and metadata generation for NFT minting.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>

namespace zenith {

class NFTMintingService {
public:
    struct MintingOptions {
        juce::String projectName;
        juce::String artistName;
        juce::String description;
        juce::File audioFile;
        juce::File coverArtFile;
        int editionSize;
        double royaltyPercentage;
    };

    struct MintingResult {
        bool success;
        juce::String tokenHash;
        juce::String metadataJson;
        juce::File packagePath;
        juce::String errorMessage;
    };

    NFTMintingService();
    ~NFTMintingService();

    /**
     * @brief Prepares an asset for minting by generating hashes and metadata.
     * This is the "Off-Chain" part of the minting process.
     */
    MintingResult prepareForMinting(const MintingOptions& options);

private:
    juce::String generateSHA256(const juce::File& file);
    juce::String generateMetadata(const MintingOptions& options, const juce::String& audioHash);
    juce::String signData(const juce::String& data);

    juce::RSAKey privateKey;
    juce::RSAKey publicKey;
};

} // namespace zenith
