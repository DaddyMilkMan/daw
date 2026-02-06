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

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SampleEditorComponent.h"

namespace zenith {

class SampleEditAction : public juce::UndoableAction
{
public:
    SampleEditAction(SampleEditorComponent* owner,
                     const juce::AudioBuffer<float>& stateBefore,
                     const juce::AudioBuffer<float>& stateAfter,
                     const juce::String& actionName)
        : owner_(owner), name_(actionName)
    {
        // Clone buffers for persistence
        before_ = std::make_unique<juce::AudioBuffer<float>>(stateBefore);
        after_ = std::make_unique<juce::AudioBuffer<float>>(stateAfter);
    }

    bool perform() override
    {
        if (owner_ && after_)
        {
            owner_->setEditBuffer(*after_);
            return true;
        }
        return false;
    }

    bool undo() override
    {
        if (owner_ && before_)
        {
            owner_->setEditBuffer(*before_);
            return true;
        }
        return false;
    }

    juce::String getDescription() { return name_; }

    int getSizeInUnits() override
    {
        // Estimate size in bytes
        if (before_)
            return (int)(before_->getNumSamples() * before_->getNumChannels() * sizeof(float));
        return 1024;
    }


private:
    juce::Component::SafePointer<SampleEditorComponent> owner_;
    std::unique_ptr<juce::AudioBuffer<float>> before_;
    std::unique_ptr<juce::AudioBuffer<float>> after_;
    juce::String name_;
};

} // namespace zenith
