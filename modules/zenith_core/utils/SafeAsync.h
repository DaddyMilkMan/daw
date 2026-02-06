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

namespace zenith {

/**
 * @struct SafeAsync
 * @brief Helper for posting safe async callbacks to the message thread.
 */


    /**
     * @brief Call a function asynchronously on the message thread, but only if the component (WeakReference) is still alive.
     * 
     * @tparam ComponentType The class type (must inherit juce::WeakReference<ComponentType>::Master)
     * @tparam Func The lambda or function to call
     * 
     * @param component Pointer to the component (this)
     * @param callback The function to execute: void(ComponentType*)
     */
    template <typename ComponentType, typename Func>
    static void call(ComponentType* component, Func&& callback) {
        juce::WeakReference<ComponentType> weakRef(component);
        juce::MessageManager::callAsync([weakRef, cb = std::forward<Func>(callback)]() {
            if (auto* comp = weakRef.get()) {
                cb(comp);
            }
        });
    }
};

} // namespace zenith
