#pragma once
#include <JuceHeader.h>
#include <set>
#include <vector>
#include <map>

struct Pitch
{
    juce::String name;     // Internal: "C#"
    juce::String display;  // Display: "C#" or "E♭"
};

struct ChordInfo
{
    juce::String roman;    // "I", "ii", "iii", etc.
    juce::String name;     // "Dm", "G", "C°"
    juce::String root;     // "D", "G", "C"
    juce::String quality;  // "maj", "min", "dim"
};

struct KeyInfo
{
    juce::String name;           // "C# Major" (internal)
    juce::String displayName;    // "E♭ Major" (enharmonic-corrected)
    std::set<int> pitchClasses;  // {0, 2, 4, 5, 7, 9, 11}
    juce::String type;           // "Major" or "Minor"
    int root;                    // 0-11
};

struct KeyResult
{
    juce::String status;           // "all-visible", "all", "some", "none"
    std::vector<KeyInfo> keys;
};

class MusicTheory
{
public:
    static const Pitch PITCHES[12];
    static const int MAJOR_INTERVALS[7];
    static const int MINOR_INTERVALS[7];

    static std::vector<KeyInfo> allKeys();
    static KeyResult getPossibleKeys (const std::set<int>& selected);
    static juce::String getKeyDisplayName (const juce::String& keyName);
    static std::vector<ChordInfo> getChordProgressions (const juce::String& keyName);
    static juce::String getRelativeKey (const juce::String& keyName);
    static juce::String getNoteNameForKey (int pitchClass, const juce::String& keyName);

private:
    static bool usesSharpAccidentals (const juce::String& keyName);
    static bool usesFlatAccidentals (const juce::String& keyName);
    static std::set<int> buildScaleSet (int root, const int* intervals, int count);
};
