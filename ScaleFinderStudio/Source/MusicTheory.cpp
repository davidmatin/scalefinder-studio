#include "MusicTheory.h"

// ── Pitch data (mirrors JS PITCHES array) ─────────────────────────────────
const Pitch MusicTheory::PITCHES[12] = {
    { "C",  "C"  },
    { "C#", "C#" },   // Sharp side of circle
    { "D",  "D"  },
    { "D#", juce::String::fromUTF8 ("E\xe2\x99\xad") },   // E♭
    { "E",  "E"  },
    { "F",  "F"  },
    { "F#", "F#" },
    { "G",  "G"  },
    { "G#", juce::String::fromUTF8 ("A\xe2\x99\xad") },   // A♭
    { "A",  "A"  },
    { "A#", juce::String::fromUTF8 ("B\xe2\x99\xad") },   // B♭
    { "B",  "B"  }
};

// ── Scale intervals ───────────────────────────────────────────────────────
const int MusicTheory::MAJOR_INTERVALS[7] = { 0, 2, 4, 5, 7, 9, 11 };
const int MusicTheory::MINOR_INTERVALS[7] = { 0, 2, 3, 5, 7, 8, 10 };

// ── Enharmonic table (pitch-class → sharp/flat names) ─────────────────────
struct EnharmonicPair { juce::String sharp; juce::String flat; };

static const std::map<int, EnharmonicPair>& getEnharmonics()
{
    static const std::map<int, EnharmonicPair> table = {
        { 1,  { "C#", juce::String::fromUTF8 ("D\xe2\x99\xad") } },
        { 3,  { "D#", juce::String::fromUTF8 ("E\xe2\x99\xad") } },
        { 6,  { "F#", juce::String::fromUTF8 ("G\xe2\x99\xad") } },
        { 8,  { "G#", juce::String::fromUTF8 ("A\xe2\x99\xad") } },
        { 10, { "A#", juce::String::fromUTF8 ("B\xe2\x99\xad") } }
    };
    return table;
}

// ── Relative keys (bidirectional) ─────────────────────────────────────────
static const std::map<juce::String, juce::String>& getRelativeKeys()
{
    static const std::map<juce::String, juce::String> table = {
        { "C Major",  "A Minor"  }, { "A Minor",  "C Major"  },
        { "C# Major", "A# Minor" }, { "A# Minor", "C# Major" },
        { "D Major",  "B Minor"  }, { "B Minor",  "D Major"  },
        { "D# Major", "C Minor"  }, { "C Minor",  "D# Major" },
        { "E Major",  "C# Minor" }, { "C# Minor", "E Major"  },
        { "F Major",  "D Minor"  }, { "D Minor",  "F Major"  },
        { "F# Major", "D# Minor" }, { "D# Minor", "F# Major" },
        { "G Major",  "E Minor"  }, { "E Minor",  "G Major"  },
        { "G# Major", "F Minor"  }, { "F Minor",  "G# Major" },
        { "A Major",  "F# Minor" }, { "F# Minor", "A Major"  },
        { "A# Major", "G Minor"  }, { "G Minor",  "A# Major" },
        { "B Major",  "G# Minor" }, { "G# Minor", "B Major"  }
    };
    return table;
}

// ── Enharmonic key name conversions ───────────────────────────────────────
static const std::map<juce::String, juce::String>& getEnharmonicConversions()
{
    static const std::map<juce::String, juce::String> table = {
        { "D# Major", juce::String::fromUTF8 ("E\xe2\x99\xad Major") },
        { "G# Major", juce::String::fromUTF8 ("A\xe2\x99\xad Major") },
        { "A# Minor", juce::String::fromUTF8 ("B\xe2\x99\xad Minor") },
        { "D# Minor", juce::String::fromUTF8 ("E\xe2\x99\xad Minor") }
    };
    return table;
}

// ── Chord patterns ────────────────────────────────────────────────────────
struct ChordPattern { juce::String roman; juce::String quality; };

static const ChordPattern MAJOR_CHORD_PATTERN[7] = {
    { "I",      "maj" },
    { "ii",     "min" },
    { "iii",    "min" },
    { "IV",     "maj" },
    { "V",      "maj" },
    { "vi",     "min" },
    { juce::String::fromUTF8 ("vii\xc2\xb0"), "dim" }   // vii°
};

static const ChordPattern MINOR_CHORD_PATTERN[7] = {
    { "i",      "min" },
    { juce::String::fromUTF8 ("ii\xc2\xb0"),  "dim" },  // ii°
    { "III",    "maj" },
    { "iv",     "min" },
    { "v",      "min" },
    { "VI",     "maj" },
    { "VII",    "maj" }
};

// ── Root-name mapping (flat symbols / ASCII flat back to internal sharp) ──
static juce::String mapRootToInternal (const juce::String& rootName)
{
    // Flat symbol variants
    if (rootName == juce::String::fromUTF8 ("E\xe2\x99\xad") || rootName == "Eb") return "D#";
    if (rootName == juce::String::fromUTF8 ("A\xe2\x99\xad") || rootName == "Ab") return "G#";
    if (rootName == juce::String::fromUTF8 ("B\xe2\x99\xad") || rootName == "Bb") return "A#";
    if (rootName == juce::String::fromUTF8 ("D\xe2\x99\xad") || rootName == "Db") return "C#";
    if (rootName == juce::String::fromUTF8 ("G\xe2\x99\xad") || rootName == "Gb") return "F#";
    return rootName;
}

static int findPitchClassByName (const juce::String& name)
{
    for (int i = 0; i < 12; ++i)
        if (MusicTheory::PITCHES[i].name == name)
            return i;
    return -1;
}

// ── Public API ────────────────────────────────────────────────────────────

std::set<int> MusicTheory::buildScaleSet (int root, const int* intervals, int count)
{
    std::set<int> result;
    for (int i = 0; i < count; ++i)
        result.insert ((root + intervals[i]) % 12);
    return result;
}

std::vector<KeyInfo> MusicTheory::allKeys()
{
    std::vector<KeyInfo> out;
    out.reserve (24);

    struct ScaleType { const char* name; const int* intervals; };
    ScaleType types[2] = {
        { "Major", MAJOR_INTERVALS },
        { "Minor", MINOR_INTERVALS }
    };

    for (auto& st : types)
    {
        for (int r = 0; r < 12; ++r)
        {
            juce::String keyName = PITCHES[r].name + " " + st.name;
            out.push_back ({
                keyName,
                getKeyDisplayName (keyName),
                buildScaleSet (r, st.intervals, 7),
                st.name,
                r
            });
        }
    }
    return out;
}

KeyResult MusicTheory::getPossibleKeys (const std::set<int>& selected)
{
    if (selected.empty())
        return { "all-visible", allKeys() };

    if (selected.size() == 12)
        return { "all", {} };

    auto all = allKeys();
    std::vector<KeyInfo> matches;

    for (auto& key : all)
    {
        bool containsAll = true;
        for (int pc : selected)
        {
            if (key.pitchClasses.count (pc) == 0)
            {
                containsAll = false;
                break;
            }
        }
        if (containsAll)
            matches.push_back (key);
    }

    return { matches.empty() ? "none" : "some", matches };
}

juce::String MusicTheory::getKeyDisplayName (const juce::String& keyName)
{
    auto& conversions = getEnharmonicConversions();
    auto it = conversions.find (keyName);
    if (it != conversions.end())
        return it->second;

    // Split "C# Major" → root="C#", type="Major"
    auto space = keyName.indexOfChar (' ');
    if (space < 0) return keyName;

    juce::String rootName = keyName.substring (0, space);
    juce::String type     = keyName.substring (space + 1);

    int pc = findPitchClassByName (rootName);
    if (pc < 0) return keyName;

    return PITCHES[pc].display + " " + type;
}

bool MusicTheory::usesSharpAccidentals (const juce::String& keyName)
{
    static const juce::StringArray sharpKeys = {
        "C Major", "G Major", "D Major", "A Major", "E Major", "B Major", "F# Major", "C# Major",
        "A Minor", "E Minor", "B Minor", "F# Minor", "C# Minor", "G# Minor", "D# Minor", "A# Minor"
    };
    return sharpKeys.contains (keyName);
}

bool MusicTheory::usesFlatAccidentals (const juce::String& keyName)
{
    static const juce::StringArray flatKeys = {
        "F Major",
        "Bb Major", juce::String::fromUTF8 ("B\xe2\x99\xad Major"),
        "Eb Major", juce::String::fromUTF8 ("E\xe2\x99\xad Major"),
        "Ab Major", juce::String::fromUTF8 ("A\xe2\x99\xad Major"),
        "D Minor", "G Minor", "C Minor", "F Minor"
    };
    return flatKeys.contains (keyName);
}

juce::String MusicTheory::getNoteNameForKey (int pitchClass, const juce::String& keyName)
{
    // Natural notes — no accidental needed
    static const std::set<int> naturals = { 0, 2, 4, 5, 7, 9, 11 };
    if (naturals.count (pitchClass))
        return PITCHES[pitchClass].name;

    auto& enh = getEnharmonics();
    auto it = enh.find (pitchClass);
    if (it == enh.end())
        return PITCHES[pitchClass].display;

    if (usesSharpAccidentals (keyName))
        return it->second.sharp;
    if (usesFlatAccidentals (keyName))
        return it->second.flat;

    return PITCHES[pitchClass].display;
}

std::vector<ChordInfo> MusicTheory::getChordProgressions (const juce::String& keyName)
{
    // Normalize enharmonic key names first
    juce::String normalizedKeyName = keyName;
    {
        // Also handle the reverse: the internal key might use sharp notation
        // but getEnharmonicConversions maps internal → display
        static const std::map<juce::String, juce::String> internalEnharmonicMap = {
            { "D# Major", "Eb Major" },
            { "G# Major", "Ab Major" },
            { "A# Minor", "Bb Minor" },
            { "D# Minor", "Eb Minor" }
        };
        auto eit = internalEnharmonicMap.find (keyName);
        if (eit != internalEnharmonicMap.end())
            normalizedKeyName = eit->second;
    }

    auto space = normalizedKeyName.indexOfChar (' ');
    if (space < 0) return {};

    juce::String rootName = normalizedKeyName.substring (0, space);
    juce::String type     = normalizedKeyName.substring (space + 1);

    // Map flat name back to internal sharp name
    juce::String internalRoot = mapRootToInternal (rootName);

    int rootPc = findPitchClassByName (internalRoot);
    if (rootPc < 0) return {};

    const int* intervals = (type == "Major") ? MAJOR_INTERVALS : MINOR_INTERVALS;
    const ChordPattern* pattern = (type == "Major") ? MAJOR_CHORD_PATTERN : MINOR_CHORD_PATTERN;

    std::vector<ChordInfo> chords;
    chords.reserve (7);

    for (int i = 0; i < 7; ++i)
    {
        int pc = (rootPc + intervals[i]) % 12;
        juce::String chordRoot = getNoteNameForKey (pc, normalizedKeyName);

        juce::String suffix;
        if (pattern[i].quality == "min")       suffix = "m";
        else if (pattern[i].quality == "dim")  suffix = juce::String::fromUTF8 ("\xc2\xb0");  // °

        chords.push_back ({
            pattern[i].roman,
            chordRoot + suffix,
            chordRoot,
            pattern[i].quality
        });
    }

    return chords;
}

juce::String MusicTheory::getRelativeKey (const juce::String& keyName)
{
    auto& table = getRelativeKeys();
    auto it = table.find (keyName);
    if (it != table.end())
        return it->second;
    return {};
}
