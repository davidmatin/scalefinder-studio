#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AudioAnalyzer.h"
#include "MusicTheory.h"
#include <functional>

class PianoKeyboard : public juce::Component
{
public:
    PianoKeyboard (ScaleFinderProcessor& p);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setHighlightedNotes (const std::set<int>& notes);
    void setRootNote (int pitchClass);
    void clearSelection();

private:
    int getNoteAtPosition (juce::Point<float> pos) const;
    int getMidiNoteForPitchClass (int pc) const;
    juce::Rectangle<float> getWhiteKeyRect (int index) const;
    juce::Rectangle<float> getBlackKeyRect (int index) const;

    ScaleFinderProcessor& processorRef;
    std::set<int> highlightedPitchClasses;
    std::set<int> pressedPitchClasses;    // Currently held down (mouseDown → mouseUp)
    int rootPitchClass = -1;              // Root note of selected key (-1 = none)
    int lastPlayedNote = -1;              // For stopping previous note on mouseUp
    int hoveredPitchClass = -1;           // Key under mouse cursor (-1 = none)

    // White key indices: C=0, D=1, E=2, F=3, G=4, A=5, B=6
    static constexpr int whiteNoteNumbers[7] = { 60, 62, 64, 65, 67, 69, 71 };
    static constexpr int blackNoteNumbers[5] = { 61, 63, 66, 68, 70 };
    // Black key positions (which white key gap they sit over): 0-1, 1-2, 3-4, 4-5, 5-6
    static constexpr int blackKeyPositions[5] = { 0, 1, 3, 4, 5 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoKeyboard)
};

// ── Scale Results Panel (scrollable card list) ──────────────────────────
class ScaleResultsPanel : public juce::Component
{
public:
    ScaleResultsPanel();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setResults (const std::vector<KeyInfo>& keys, int selectedNoteCount);
    void setSelectedKey (const juce::String& keyName);

    std::function<void (const juce::String&)> onCardClicked;

private:
    int getCardAtPosition (juce::Point<float> pos) const;

    struct CardEntry {
        KeyInfo key;
        juce::String relativeKeyDisplay;
        bool isExactMatch;
    };
    std::vector<CardEntry> cards;
    juce::String selectedKeyName;

    struct CardRow {
        int leftIdx;    // Major card index, or -1
        int rightIdx;   // Minor card index, or -1
    };
    std::vector<CardRow> pairedRows;

    int hoveredCardIndex = -1;

    static constexpr float cardHeight = 40.0f;
    static constexpr float cardGap = 8.0f;
    static constexpr float colGap = 8.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleResultsPanel)
};

// ── Title bar LookAndFeel: close/minimize on left (Mac style) ──────────
class TitleBarLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void positionDocumentWindowButtons (juce::DocumentWindow&,
                                        int titleBarX, int titleBarY,
                                        int titleBarW, int titleBarH,
                                        juce::Button* minimiseButton,
                                        juce::Button* maximiseButton,
                                        juce::Button* closeButton,
                                        bool) override;
    juce::Button* createDocumentWindowButton (int buttonType) override;
};

// ── Options button LookAndFeel: draws slider-controls icon ─────────────

class OptionsIconLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour bgColour { 0xff0f0a1a };
    bool isActive = false;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// ── App-wide LookAndFeel for popup menus ────────────────────────────────
class AppMenuLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AppMenuLookAndFeel();
    void drawPopupMenuBackground (juce::Graphics&, int, int) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&,
                            bool, bool, bool, bool, bool,
                            const juce::String&, const juce::String&,
                            const juce::Drawable*, const juce::Colour*) override;
    int getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&) override;
};

// ── Keyboard toggle button LookAndFeel: draws 3x3 grid icon ────────────
class KeyboardIconLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour bgColour { 0xff0f0a1a };
    bool isEnabled = false;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// ── Dropdown-style button LookAndFeel ────────────────────────────────
class DropdownButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// ── Reset button LookAndFeel: transparent fill, thicker outline ──────
class ResetButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool, bool) override;
};

// ── Grid popup for key selection ──────────────────────────────────────
class KeyGridPopup : public juce::Component
{
public:
    KeyGridPopup();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setSelectedKey (const juce::String& keyName);
    std::function<void (const juce::String&)> onKeySelected;

private:
    struct KeyCell {
        juce::String internalName;
        juce::String displayName;
        juce::Rectangle<float> bounds;
    };
    std::vector<KeyCell> cells;
    juce::String selectedKeyName;
    int hoveredIndex = -1;

    int getCellAtPosition (juce::Point<float> pos) const;
    void buildCells();
};

class ScaleFinderEditor : public juce::AudioProcessorEditor,
                          public juce::FileDragAndDropTarget,
                          public juce::KeyListener,
                          private juce::Timer
{
public:
    explicit ScaleFinderEditor (ScaleFinderProcessor&);
    ~ScaleFinderEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void parentHierarchyChanged() override;

    // KeyListener
    bool keyPressed (const juce::KeyPress& key, juce::Component*) override;
    bool keyStateChanged (bool isKeyDown, juce::Component*) override;

private:
    void timerCallback() override;
    void updateUI();
    void onKeyButtonClicked (const juce::String& keyName);
    void updateChordsDisplay();
    void showKeyGridPopup();
    void dismissKeyGridPopup();

    ScaleFinderProcessor& processorRef;

    // ── UI components ─────────────────────────────────────────────────
    PianoKeyboard pianoKeyboard;
    juce::TextButton resetButton { "reset" };
    juce::TextButton keyDropdown { "select key..." };
    std::unique_ptr<KeyGridPopup> keyGridPopup;
    juce::Label titleLabel1, titleLabel2;

    // ── Scale results ─────────────────────────────────────────────────
    juce::Viewport resultsViewport;
    ScaleResultsPanel resultsPanel;

    // ── Audio file analysis ─────────────────────────────────────────────
    AudioAnalyzer audioAnalyzer;
    bool isDragOver = false;
    juce::String analysisStatusText;

    // ── Alternative key suggestions ──────────────────────────────────────
    std::vector<AudioAnalyzer::AlternativeKey> currentAlternatives;
    juce::TextButton altKeyButton1, altKeyButton2;
    void applyAlternativeKey (int index);

    // ── Text hierarchy (3 tiers) ───────────────────────────────────────
    const juce::Colour textPrimary   { 0xffE8EAF0 };  // ~93% lightness
    const juce::Colour textSecondary { 0xff8B90A0 };  // ~60% lightness
    const juce::Colour textMuted     { 0xff4A4F62 };  // ~35% lightness

    // ── Accent / brand ──────────────────────────────────────────────────
    const juce::Colour accentColour  { 0xff6366f1 };
    const juce::Colour accentPurple  { 0xff8b5cf6 };

    // ── Surfaces ────────────────────────────────────────────────────────
    const juce::Colour bgTop         { 0xff0f0a1a };
    const juce::Colour bgBottom      { 0xff1a1a2e };
    const juce::Colour cardBg        { 0xff1C2030 };
    const juce::Colour cardBgSelected{ 0xff252540 };
    const juce::Colour borderSubtle  { 0xff252836 };

    // ── Title bar styling ─────────────────────────────────────────────────
    TitleBarLookAndFeel titleBarLF;
    OptionsIconLookAndFeel optionsIconLF;
    KeyboardIconLookAndFeel keyboardIconLF;
    AppMenuLookAndFeel appMenuLF;
    juce::LookAndFeel* previousDefaultLF = nullptr;
    DropdownButtonLookAndFeel dropdownLF;
    ResetButtonLookAndFeel resetButtonLF;

    // ── Computer keyboard MIDI toggle ────────────────────────────────────
    juce::TextButton* keyboardToggleButton = nullptr;
    bool computerKeyboardEnabled = false;
    std::set<int> pressedKeyboardNotes;

    // ── Options button replacement (owns popup, properly positioned) ─────
    juce::TextButton* optionsButtonReplacement = nullptr;
    bool optionsMenuOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleFinderEditor)
};
