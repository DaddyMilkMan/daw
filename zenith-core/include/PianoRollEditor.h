/**
 * @file PianoRollEditor.h
 * @brief Piano roll editor for MIDI clips
 *
 * Integration stub: Shows how PianoRollEditor integrates with ProjectState
 * to edit MIDI notes within a clip.
 *
 * Data Flow:
 * 1. Opened via ArrangerView double-click callback
 * 2. Reads MIDI notes from ProjectState clip (when MIDI note model is merged)
 * 3. User edits notes (add/move/delete)
 * 4. Updates ProjectState clip data
 * 5. Engine playback reflects changes
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"

//==============================================================================
/**
 * @class PianoRollEditor
 * @brief MIDI note editor window
 *
 * Displays MIDI notes in a piano roll view:
 * - Vertical axis: Pitch (piano keyboard on left)
 * - Horizontal axis: Time (beats)
 * - Notes represented as rectangles
 * - Click to add, drag to move, delete key to remove
 *
 * Integration points:
 * - Reads/writes MIDI note data from ProjectState clip
 * - When U3 MIDI note model is merged, will use clip's NOTES child nodes
 */
class PianoRollEditor : public juce::DocumentWindow
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state
     * @param trackId Track ID containing the clip
     * @param clipId Clip ID to edit
     */
    PianoRollEditor(ProjectState& projectState,
                    const juce::String& trackId,
                    const juce::String& clipId);

    /**
     * @brief Destructor
     */
    ~PianoRollEditor() override;

    //==========================================================================
    // DocumentWindow interface
    //==========================================================================

    void closeButtonPressed() override;

private:
    //==========================================================================
    // Content Component
    //==========================================================================

    class ContentComponent : public juce::Component
    {
    public:
        ContentComponent(ProjectState& projectState,
                         const juce::String& trackId,
                         const juce::String& clipId);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;

    private:
        ProjectState& projectState;
        juce::String trackId;
        juce::String clipId;

        // View settings
        float pixelsPerBeat = 60.0f;
        int noteHeight = 12;

        // Convert pitch to Y coordinate
        int pitchToY(int midiNote) const;
        int yToPitch(int y) const;
    };

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    juce::String trackId;
    juce::String clipId;

    std::unique_ptr<ContentComponent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};

