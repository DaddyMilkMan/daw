# Prompt Engineering Guide for AI Mastering

## The Art of Getting Good Mastering Decisions from Grok

### Core Principle
The quality of Grok's mastering decisions depends entirely on:
1. **What you tell it** (the prompt)
2. **What you measure** (audio features)
3. **How you parse its response** (JSON structure)

## Current Prompt Structure

### What We Send to Grok

```
ROLE: You are an expert mastering engineer

CONTEXT: Audio analysis data (peak, RMS, frequency balance, etc.)

CONSTRAINTS: Return JSON with specific parameter ranges

INTENT: User's description of desired sound
```

## Improving the Prompt

### Add More Context

```cpp
// In buildPrompt(), add:
juce::String genreContext = detectGenre(features);  // Analyze features to guess genre

prompt += R"(
ESTIMATED GENRE: )" + genreContext + R"(
GENRE CHARACTERISTICS:
)" + getGenreExpectations(genreContext);
```

### Include Dynamic Range Context

```cpp
float dr = features.peakDb - features.rmsDb;
juce::String drAssessment;

if (dr > 18.0f) drAssessment = "Very dynamic, preserve transients";
else if (dr > 12.0f) drAssessment = "Good dynamics, moderate compression acceptable";
else if (dr > 8.0f) drAssessment = "Already compressed, gentle processing only";
else drAssessment = "Heavily compressed, avoid further dynamics processing";

prompt += "DYNAMIC RANGE ASSESSMENT: " + drAssessment + "\n";
```

### Better Frequency Analysis

Instead of just percentages, add descriptive analysis:

```cpp
juce::String analyzeFreqBalance(const Features& features) {
    juce::String analysis = "";
    
    // Check for problematic areas
    if (features.bass > 0.25f) 
        analysis += "⚠️ Bass-heavy, may need taming. ";
    if (features.lowMid > 0.20f)
        analysis += "⚠️ Muddy low-mids, consider cutting. ";
    if (features.brilliance < 0.05f)
        analysis += "⚠️ Lacks air, boost high shelf. ";
    if (features.presence < 0.08f)
        analysis += "⚠️ Lacks clarity, boost presence. ";
        
    // Check for good qualities
    if (features.mid > 0.25f && features.mid < 0.40f)
        analysis += "✓ Good midrange balance. ";
    if (features.crestFactor > 3.0f)
        analysis += "✓ Healthy dynamics preserved. ";
        
    return analysis;
}
```

## Advanced Prompting Techniques

### 1. Few-Shot Learning

Add examples of good mastering decisions:

```cpp
juce::String fewShotExamples = R"(
EXAMPLE 1 - Thin, brittle electronic track:
Input: Bass 8%, Mid 35%, Brilliance 22%
Decision: +2.5dB low shelf, -1.2dB @ 3kHz, +1.8dB air
Reasoning: Add weight to thin lows, tame harsh presence, enhance air

EXAMPLE 2 - Muddy acoustic mix:
Input: Bass 28%, Low-mid 24%, Mid 18%
Decision: -1.5dB low shelf, -2.8dB @ 300Hz, +2.2dB presence
Reasoning: Control excessive bass, clear muddy low-mids, add clarity

Now analyze this track:
)";
```

### 2. Chain of Thought Prompting

Ask Grok to show its reasoning step-by-step:

```cpp
prompt += R"(
Analyze this track step-by-step:
1. What's the biggest tonal problem?
2. What's the target loudness vs current level?
3. How much dynamic processing is needed?
4. What EQ moves will help most?
5. What are the risks of over-processing?

Then provide your JSON parameters with reasoning.
)";
```

### 3. Constraining Bad Decisions

```cpp
prompt += R"(
CRITICAL RULES:
- If dynamic range < 8dB, use compression ratio < 2:1
- If brilliance < 5%, air boost must be > +1dB
- If bass > 25%, low shelf gain must be negative
- Never boost and cut in adjacent frequency bands
- Limiter ceiling should be -0.3dB for streaming, -0.1dB for CD
)";
```

## Genre-Specific Prompts

### Electronic Music
```cpp
if (genre == "electronic") {
    prompt += R"(
GENRE NOTES: Electronic music
- Expect heavy bass and sub content
- Compression can be aggressive (up to 4:1)
- High frequencies should sparkle
- Stereo width is important
- Target loudness: -8 to -10 LUFS (loud)
)";
}
```

### Acoustic/Classical
```cpp
if (genre == "acoustic") {
    prompt += R"(
GENRE NOTES: Acoustic/Classical
- Preserve natural dynamics (compression < 2:1)
- Subtle EQ moves only
- Maintain depth and space
- Target loudness: -16 to -18 LUFS (dynamic)
- Avoid limiting artifacts
)";
}
```

### Rock/Metal
```cpp
if (genre == "rock") {
    prompt += R"(
GENRE NOTES: Rock/Metal
- Expect aggressive sound
- Moderate-heavy compression (2:1 to 3:1)
- Presence band critical (2-5kHz)
- Tight low end needed
- Target loudness: -10 to -12 LUFS
)";
}
```

## Handling Grok's Response

### Make Response More Robust

```cpp
MasteringDecision parseGrokResponse(const juce::String& response) {
    // Try multiple parsing strategies
    
    // Strategy 1: Look for JSON block
    auto jsonStart = response.indexOf("{");
    auto jsonEnd = response.lastIndexOf("}");
    
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
        juce::String jsonStr = response.substring(jsonStart, jsonEnd + 1);
        auto decision = tryParseJSON(jsonStr);
        if (decision.valid) return decision;
    }
    
    // Strategy 2: Extract values with regex if JSON fails
    return extractParametersWithRegex(response);
}
```

### Validate AI Decisions

```cpp
bool validateDecision(const MasteringDecision& decision, const Features& features) {
    // Don't trust AI blindly - validate decisions make sense
    
    // If mix is already bright, don't boost highs more
    if (features.brilliance > 0.15f && decision.eq.airGain > 2.0f) {
        DBG("AI wants too much air boost, clamping to +1dB");
        decision.eq.airGain = 1.0f;
    }
    
    // If already heavily compressed, don't compress more
    float dr = features.peakDb - features.rmsDb;
    if (dr < 8.0f && decision.compression.ratio > 2.5f) {
        DBG("Mix already compressed, reducing ratio");
        decision.compression.ratio = 1.8f;
    }
    
    return true;
}
```

## Iterative Refinement

Allow users to refine AI decisions:

```cpp
struct RefinementRequest {
    juce::String feedback;  // "make it warmer", "less compression", etc.
    MasteringDecision previousDecision;
    Features audioFeatures;
};

MasteringDecision refineDecision(const RefinementRequest& request) {
    juce::String refinementPrompt = R"(
Previous mastering decision:
)" + previousDecisionToString(request.previousDecision) + R"(

User feedback: ")" + request.feedback + R"("

Adjust the parameters based on this feedback while maintaining overall balance.
)";
    
    return grokAI_.getMasteringDecision(
        request.audioFeatures,
        request.feedback,
        -14.0f
    );
}
```

## Testing Your Prompts

### Create Test Cases

```cpp
void testPromptQuality() {
    // Test case 1: Thin mix
    Features thinMix;
    thinMix.bass = 0.05f;
    thinMix.brilliance = 0.25f;  // Too bright
    
    auto decision = grokAI_.getMasteringDecision(thinMix, "add weight", -14.0f);
    jassert(decision.eq.lowShelfGain > 1.0f);  // Should boost bass
    jassert(decision.eq.airGain < 1.0f);       // Should tame brightness
    
    // Test case 2: Muddy mix
    Features muddyMix;
    muddyMix.lowMid = 0.30f;  // Too muddy
    
    auto decision2 = grokAI_.getMasteringDecision(muddyMix, "clarity", -14.0f);
    jassert(decision2.eq.midCutGain < -0.5f);  // Should cut mids
}
```

## Pro Tips

1. **Be specific about intent**: "warm and punchy" is better than "good"
2. **Include reference context**: "like deadmau5" tells Grok more than "electronic"
3. **Mention the destination**: "for Spotify streaming" vs "for vinyl cutting"
4. **Provide constraints**: "preserve dynamics" vs "maximize loudness"
5. **Ask for reasoning**: Always request explanation so you can learn

## Example: Perfect Prompt

```cpp
juce::String buildPerfectPrompt(...) {
    return R"(
ROLE: You are a Grammy-award winning mastering engineer with 30 years experience.

AUDIO ANALYSIS:
Peak: -2.1 dB, RMS: -16.8 dB, Dynamic Range: 14.7 dB
Frequency Balance: Bass-light (8%), Mid-forward (38%), Bright (18%)
Stereo: Wide (82% decorrelation)
Genre Detected: Electronic/House
Crest Factor: 4.2 (good dynamics preserved)

PROBLEMS DETECTED:
⚠️ Lacking sub-bass weight
⚠️ Slightly harsh at 3kHz
✓ Good dynamic range
✓ Nice stereo width

TARGET:
Loudness: -14 LUFS (Spotify/Apple Music streaming)
User Intent: "Warm, club-ready master with punchy kick and clear vocals"
Reference Vibe: "Like Fred Again or Disclosure"

CONSTRAINTS:
- Preserve dynamics (compression ratio < 3:1)
- Add weight without muddiness
- Tame harshness without losing clarity
- Leave -0.3dB headroom for streaming codecs

Provide mastering parameters in JSON format with detailed reasoning.
)";
}
```

## Result: Better Decisions

With proper prompting, Grok will:
- Make context-aware decisions specific to your audio
- Avoid rookie mistakes (over-compression, excessive EQ)
- Provide useful reasoning you can learn from
- Adapt to different genres and intents
- Suggest creative solutions you might not think of

The AI is only as good as the information you give it. Garbage in, garbage out. Quality features + quality prompts = quality masters.
