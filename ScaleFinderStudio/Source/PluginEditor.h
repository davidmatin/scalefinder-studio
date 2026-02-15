#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AudioAnalyzer.h"

class PianoKeyboard : public juce::Component
{
public:
    PianoKeyboard (ScaleFinderProcessor& p);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void setHighlightedNotes (const std::set<int>& notes);
    void clearSelection();

private:
    int getNoteAtPosition (juce::Point<float> pos) const;
    int getMidiNoteForPitchClass (int pc) const;
    juce::Rectangle<float> getWhiteKeyRect (int index) const;
    juce::Rectangle<float> getBlackKeyRect (int index) const;

    ScaleFinderProcessor& processorRef;
    std::set<int> highlightedPitchClasses;
    int lastPlayedNote = -1;  // For stopping previous note on mouseUp

    // White key indices: C=0, D=1, E=2, F=3, G=4, A=5, B=6
    static constexpr int whiteNoteNumbers[7] = { 60, 62, 64, 65, 67, 69, 71 };
    static constexpr int blackNoteNumbers[5] = { 61, 63, 66, 68, 70 };
    // Black key positions (which white key gap they sit over): 0-1, 1-2, 3-4, 4-5, 5-6
    static constexpr int blackKeyPositions[5] = { 0, 1, 3, 4, 5 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoKeyboard)
};

class ScaleFinderEditor : public juce::AudioProcessorEditor,
                          public juce::FileDragAndDropTarget,
                          private juce::Timer
{
public:
    explicit ScaleFinderEditor (ScaleFinderProcessor&);
    ~ScaleFinderEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void updateUI();
    void onKeyButtonClicked (const juce::String& keyName);
    void rebuildKeyButtons (const KeyResult& result);
    void updateChordsDisplay();
    void populateKeyDropdown();

    ScaleFinderProcessor& processorRef;

    // ── UI components ─────────────────────────────────────────────────
    PianoKeyboard pianoKeyboard;
    juce::TextButton resetButton { "Reset" };
    juce::ComboBox keyDropdown;
    juce::Label titleLabel1, titleLabel2;

    std::vector<std::unique_ptr<juce::TextButton>> majorButtons;
    std::vector<std::unique_ptr<juce::TextButton>> minorButtons;

    // ── Audio file analysis ─────────────────────────────────────────────
    AudioAnalyzer audioAnalyzer;
    bool isDragOver = false;
    juce::String analysisStatusText;

    // ── Theme colours (matching web app) ──────────────────────────────
    const juce::Colour bgTop       { 0xff0f0a1a };
    const juce::Colour bgBottom    { 0xff1a1a2e };
    const juce::Colour accentColour{ 0xff6366f1 };
    const juce::Colour purpleText  { 0xff8b5cf6 };
    const juce::Colour textColour  { 0xffe4e4e7 };
    const juce::Colour mutedText   { 0xffa1a1aa };
    const juce::Colour cardColour  { 0xff27272a };
    const juce::Colour borderColour{ 0xff3f3f46 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleFinderEditor)
};
