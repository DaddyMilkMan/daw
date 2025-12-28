#pragma once
#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @struct SafeAsync
 * @brief Helper for posting safe async callbacks to the message thread.
 */
struct SafeAsync {
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
