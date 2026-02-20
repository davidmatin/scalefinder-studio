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
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setResults (const std::vector<KeyInfo>& keys, int selectedNoteCount);
    void setSelectedKey (const juce::String& keyName);

    std::function<void (const juce::String&)> onCardClicked;

private:
    int getCardAtPosition (juce::Point<float> pos) const;
    void layoutCards (float availableWidth);

    struct CardEntry {
        KeyInfo key;
        juce::String chipText;                // Abbreviated ("C Maj") for chip display
        juce::String displayText;             // Full display name ("C Major")
        juce::Rectangle<float> bounds;
        bool isExactMatch;
        std::vector<ChordInfo> chords;        // Chord progression for this key
    };
    std::vector<CardEntry> cards;
    juce::String selectedKeyName;

    int hoveredCardIndex = -1;
    int majorCount = 0;
    int selectedCardIdx = -1;                 // Index of selected card (-1 = none)
    float relLabelY = 0.0f;                   // Y position of "relative minor/major" label
    float chipSectionY = 0.0f;                // Y where chips start (after selected card)
    bool isRelativePair = false;

    // Chip constants (browsing / unselected keys)
    static constexpr float chipHeight = 28.0f;
    static constexpr float chipGap = 4.0f;
    static constexpr float chipPadX = 8.0f;
    static constexpr float chipRadius = 14.0f;
    static constexpr float chipFontSize = 14.0f;
    static constexpr float separatorGap = 8.0f;

    // Card constants (selected key detail view)
    static constexpr float cardHeight = 52.0f;       // Selected key card height
    static constexpr float cardPrimaryH = 56.0f;     // Primary card in relative pair
    static constexpr float cardGap = 6.0f;           // Gap between card and chips
    static constexpr float cardRadius = 10.0f;       // Corner radius
    static constexpr float cardPadX = 14.0f;         // Horizontal padding inside card
    static constexpr float cardPadY = 8.0f;          // Vertical padding inside card

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
    void drawTooltip (juce::Graphics&, const juce::String&, int, int) override;
};

// ── Keyboard toggle button LookAndFeel: draws 3x3 grid icon ────────────
class KeyboardIconLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour bgColour { 0xff0f0a1a };
    bool isEnabled = true;
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

// ── Browse icon button LookAndFeel: outline + folder icon ─────────
class BrowseIconLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// ── Invisible button LookAndFeel (no-op paint) ──────────────────────
class InvisibleButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool, bool) override {}
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
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

// ── Theme: centralized color palette ─────────────────────────────────
struct Theme
{
    // Text hierarchy
    static juce::Colour textPrimary()   { return juce::Colour (0xffE8EAF0); }
    static juce::Colour textSecondary() { return juce::Colour (0xff8B90A0); }
    static juce::Colour textMuted()     { return juce::Colour (0xff4A4F62); }

    // Accent / brand
    static juce::Colour accent()        { return juce::Colour (0xff6366f1); }
    static juce::Colour accentPurple()  { return juce::Colour (0xff8b5cf6); }

    // Surfaces
    static juce::Colour bgTop()         { return juce::Colour (0xff0f0a1a); }
    static juce::Colour bgBottom()      { return juce::Colour (0xff1a1a2e); }
    static juce::Colour cardBg()        { return juce::Colour (0xff1C2030); }
    static juce::Colour cardSelected()  { return juce::Colour (0xff252540); }
    static juce::Colour borderSubtle()  { return juce::Colour (0xff252836); }
    static juce::Colour borderFaint()   { return juce::Colour (0x14ffffff); }
    static juce::Colour borderVFaint()  { return juce::Colour (0x0affffff); }
    static juce::Colour borderGhost()   { return juce::Colour (0x0fffffff); }
};

// ── Chords / status display (leaf component, paint only) ─────────────
class ChordsDisplay : public juce::Component
{
public:
    ChordsDisplay() = default;
    void paint (juce::Graphics&) override;

    void setChords (const std::vector<ChordInfo>& chords, const juce::String& keyName);
    void setStatus (const juce::String& status, const juce::String& resultStatus);
    void clear();

    // Layout values set by the editor before repaint
    int viewportBottom = 0;     // Y of results viewport bottom edge
    int altKeyY = 0;            // Y position of alt key button
    int altKeyH = 0;            // Height of alt key button
    bool altKeysVisible = false;
    bool emptyStateHovered = false;

private:
    std::vector<ChordInfo> chordList;
    juce::String keyName;
    juce::String statusText;
    juce::String resultStatus;   // "all-visible", "none", "all", "some"

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordsDisplay)
};

// ── Drag-drop overlay (leaf component, paint only) ───────────────────
class DragOverlay : public juce::Component
{
public:
    DragOverlay() = default;
    void paint (juce::Graphics&) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DragOverlay)
};

// ── Options popup (stays inside editor, like KeyGridPopup) ────────────
class OptionsPopup : public juce::Component
{
public:
    OptionsPopup();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    std::function<void (int)> onItemSelected;

private:
    struct MenuItem {
        juce::String text;
        int itemId;
        juce::Rectangle<float> bounds;
    };
    std::vector<MenuItem> items;
    int hoveredIndex = -1;

    int getItemAtPosition (juce::Point<float> pos) const;
    void buildItems();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OptionsPopup)
};

// ── Volume knob (neumorphic rotary control with mute toggle) ─────────
class VolumeKnob : public juce::Component,
                   public juce::SettableTooltipClient
{
public:
    VolumeKnob();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    float getValue() const { return volume; }
    void  setValue (float v) { volume = juce::jlimit (0.0f, 1.0f, v); repaint(); }
    bool  isMuted() const { return muted; }
    void  setMuted (bool m) { muted = m; repaint(); }

    std::function<void()> onValueChange;
    std::function<void()> onMuteToggle;

private:
    float volume = 0.75f;
    bool  muted = false;
    float dragStartValue = 0.0f;
    bool  clickedCenter = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VolumeKnob)
};

// ── Instrument selector button (neumorphic pill showing current name) ──
class InstrumentButton : public juce::Component,
                         public juce::SettableTooltipClient
{
public:
    InstrumentButton();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setPopupOpen (bool open) { popupOpen = open; repaint(); }
    bool isPopupOpen() const { return popupOpen; }
    void setSelectedIndex (int index) { selectedIndex = index; repaint(); }
    int  getSelectedIndex() const { return selectedIndex; }

    std::function<void()> onClick;

    static constexpr const char* shortNames[] = { "Synth", "Piano", "E-Piano", "Guitar" };

private:
    int  selectedIndex = 0;
    bool hovered = false;
    bool popupOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentButton)
};

// ── Instrument popup (dark dropdown with 4 instrument choices) ──────
class InstrumentPopup : public juce::Component
{
public:
    InstrumentPopup();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setSelectedIndex (int index) { selectedIndex = index; repaint(); }
    std::function<void (int)> onItemSelected;

private:
    struct MenuItem {
        juce::String text;
        int instrumentId;
        juce::Rectangle<float> bounds;
    };
    std::vector<MenuItem> items;
    int hoveredIndex = -1;
    int selectedIndex = 0;

    int getItemAtPosition (juce::Point<float> pos) const;
    void buildItems();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentPopup)
};

// ── Main editor ──────────────────────────────────────────────────────
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
    void showOptionsPopup();
    void dismissOptionsPopup();
    void showInstrumentPopup();
    void dismissInstrumentPopup();
    void openFileBrowser();

    ScaleFinderProcessor& processorRef;

    // ── UI components ─────────────────────────────────────────────────
    PianoKeyboard pianoKeyboard;
    VolumeKnob volumeKnob;
    InstrumentButton instrumentButton;
    juce::TextButton resetButton { "reset" };
    juce::TextButton keyDropdown { "select key..." };
    std::unique_ptr<KeyGridPopup> keyGridPopup;
    std::unique_ptr<OptionsPopup> optionsPopup;
    std::unique_ptr<InstrumentPopup> instrumentPopup;
    juce::Label titleLabel1, titleLabel2;

    // ── Scale results ─────────────────────────────────────────────────
    juce::Viewport resultsViewport;
    ScaleResultsPanel resultsPanel;

    // ── Chord / status display (extracted from paint) ─────────────────
    ChordsDisplay chordsDisplay;
    DragOverlay dragOverlay;
    juce::TooltipWindow tooltipWindow { this, 500 };

    // ── Audio file analysis ─────────────────────────────────────────────
    AudioAnalyzer audioAnalyzer;
    bool isDragOver = false;
    juce::String analysisStatusText;
    juce::TextButton browseButton { "" };
    juce::TextButton browseIconButton { "" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    // ── Alternative key suggestions ──────────────────────────────────────
    std::vector<AudioAnalyzer::AlternativeKey> currentAlternatives;
    juce::TextButton altKeyButton1, altKeyButton2;
    void applyAlternativeKey (int index);

    // ── Title bar styling ─────────────────────────────────────────────────
    TitleBarLookAndFeel titleBarLF;
    OptionsIconLookAndFeel optionsIconLF;
    KeyboardIconLookAndFeel keyboardIconLF;
    AppMenuLookAndFeel appMenuLF;
    juce::LookAndFeel* previousDefaultLF = nullptr;
    DropdownButtonLookAndFeel dropdownLF;
    ResetButtonLookAndFeel resetButtonLF;
    InvisibleButtonLookAndFeel invisibleButtonLF;
    BrowseIconLookAndFeel browseIconLF;

    // ── Computer keyboard MIDI toggle ────────────────────────────────────
    juce::TextButton* keyboardToggleButton = nullptr;
    bool computerKeyboardEnabled = true;
    std::set<int> pressedKeyboardNotes;

    // ── Options button replacement (owns popup, properly positioned) ─────
    juce::TextButton* optionsButtonReplacement = nullptr;
    bool optionsMenuOpen = false;

    // ── Cached title bar button pointers (avoids iterating children at 30Hz) ──
    juce::Button* cachedCloseBtn = nullptr;
    juce::Button* cachedMinimiseBtn = nullptr;
    bool titleBarButtonsCached = false;
    bool windowSizeConfigured = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleFinderEditor)
};
