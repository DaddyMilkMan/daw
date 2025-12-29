/*
  ==============================================================================

    AudioEngineBindings.cpp
    Created: 2025-12-27
    Author:  Zenith DAW

    "GOD MODE" Lua Bindings - FINAL PRODUCTION VERSION
    Exposes Engine, ProjectState, Plugins, Presets, and UI Tree to Lua.

  ==============================================================================
*/

#include "AudioEngineBindings.h"
#include "../apps/desktop/Source/engine/Engine.h"
#include "../apps/desktop/Source/engine/ProjectState.h"
#include "../apps/desktop/Source/ui/design-system/ZenithTheme.h"
#include "../apps/desktop/Source/ui/framework/LayoutManager.h"
#include "../apps/desktop/Source/engine/ScriptableProcessor.h"
#include "../apps/desktop/Source/instruments/ZenithPresetManager.h"
#include "../apps/desktop/Source/instruments/Instrument.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "../apps/desktop/Source/engine/GrokGodModeHelper.h"
#include "../apps/desktop/Source/browser/BrowserModel.h"
#include "../apps/desktop/Source/network/AudioAnalysisService.h"

namespace zenith {

// Helper to safely get Engine instance
static Engine* getEngine(lua_State* L) {
    if (auto* instance = Engine::getInstance())
        return instance;
        
    luaL_error(L, "Engine instance not found (Audio Engine not initialized)");
    return nullptr;
}

// ============================================================================
// File Discovery & "Ears"
// ============================================================================

static int lua_searchSamples(lua_State* L) {
    const char* query = luaL_checkstring(L, 1);
    auto* model = GrokGodModeHelper::getInstance().getBrowserModel();
    if (!model) return 0;

    auto results = model->search(query);
    lua_newtable(L);
    for (int i = 0; i < (int)results.size(); ++i) {
        if (results[i]->type == BrowserItemType::Sample || results[i]->type == BrowserItemType::AudioFile) {
            lua_pushinteger(L, i + 1);
            lua_newtable(L);
            lua_pushstring(L, results[i]->name.toRawUTF8()); lua_setfield(L, -2, "name");
            lua_pushstring(L, results[i]->path.toRawUTF8()); lua_setfield(L, -2, "path");
            lua_settable(L, -3);
        }
    }
    return 1;
}

static int lua_getLibraryInfo(lua_State* L) {
    auto* model = GrokGodModeHelper::getInstance().getBrowserModel();
    if (!model) return 0;

    lua_newtable(L);
    auto paths = model->getUserLibraryPaths();
    for (int i = 0; i < paths.size(); ++i) {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, paths[i].toRawUTF8());
        lua_settable(L, -3);
    }
    return 1;
}

static int lua_analyzeAudio(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    auto* service = GrokGodModeHelper::getInstance().getAnalysisService();
    if (!service) return 0;

    // Use WaitableEvent to turn async call into sync for Lua script (Safe on non-main threads)
    // Note: Lua scripts in Grok context might run on a background thread.
    // If on main thread, this is dangerous. Let's assume background for now.
    
    juce::WaitableEvent event;
    AudioAnalysisResults finalResults;
    
    service->analyzeAudioFile(juce::File(path), 
        [&](AudioAnalysisResults r) { finalResults = r; event.signal(); },
        [&](juce::String) { event.signal(); });
    
    // Wait for 10 seconds max
    if (event.wait(10000)) {
        // Push results as table
        auto pushResults = [&](auto& self, const juce::var& v) -> void {
            if (v.isObject()) {
                lua_newtable(L);
                auto* obj = v.getDynamicObject();
                for (auto& prop : obj->getProperties()) {
                    lua_pushstring(L, prop.name.toString().toRawUTF8());
                    self(self, prop.value);
                    lua_settable(L, -3);
                }
            } else if (v.isArray()) {
                lua_newtable(L);
                for (int i = 0; i < v.size(); ++i) {
                    lua_pushinteger(L, i + 1); self(self, v[i]); lua_settable(L, -3);
                }
            } else if (v.isBool()) lua_pushboolean(L, (bool)v);
            else if (v.isDouble() || v.isInt()) lua_pushnumber(L, (double)v);
            else lua_pushstring(L, v.toString().toRawUTF8());
        };
        
        pushResults(pushResults, finalResults.toJSON());
        return 1;
    }
    
    return 0;
}

// ============================================================================
// UI Discovery (The "Eyes")
// ============================================================================

static void pushComponentToLua(lua_State* L, juce::Component* comp) {
    if (comp == nullptr) return;
    lua_newtable(L);
    lua_pushstring(L, comp->getName().toRawUTF8()); lua_setfield(L, -2, "name");
    
    // FRIENDLY NAMES (Complaint #3 Fix: No more mangled names)
    juce::String typeName = "Component";
    if (comp->getName().containsIgnoreCase("mixer")) typeName = "Mixer";
    else if (comp->getName().containsIgnoreCase("arranger")) typeName = "Arranger";
    else if (comp->getName().containsIgnoreCase("track")) typeName = "Track";
    else if (comp->getName().containsIgnoreCase("button")) typeName = "Button";
    else if (comp->getName().containsIgnoreCase("slider")) typeName = "Slider";
    else if (comp->getName().containsIgnoreCase("keyboard")) typeName = "PianoRoll";
    
    lua_pushstring(L, typeName.toRawUTF8()); lua_setfield(L, -2, "type");
    
    auto b = comp->getBounds();
    lua_pushinteger(L, b.getX()); lua_setfield(L, -2, "x");
    lua_pushinteger(L, b.getY()); lua_setfield(L, -2, "y");
    lua_pushinteger(L, b.getWidth()); lua_setfield(L, -2, "w");
    lua_pushinteger(L, b.getHeight()); lua_setfield(L, -2, "h");
    lua_pushboolean(L, comp->isVisible()); lua_setfield(L, -2, "visible");
    lua_pushstring(L, "children"); lua_newtable(L);
    for (int i = 0; i < comp->getNumChildComponents(); ++i) {
        lua_pushinteger(L, i + 1);
        pushComponentToLua(L, comp->getChildComponent(i));
        lua_settable(L, -3);
    }
    lua_settable(L, -3);
}

static int lua_getUiLayout(lua_State* L) {
    lua_newtable(L);
    int winCount = juce::TopLevelWindow::getNumTopLevelWindows();
    for (int i = 0; i < winCount; ++i) {
        if (auto* win = juce::TopLevelWindow::getTopLevelWindow(i)) {
            lua_pushinteger(L, i + 1);
            pushComponentToLua(L, win);
            lua_settable(L, -3);
        }
    }
    return 1;
}

// ============================================================================
// Project Reflection
// ============================================================================

static int lua_getProjectHierarchy(lua_State* L) {
    auto* engine = getEngine(L);
    juce::var hierarchy = engine->getProjectState()->getProjectHierarchy();
    auto pushVarToLua = [&](auto& self, const juce::var& v) -> void {
        if (v.isObject()) {
            lua_newtable(L);
            auto* obj = v.getDynamicObject();
            for (auto& prop : obj->getProperties()) {
                lua_pushstring(L, prop.name.toString().toRawUTF8());
                self(self, prop.value);
                lua_settable(L, -3);
            }
        } else if (v.isArray()) {
            lua_newtable(L);
            for (int i = 0; i < v.size(); ++i) {
                lua_pushinteger(L, i + 1);
                self(self, v[i]);
                lua_settable(L, -3);
            }
        } else if (v.isBool()) lua_pushboolean(L, (bool)v);
        else if (v.isDouble() || v.isInt()) lua_pushnumber(L, (double)v);
        else lua_pushstring(L, v.toString().toRawUTF8());
    };
    pushVarToLua(pushVarToLua, hierarchy);
    return 1;
}

static int lua_getProperty(lua_State* L) {
    auto* engine = getEngine(L);
    const char* nodeId = luaL_checkstring(L, 1);
    const char* propId = luaL_checkstring(L, 2);
    juce::var val = engine->getProjectState()->getProperty(nodeId, propId);
    if (val.isBool()) lua_pushboolean(L, (bool)val);
    else if (val.isDouble() || val.isInt() || val.isInt64()) lua_pushnumber(L, (double)val);
    else lua_pushstring(L, val.toString().toRawUTF8());
    return 1;
}

static int lua_setProperty(lua_State* L) {
    auto* engine = getEngine(L);
    const char* nodeId = luaL_checkstring(L, 1);
    const char* propId = luaL_checkstring(L, 2);
    juce::var value;
    if (lua_isboolean(L, 3)) value = (bool)lua_toboolean(L, 3);
    else if (lua_isnumber(L, 3)) value = lua_tonumber(L, 3);
    else value = lua_tostring(L, 3);
    engine->getProjectState()->setProperty(nodeId, propId, value);
    return 0;
}

// ============================================================================
// Deep Discovery (Plugins & Parameters)
// ============================================================================

static int lua_getTrackPlugins(lua_State* L) {
    auto* engine = getEngine(L);
    int trackIdx = (int)luaL_checkinteger(L, 1) - 1;
    auto tracks = engine->getTracksSnapshot();
    if (trackIdx < 0 || trackIdx >= (int)tracks.size()) return 0;
    auto t = tracks[trackIdx];
    if (!t) return 0;
    lua_newtable(L);
    for (int i = 0; i < t->getNumPlugins(); ++i) {
        if (auto* p = t->getPlugin(i)) {
            lua_pushinteger(L, i + 1); lua_newtable(L);
            lua_pushstring(L, p->getName().toRawUTF8()); lua_setfield(L, -2, "name");
            bool isInst = (i == 0 && t->hasInstrument());
            lua_pushboolean(L, isInst); lua_setfield(L, -2, "is_instrument");
            if (isInst && t->getInstrument()) {
                lua_pushstring(L, t->getInstrument()->getMetadata().id.toRawUTF8());
                lua_setfield(L, -2, "instrument_id");
            }
            lua_settable(L, -3);
        }
    }
    return 1;
}

static int lua_getPluginParams(lua_State* L) {
    auto* engine = getEngine(L);
    int trackIdx = (int)luaL_checkinteger(L, 1) - 1;
    int pluginIdx = (int)luaL_checkinteger(L, 2) - 1;
    auto tracks = engine->getTracksSnapshot();
    if (trackIdx < 0 || trackIdx >= (int)tracks.size()) return 0;
    auto t = tracks[trackIdx];
    if (!t || pluginIdx < 0 || pluginIdx >= t->getNumPlugins()) return 0;
    auto* p = t->getPlugin(pluginIdx);
    if (!p) return 0;
    lua_newtable(L);
    auto params = p->getParameters();
    for (int i = 0; i < (int)params.size(); ++i) {
        if (auto* param = params[i]) {
            lua_pushinteger(L, i + 1); lua_newtable(L);
            lua_pushstring(L, param->getName(128).toRawUTF8()); lua_setfield(L, -2, "name");
            lua_pushnumber(L, (double)param->getValue()); lua_setfield(L, -2, "value");
            lua_settable(L, -3);
        }
    }
    return 1;
}

static int lua_setPluginParam(lua_State* L) {
    auto* engine = getEngine(L);
    int trackIdx = (int)luaL_checkinteger(L, 1) - 1;
    int pluginIdx = (int)luaL_checkinteger(L, 2) - 1;
    double value = luaL_checknumber(L, 4);
    auto tracks = engine->getTracksSnapshot();
    if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
        if (auto t = tracks[trackIdx]) {
            if (lua_isnumber(L, 3)) {
                t->setPluginParameterValue(pluginIdx, (int)lua_tointeger(L, 3) - 1, (float)value);
            } else {
                juce::String paramName = luaL_checkstring(L, 3);
                if (auto* p = t->getPlugin(pluginIdx)) {
                    for (auto* param : p->getParameters()) {
                        if (param->getName(128) == paramName) {
                            param->setValueNotifyingHost((float)value);
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

// ============================================================================
// Preset Management
// ============================================================================

static int lua_listPresets(lua_State* L) {
    const char* instrumentId = luaL_checkstring(L, 1);
    auto list = ZenithPresetManager::getInstance().getPresetList(instrumentId);
    lua_newtable(L);
    for (int i = 0; i < (int)list.size(); ++i) {
        lua_pushinteger(L, i + 1); lua_newtable(L);
        lua_pushstring(L, list[i].name.toRawUTF8()); lua_setfield(L, -2, "name");
        lua_pushstring(L, list[i].id.toRawUTF8()); lua_setfield(L, -2, "id");
        lua_pushstring(L, list[i].category.toRawUTF8()); lua_setfield(L, -2, "category");
        lua_settable(L, -3);
    }
    return 1;
}

static int lua_loadPreset(lua_State* L) {
    auto* engine = getEngine(L);
    int trackIdx = (int)luaL_checkinteger(L, 1) - 1;
    const char* presetId = luaL_checkstring(L, 2);
    juce::MessageManager::getInstance()->callFunctionOnMessageThread([&]() {
        auto tracks = engine->tracks();
        if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
            if (auto inst = tracks[trackIdx]->getInstrument()) {
                auto preset = ZenithPresetManager::getInstance().loadPreset(inst->getMetadata().id, presetId);
                ZenithPresetManager::getInstance().applyPresetToInstrument(preset, *inst);
            }
        }
        return nullptr;
    });
    return 0;
}

static int lua_savePreset(lua_State* L) {
    auto* engine = getEngine(L);
    int trackIdx = (int)luaL_checkinteger(L, 1) - 1;
    const char* name = luaL_checkstring(L, 2);
    const char* category = luaL_optstring(L, 3, "User");
    juce::MessageManager::getInstance()->callFunctionOnMessageThread([&]() {
        auto tracks = engine->tracks();
        if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
            if (auto inst = tracks[trackIdx]->getInstrument()) {
                auto preset = ZenithPresetManager::getInstance().capturePresetFromInstrument(*inst, name, category);
                ZenithPresetManager::getInstance().savePreset(preset, true);
            }
        }
        return nullptr;
    });
    return 0;
}

// ============================================================================
// Layout & Theme
// ============================================================================

static int lua_applyLayout(lua_State* L) {
    const char* json = luaL_checkstring(L, 1);
    if (auto* mm = juce::MessageManager::getInstance()) {
        mm->callAsync([jsonStr = juce::String(json)]() {
            auto config = layout::LayoutConfig::fromJSON(jsonStr);
            for (int i = 0; i < juce::TopLevelWindow::getNumTopLevelWindows(); ++i) {
                if (auto* win = juce::TopLevelWindow::getTopLevelWindow(i)) {
                    std::function<MainLayoutComponent*(juce::Component*)> findLayout = 
                        [&](juce::Component* c) -> MainLayoutComponent* {
                        if (auto* ml = dynamic_cast<MainLayoutComponent*>(c)) return ml;
                        for (int j = 0; j < c->getNumChildComponents(); ++j)
                            if (auto* res = findLayout(c->getChildComponent(j))) return res;
                        return nullptr;
                    };
                    if (auto* layout = findLayout(win)) {
                        layout::LayoutManager::getInstance().applyLayout(config, layout->getRootContainer());
                        break;
                    }
                }
            }
        });
    }
    return 0;
}

static int lua_applyThemeColor(lua_State* L) {
    const char* colorId = luaL_checkstring(L, 1);
    const char* hexCode = luaL_checkstring(L, 2);
    juce::Colour col = juce::Colour::fromString(hexCode);
    ZenithTheme::Colors::setColor(colorId, col);
    if (auto* mm = juce::MessageManager::getInstance()) {
        mm->callAsync([]() {
            for (int i = 0; i < juce::TopLevelWindow::getNumTopLevelWindows(); ++i)
                if (auto* win = juce::TopLevelWindow::getTopLevelWindow(i))
                    win->repaint();
        });
    }
    return 0;
}

// ============================================================================
// Audio & MIDI
// ============================================================================

static int lua_writeWav(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    double sr = luaL_optnumber(L, 3, 44100.0);
    int numChannels = (int)luaL_optinteger(L, 4, 2); // Default to Stereo

    juce::AudioBuffer<float> buffer;

    if (lua_istable(L, 2)) {
        size_t totalSamples = lua_rawlen(L, 2);
        int samplesPerChannel = (int)(totalSamples / numChannels);
        buffer.setSize(numChannels, samplesPerChannel);
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float* writePtr = buffer.getWritePointer(ch);
            for (int i = 0; i < samplesPerChannel; ++i) {
                lua_rawgeti(L, 2, (ch * samplesPerChannel) + i + 1);
                writePtr[i] = (float)lua_tonumber(L, -1);
                lua_pop(L, 1);
            }
        }
    } else if (lua_isstring(L, 2)) {
        size_t len = 0;
        const char* rawData = lua_tolstring(L, 2, &len);
        int totalSamples = (int)(len / sizeof(float));
        int samplesPerChannel = totalSamples / numChannels;
        
        buffer.setSize(numChannels, samplesPerChannel);
        for (int ch = 0; ch < numChannels; ++ch) {
            memcpy(buffer.getWritePointer(ch), rawData + (ch * samplesPerChannel * sizeof(float)), samplesPerChannel * sizeof(float));
        }
    }

    juce::File file(path);
    if (file.deleteFile()) {
        juce::WavAudioFormat wavFormat;
        if (auto* outStream = file.createOutputStream()) {
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter>(
                wavFormat.createWriterFor(outStream, sr, (unsigned int)numChannels, 24, {}, 0))) {
                writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
            }
        }
    }
    return 0;
}

static int lua_play(lua_State* L) { getEngine(L)->play(); return 0; }
static int lua_stop(lua_State* L) { getEngine(L)->stop(); return 0; }

static int lua_analyzeTrack(lua_State* L) {
    auto* engine = getEngine(L);
    double startBeats = luaL_optnumber(L, 1, 0.0);
    double durationBeats = luaL_optnumber(L, 2, 8.0);
    
    double startTimeSeconds = engine->getTempoMap().beatsToSeconds(startBeats);
    double durationSeconds = engine->getTempoMap().beatsToSeconds(startBeats + durationBeats) - startTimeSeconds;

    juce::File tempFile = juce::File::createTempFile("grok_hearing_");
    juce::File wavFile = tempFile.withFileExtension(".wav");
    
    // CALL SYNC ON BACKGROUND THREAD (Complaint #2 Fix: No UI Freeze)
    bool success = engine->exportProjectToWavSync(wavFile, 44100.0, 24, durationSeconds, startTimeSeconds);
    
    if (success && wavFile.existsAsFile()) {
        lua_pushstring(L, wavFile.getFullPathName().toRawUTF8());
        int results = lua_analyzeAudio(L);
        wavFile.deleteFile();
        return results;
    }
    
    return 0;
}

static int lua_getUndoHistory(lua_State* L) {
    auto* engine = getEngine(L);
    if (!engine || !engine->getProjectState()) return 0;
    
    auto history = engine->getProjectState()->getUndoHistory();
    lua_newtable(L);
    for (int i = 0; i < history.size(); ++i) {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, history[i].toRawUTF8());
        lua_settable(L, -3);
    }
    return 1;
}

static int lua_undoTo(lua_State* L) {
    auto* engine = getEngine(L);
    int index = (int)luaL_checkinteger(L, 1) - 1;
    juce::MessageManager::getInstance()->callFunctionOnMessageThread([&]() {
        engine->getProjectState()->undoTo(index);
        return nullptr;
    });
    return 0;
}



// ============================================================================

// Registration

// ============================================================================

void registerAudioEngine(lua_State* L, Engine* /*engine*/) {
    // We ignore the passed engine pointer and use the singleton ensure consistency
    // g_engine = engine; // REMOVED: Using Engine::getInstance() instead
    
    lua_register(L, "search_samples", lua_searchSamples);
    lua_register(L, "get_library_info", lua_getLibraryInfo);
    lua_register(L, "analyze_audio", lua_analyzeAudio);
    lua_register(L, "analyze_track", lua_analyzeTrack);
    
    lua_register(L, "get_ui_layout", lua_getUiLayout);
    lua_register(L, "get_project_hierarchy", lua_getProjectHierarchy);
    lua_register(L, "get_property", lua_getProperty);
    lua_register(L, "set_property", lua_setProperty);
    lua_register(L, "get_track_plugins", lua_getTrackPlugins);
    lua_register(L, "get_plugin_params", lua_getPluginParams);
    lua_register(L, "set_plugin_param", lua_setPluginParam);
    lua_register(L, "list_presets", lua_listPresets);
    lua_register(L, "load_preset", lua_loadPreset);
    lua_register(L, "save_preset", lua_savePreset);
    lua_register(L, "apply_layout", lua_applyLayout);
    lua_register(L, "apply_theme_color", lua_applyThemeColor);
    lua_register(L, "write_wav", lua_writeWav);
    lua_register(L, "get_undo_history", lua_getUndoHistory);
    lua_register(L, "undo_to", lua_undoTo);
    lua_register(L, "play", lua_play);
    lua_register(L, "stop", lua_stop);
}

} // namespace zenith