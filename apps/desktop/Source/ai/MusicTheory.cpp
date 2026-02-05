/*
  ==============================================================================

    MusicTheory.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of music theory utilities for AI-powered composition.

  ==============================================================================
*/

#include "MusicTheory.h"

namespace zenith {
namespace ai {

//==============================================================================
// Static member initialization
std::vector<ProgressionPattern> MusicTheory::progressions_;
bool MusicTheory::progressionsInitialized_ = false;

//==============================================================================
int NoteName::parse(const juce::String& noteName) {
    if (noteName.isEmpty()) return -1;
    
    juce::String note = noteName.toUpperCase().trim();
    int noteValue = -1;
    int charIndex = 0;
    
    // Parse note letter
    switch (note[0]) {
        case 'C': noteValue = 0; break;
        case 'D': noteValue = 2; break;
        case 'E': noteValue = 4; break;
        case 'F': noteValue = 5; break;
        case 'G': noteValue = 7; break;
        case 'A': noteValue = 9; break;
        case 'B': noteValue = 11; break;
        default: return -1;
    }
    charIndex++;
    
    // Parse accidentals
    while (charIndex < note.length() && (note[charIndex] == '#' || note[charIndex] == 'B')) {
        if (note[charIndex] == '#') noteValue++;
        else if (note[charIndex] == 'B') noteValue--;
        charIndex++;
    }
    
    // Parse octave
    int octave = 4; // Default octave
    if (charIndex < note.length()) {
        juce::String octaveStr = note.substring(charIndex);
        if (octaveStr.containsOnly("-0123456789")) {
            octave = octaveStr.getIntValue();
        }
    }
    
    return (octave + 1) * 12 + ((noteValue + 12) % 12);
}

//==============================================================================
std::vector<int> Chord::getMidiNotes(int octave) const {
    std::vector<int> notes;
    int rootMidi = (octave + 1) * 12 + root;
    
    auto intervals = getIntervalsForQuality(quality);
    for (int interval : intervals) {
        notes.push_back(rootMidi + interval);
    }
    
    // Add extensions
    for (int ext : extensions) {
        notes.push_back(rootMidi + ext);
    }
    
    // Handle bass note for slash chords
    if (bassNote >= 0 && bassNote != root) {
        int bassMidi = (octave) * 12 + bassNote; // Bass note one octave lower
        notes.insert(notes.begin(), bassMidi);
    }
    
    return notes;
}

juce::String Chord::getName(bool useFlats) const {
    juce::String name = useFlats ? NoteName::flatNames[static_cast<size_t>(root)] 
                                  : NoteName::names[static_cast<size_t>(root)];
    
    switch (quality) {
        case ChordQuality::Major:           break; // No suffix for major
        case ChordQuality::Minor:           name += "m"; break;
        case ChordQuality::Diminished:      name += "dim"; break;
        case ChordQuality::Augmented:       name += "aug"; break;
        case ChordQuality::Major7:          name += "maj7"; break;
        case ChordQuality::Minor7:          name += "m7"; break;
        case ChordQuality::Dominant7:       name += "7"; break;
        case ChordQuality::Diminished7:     name += "dim7"; break;
        case ChordQuality::HalfDiminished7: name += "m7b5"; break;
        case ChordQuality::MinorMajor7:     name += "mMaj7"; break;
        case ChordQuality::Augmented7:      name += "aug7"; break;
        case ChordQuality::Sus2:            name += "sus2"; break;
        case ChordQuality::Sus4:            name += "sus4"; break;
        case ChordQuality::Add9:            name += "add9"; break;
        case ChordQuality::Major9:          name += "maj9"; break;
        case ChordQuality::Minor9:          name += "m9"; break;
        case ChordQuality::Dominant9:       name += "9"; break;
        case ChordQuality::Power5:          name += "5"; break;
    }
    
    if (bassNote >= 0 && bassNote != root) {
        name += "/" + juce::String(useFlats ? NoteName::flatNames[static_cast<size_t>(bassNote)] 
                                             : NoteName::names[static_cast<size_t>(bassNote)]);
    }
    
    return name;
}

std::vector<int> Chord::getIntervalsForQuality(ChordQuality q) {
    switch (q) {
        case ChordQuality::Major:           return {0, 4, 7};
        case ChordQuality::Minor:           return {0, 3, 7};
        case ChordQuality::Diminished:      return {0, 3, 6};
        case ChordQuality::Augmented:       return {0, 4, 8};
        case ChordQuality::Major7:          return {0, 4, 7, 11};
        case ChordQuality::Minor7:          return {0, 3, 7, 10};
        case ChordQuality::Dominant7:       return {0, 4, 7, 10};
        case ChordQuality::Diminished7:     return {0, 3, 6, 9};
        case ChordQuality::HalfDiminished7: return {0, 3, 6, 10};
        case ChordQuality::MinorMajor7:     return {0, 3, 7, 11};
        case ChordQuality::Augmented7:      return {0, 4, 8, 10};
        case ChordQuality::Sus2:            return {0, 2, 7};
        case ChordQuality::Sus4:            return {0, 5, 7};
        case ChordQuality::Add9:            return {0, 4, 7, 14};
        case ChordQuality::Major9:          return {0, 4, 7, 11, 14};
        case ChordQuality::Minor9:          return {0, 3, 7, 10, 14};
        case ChordQuality::Dominant9:       return {0, 4, 7, 10, 14};
        case ChordQuality::Power5:          return {0, 7};
        default:                            return {0, 4, 7};
    }
}

//==============================================================================
Scale::Scale(int root, ScaleType type) 
    : root_(root % 12), type_(type), intervals_(getIntervals(type)) {}

std::vector<int> Scale::getNotesInRange(int lowNote, int highNote) const {
    std::vector<int> notes;
    
    for (int midi = lowNote; midi <= highNote; ++midi) {
        if (containsNote(midi)) {
            notes.push_back(midi);
        }
    }
    
    return notes;
}

bool Scale::containsNote(int midiNote) const {
    int noteClass = ((midiNote % 12) - root_ + 12) % 12;
    
    for (int interval : intervals_) {
        if (interval == noteClass) return true;
    }
    return false;
}

int Scale::getScaleDegree(int midiNote) const {
    int noteClass = ((midiNote % 12) - root_ + 12) % 12;
    
    for (size_t i = 0; i < intervals_.size(); ++i) {
        if (intervals_[i] == noteClass) {
            return static_cast<int>(i) + 1;
        }
    }
    return 0;
}

int Scale::getNoteAtDegree(int degree, int octave) const {
    if (degree < 1 || degree > static_cast<int>(intervals_.size())) return -1;
    return (octave + 1) * 12 + root_ + intervals_[static_cast<size_t>(degree - 1)];
}

Chord Scale::getDiatonicChord(int degree) const {
    if (degree < 1 || degree > static_cast<int>(intervals_.size())) {
        return Chord{root_, ChordQuality::Major};
    }
    
    Chord chord;
    chord.root = (root_ + intervals_[static_cast<size_t>(degree - 1)]) % 12;
    
    // Determine chord quality based on scale type and degree
    if (type_ == ScaleType::Major || type_ == ScaleType::Lydian || 
        type_ == ScaleType::Mixolydian) {
        // Major scale diatonic qualities: I, ii, iii, IV, V, vi, vii°
        static const std::array<ChordQuality, 7> majorQualities = {
            ChordQuality::Major, ChordQuality::Minor, ChordQuality::Minor,
            ChordQuality::Major, ChordQuality::Major, ChordQuality::Minor,
            ChordQuality::Diminished
        };
        if (degree <= 7) chord.quality = majorQualities[static_cast<size_t>(degree - 1)];
    } else if (type_ == ScaleType::NaturalMinor || type_ == ScaleType::Dorian || 
               type_ == ScaleType::Phrygian) {
        // Natural minor diatonic qualities: i, ii°, III, iv, v, VI, VII
        static const std::array<ChordQuality, 7> minorQualities = {
            ChordQuality::Minor, ChordQuality::Diminished, ChordQuality::Major,
            ChordQuality::Minor, ChordQuality::Minor, ChordQuality::Major,
            ChordQuality::Major
        };
        if (degree <= 7) chord.quality = minorQualities[static_cast<size_t>(degree - 1)];
    } else if (type_ == ScaleType::HarmonicMinor) {
        // Harmonic minor: i, ii°, III+, iv, V, VI, vii°
        static const std::array<ChordQuality, 7> harmonicMinorQualities = {
            ChordQuality::Minor, ChordQuality::Diminished, ChordQuality::Augmented,
            ChordQuality::Minor, ChordQuality::Major, ChordQuality::Major,
            ChordQuality::Diminished
        };
        if (degree <= 7) chord.quality = harmonicMinorQualities[static_cast<size_t>(degree - 1)];
    }
    
    return chord;
}

std::vector<Chord> Scale::getDiatonicTriads() const {
    std::vector<Chord> chords;
    int numDegrees = static_cast<int>(intervals_.size());
    if (numDegrees > 7) numDegrees = 7;
    
    for (int i = 1; i <= numDegrees; ++i) {
        chords.push_back(getDiatonicChord(i));
    }
    return chords;
}

std::vector<Chord> Scale::getDiatonicSeventhChords() const {
    std::vector<Chord> chords;
    int numDegrees = static_cast<int>(intervals_.size());
    if (numDegrees > 7) numDegrees = 7;
    
    for (int i = 1; i <= numDegrees; ++i) {
        Chord chord = getDiatonicChord(i);
        // Convert triads to 7th chords
        switch (chord.quality) {
            case ChordQuality::Major:
                chord.quality = (i == 1 || i == 4) ? ChordQuality::Major7 : ChordQuality::Dominant7;
                break;
            case ChordQuality::Minor:
                chord.quality = ChordQuality::Minor7;
                break;
            case ChordQuality::Diminished:
                chord.quality = ChordQuality::HalfDiminished7;
                break;
            default:
                break;
        }
        chords.push_back(chord);
    }
    return chords;
}

juce::String Scale::getName(bool useFlats) const {
    juce::String name = useFlats ? NoteName::flatNames[static_cast<size_t>(root_)] 
                                  : NoteName::names[static_cast<size_t>(root_)];
    
    switch (type_) {
        case ScaleType::Major:          name += " Major"; break;
        case ScaleType::NaturalMinor:   name += " Minor"; break;
        case ScaleType::HarmonicMinor:  name += " Harmonic Minor"; break;
        case ScaleType::MelodicMinor:   name += " Melodic Minor"; break;
        case ScaleType::Dorian:         name += " Dorian"; break;
        case ScaleType::Phrygian:       name += " Phrygian"; break;
        case ScaleType::Lydian:         name += " Lydian"; break;
        case ScaleType::Mixolydian:     name += " Mixolydian"; break;
        case ScaleType::Locrian:        name += " Locrian"; break;
        case ScaleType::MajorPentatonic: name += " Major Pentatonic"; break;
        case ScaleType::MinorPentatonic: name += " Minor Pentatonic"; break;
        case ScaleType::Blues:          name += " Blues"; break;
        case ScaleType::WholeTone:      name += " Whole Tone"; break;
        case ScaleType::Chromatic:      name += " Chromatic"; break;
    }
    
    return name;
}

std::vector<int> Scale::getIntervals(ScaleType type) {
    switch (type) {
        case ScaleType::Major:          return {0, 2, 4, 5, 7, 9, 11};
        case ScaleType::NaturalMinor:   return {0, 2, 3, 5, 7, 8, 10};
        case ScaleType::HarmonicMinor:  return {0, 2, 3, 5, 7, 8, 11};
        case ScaleType::MelodicMinor:   return {0, 2, 3, 5, 7, 9, 11};
        case ScaleType::Dorian:         return {0, 2, 3, 5, 7, 9, 10};
        case ScaleType::Phrygian:       return {0, 1, 3, 5, 7, 8, 10};
        case ScaleType::Lydian:         return {0, 2, 4, 6, 7, 9, 11};
        case ScaleType::Mixolydian:     return {0, 2, 4, 5, 7, 9, 10};
        case ScaleType::Locrian:        return {0, 1, 3, 5, 6, 8, 10};
        case ScaleType::MajorPentatonic: return {0, 2, 4, 7, 9};
        case ScaleType::MinorPentatonic: return {0, 3, 5, 7, 10};
        case ScaleType::Blues:          return {0, 3, 5, 6, 7, 10};
        case ScaleType::WholeTone:      return {0, 2, 4, 6, 8, 10};
        case ScaleType::Chromatic:      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        default:                        return {0, 2, 4, 5, 7, 9, 11};
    }
}

//==============================================================================
int MusicTheory::getInterval(int fromNote, int toNote) {
    return ((toNote - fromNote) % 12 + 12) % 12;
}

int MusicTheory::transpose(int midiNote, int semitones) {
    return midiNote + semitones;
}

int MusicTheory::getRelativeKey(int root, bool toMinor) {
    // Relative minor is 3 semitones below major, relative major is 3 above minor
    return toMinor ? (root + 9) % 12 : (root + 3) % 12;
}

int MusicTheory::getParallelKey(int root) {
    // Parallel keys share the same root
    return root;
}

int MusicTheory::quantizeToScale(int midiNote, const Scale& scale) {
    if (scale.containsNote(midiNote)) return midiNote;
    
    // Find nearest note in scale
    for (int offset = 1; offset <= 6; ++offset) {
        if (scale.containsNote(midiNote + offset)) return midiNote + offset;
        if (scale.containsNote(midiNote - offset)) return midiNote - offset;
    }
    
    return midiNote; // Fallback
}

std::vector<ProgressionPattern> MusicTheory::getProgressionsForGenre(const juce::String& genre) {
    initializeProgressions();
    
    std::vector<ProgressionPattern> result;
    juce::String lowerGenre = genre.toLowerCase();
    
    for (const auto& prog : progressions_) {
        if (prog.genre.toLowerCase().contains(lowerGenre) || 
            lowerGenre.contains(prog.genre.toLowerCase())) {
            result.push_back(prog);
        }
    }
    
    // If no genre match, return some universal progressions
    if (result.empty()) {
        for (const auto& prog : progressions_) {
            if (prog.genre.toLowerCase() == "universal" || 
                prog.genre.toLowerCase() == "pop") {
                result.push_back(prog);
            }
        }
    }
    
    return result;
}

std::vector<ProgressionPattern> MusicTheory::getAllProgressions() {
    initializeProgressions();
    return progressions_;
}

void MusicTheory::initializeProgressions() {
    if (progressionsInitialized_) return;
    progressionsInitialized_ = true;
    
    // Pop/Rock progressions
    progressions_.push_back({
        "Pop I-V-vi-IV", {1, 5, 6, 4},
        {ChordQuality::Major, ChordQuality::Major, ChordQuality::Minor, ChordQuality::Major},
        "Pop", 0.3f, "Uplifting, anthemic"
    });
    
    progressions_.push_back({
        "Pop vi-IV-I-V", {6, 4, 1, 5},
        {ChordQuality::Minor, ChordQuality::Major, ChordQuality::Major, ChordQuality::Major},
        "Pop", 0.4f, "Emotional, driving"
    });
    
    progressions_.push_back({
        "Rock I-IV-V", {1, 4, 5},
        {ChordQuality::Major, ChordQuality::Major, ChordQuality::Major},
        "Rock", 0.5f, "Classic, powerful"
    });
    
    progressions_.push_back({
        "50s I-vi-IV-V", {1, 6, 4, 5},
        {ChordQuality::Major, ChordQuality::Minor, ChordQuality::Major, ChordQuality::Major},
        "Rock", 0.3f, "Nostalgic, doo-wop"
    });
    
    // Jazz progressions
    progressions_.push_back({
        "Jazz ii-V-I", {2, 5, 1},
        {ChordQuality::Minor7, ChordQuality::Dominant7, ChordQuality::Major7},
        "Jazz", 0.4f, "Sophisticated, resolved"
    });
    
    progressions_.push_back({
        "Jazz I-vi-ii-V", {1, 6, 2, 5},
        {ChordQuality::Major7, ChordQuality::Minor7, ChordQuality::Minor7, ChordQuality::Dominant7},
        "Jazz", 0.3f, "Smooth, cyclical"
    });
    
    progressions_.push_back({
        "Jazz Rhythm Changes", {1, 6, 2, 5, 1, 6, 2, 5},
        {ChordQuality::Major7, ChordQuality::Minor7, ChordQuality::Minor7, ChordQuality::Dominant7,
         ChordQuality::Major7, ChordQuality::Minor7, ChordQuality::Minor7, ChordQuality::Dominant7},
        "Jazz", 0.4f, "Bebop, energetic"
    });
    
    // Blues progressions
    progressions_.push_back({
        "12-Bar Blues", {1, 1, 1, 1, 4, 4, 1, 1, 5, 4, 1, 5},
        {ChordQuality::Dominant7, ChordQuality::Dominant7, ChordQuality::Dominant7, ChordQuality::Dominant7,
         ChordQuality::Dominant7, ChordQuality::Dominant7, ChordQuality::Dominant7, ChordQuality::Dominant7,
         ChordQuality::Dominant7, ChordQuality::Dominant7, ChordQuality::Dominant7, ChordQuality::Dominant7},
        "Blues", 0.5f, "Soulful, traditional"
    });
    
    // EDM/Electronic progressions
    progressions_.push_back({
        "EDM i-VI-III-VII", {1, 6, 3, 7},
        {ChordQuality::Minor, ChordQuality::Major, ChordQuality::Major, ChordQuality::Major},
        "Electronic", 0.6f, "Epic, euphoric"
    });
    
    progressions_.push_back({
        "Trance i-VII-VI-VII", {1, 7, 6, 7},
        {ChordQuality::Minor, ChordQuality::Major, ChordQuality::Major, ChordQuality::Major},
        "Electronic", 0.7f, "Hypnotic, building"
    });
    
    // Hip-Hop/R&B
    progressions_.push_back({
        "R&B i-IV-VII-III", {1, 4, 7, 3},
        {ChordQuality::Minor7, ChordQuality::Major7, ChordQuality::Major7, ChordQuality::Major7},
        "Hip-Hop", 0.3f, "Smooth, laid-back"
    });
    
    progressions_.push_back({
        "Neo-Soul ii-V-I-IV", {2, 5, 1, 4},
        {ChordQuality::Minor9, ChordQuality::Dominant9, ChordQuality::Major9, ChordQuality::Major9},
        "R&B", 0.3f, "Lush, sophisticated"
    });
    
    // Minor key progressions
    progressions_.push_back({
        "Minor i-iv-V", {1, 4, 5},
        {ChordQuality::Minor, ChordQuality::Minor, ChordQuality::Major},
        "Universal", 0.5f, "Dark, resolving"
    });
    
    progressions_.push_back({
        "Andalusian i-VII-VI-V", {1, 7, 6, 5},
        {ChordQuality::Minor, ChordQuality::Major, ChordQuality::Major, ChordQuality::Major},
        "Flamenco", 0.6f, "Dramatic, Spanish"
    });
    
    // Ambient/Cinematic
    progressions_.push_back({
        "Cinematic I-III-IV-iv", {1, 3, 4, 4},
        {ChordQuality::Major, ChordQuality::Major, ChordQuality::Major, ChordQuality::Minor},
        "Cinematic", 0.4f, "Emotional, borrowed chord"
    });
    
    progressions_.push_back({
        "Ambient I-V/3-vi", {1, 5, 6},
        {ChordQuality::Major, ChordQuality::Major, ChordQuality::Minor},
        "Ambient", 0.2f, "Spacious, ethereal"
    });
}

std::vector<int> MusicTheory::parseRomanNumerals(const juce::String& notation) {
    std::vector<int> degrees;
    juce::StringArray tokens;
    tokens.addTokens(notation, "-", "");
    
    for (const auto& token : tokens) {
        juce::String upper = token.toUpperCase().trim();
        if (upper == "I" || upper == "i") degrees.push_back(1);
        else if (upper == "II" || upper == "ii") degrees.push_back(2);
        else if (upper == "III" || upper == "iii") degrees.push_back(3);
        else if (upper == "IV" || upper == "iv") degrees.push_back(4);
        else if (upper == "V" || upper == "v") degrees.push_back(5);
        else if (upper == "VI" || upper == "vi") degrees.push_back(6);
        else if (upper == "VII" || upper == "vii") degrees.push_back(7);
    }
    
    return degrees;
}

juce::String MusicTheory::toRomanNumerals(const std::vector<int>& degrees, 
                                           const std::vector<ChordQuality>& qualities) {
    static const std::array<const char*, 7> upperNumerals = {"I", "II", "III", "IV", "V", "VI", "VII"};
    static const std::array<const char*, 7> lowerNumerals = {"i", "ii", "iii", "iv", "v", "vi", "vii"};
    
    juce::String result;
    
    for (size_t i = 0; i < degrees.size(); ++i) {
        if (i > 0) result += "-";
        
        int degree = degrees[i];
        if (degree < 1 || degree > 7) continue;
        
        bool isMinor = (i < qualities.size()) && 
                       (qualities[i] == ChordQuality::Minor || 
                        qualities[i] == ChordQuality::Minor7 ||
                        qualities[i] == ChordQuality::Diminished);
        
        result += isMinor ? lowerNumerals[static_cast<size_t>(degree - 1)] 
                          : upperNumerals[static_cast<size_t>(degree - 1)];
        
        if (i < qualities.size() && qualities[i] == ChordQuality::Diminished) {
            result += juce::CharPointer_UTF8("\xC2\xB0"); // Degree symbol
        }
    }
    
    return result;
}

} // namespace ai
} // namespace zenith
