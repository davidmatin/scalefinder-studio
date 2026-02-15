#include "PluginEditor.h"

static const char* whiteKeyNames[7] = { "C", "D", "E", "F", "G", "A", "B" };
static const char* blackKeyNames[5] = { "C#", "E\xe2\x99\xad", "F#", "A\xe2\x99\xad", "B\xe2\x99\xad" };

static const char* MAJOR_DEGREE_NAMES[7] = {
    "Tonic", "Supertonic", "Mediant", "Subdominant", "Dominant", "Submediant", "Leading\nTone"
};
static const char* MINOR_DEGREE_NAMES[7] = {
    "Tonic", "Supertonic", "Mediant", "Subdominant", "Dominant", "Submediant", "Subtonic"
};

// ═══════════════════════════════════════════════════════════════════════════
// PianoKeyboard
// ═══════════════════════════════════════════════════════════════════════════

constexpr int PianoKeyboard::whiteNoteNumbers[7];
constexpr int PianoKeyboard::blackNoteNumbers[5];
constexpr int PianoKeyboard::blackKeyPositions[5];

PianoKeyboard::PianoKeyboard (ScaleFinderProcessor& p) : processorRef (p)
{
    setInterceptsMouseClicks (true, false);
}

juce::Rectangle<float> PianoKeyboard::getWhiteKeyRect (int index) const
{
    auto bounds = getLocalBounds().toFloat();
    float keyW = bounds.getWidth() / 7.0f;
    float gap = 2.0f;
    return { bounds.getX() + index * keyW + gap / 2.0f, bounds.getY(),
             keyW - gap, bounds.getHeight() };
}

juce::Rectangle<float> PianoKeyboard::getBlackKeyRect (int index) const
{
    auto bounds = getLocalBounds().toFloat();
    float whiteKeyW = bounds.getWidth() / 7.0f;
    float blackW = whiteKeyW * 0.58f;
    float blackH = bounds.getHeight() * 0.62f;
    float x = bounds.getX() + (blackKeyPositions[index] + 1) * whiteKeyW - blackW / 2.0f;
    return { x, bounds.getY(), blackW, blackH };
}

int PianoKeyboard::getMidiNoteForPitchClass (int pc) const
{
    for (int i = 0; i < 7; ++i)
        if (whiteNoteNumbers[i] % 12 == pc)
            return whiteNoteNumbers[i];
    for (int i = 0; i < 5; ++i)
        if (blackNoteNumbers[i] % 12 == pc)
            return blackNoteNumbers[i];
    return 60 + pc;
}

void PianoKeyboard::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Piano container with rounded corners and dark background
    g.setColour (juce::Colour (0xff1a1a2e));
    g.fillRoundedRectangle (bounds, 10.0f);

    // Draw white keys
    for (int i = 0; i < 7; ++i)
    {
        auto r = getWhiteKeyRect (i);
        auto inner = r.reduced (0, 1).withTrimmedTop (1);

        int pc = whiteNoteNumbers[i] % 12;
        bool highlighted = highlightedPitchClasses.count (pc) > 0;

        // White key body
        g.setColour (highlighted ? juce::Colour (0xffc4b5fd) : juce::Colour (0xfff5f5f5));
        g.fillRoundedRectangle (inner, 6.0f);

        // Subtle shadow at top
        g.setColour (juce::Colour (0x15000000));
        g.fillRect (inner.withHeight (3.0f));

        // Key label at bottom
        g.setColour (highlighted ? juce::Colour (0xff6d28d9) : juce::Colour (0xff71717a));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (whiteKeyNames[i], inner.removeFromBottom (22.0f), juce::Justification::centred);
    }

    // Draw black keys (on top)
    for (int i = 0; i < 5; ++i)
    {
        auto r = getBlackKeyRect (i);
        int pc = blackNoteNumbers[i] % 12;
        bool highlighted = highlightedPitchClasses.count (pc) > 0;

        // Shadow
        g.setColour (juce::Colour (0x40000000));
        g.fillRoundedRectangle (r.translated (1.0f, 1.0f), 5.0f);

        // Black key body
        g.setColour (highlighted ? juce::Colour (0xff7c3aed) : juce::Colour (0xff2a2a2a));
        g.fillRoundedRectangle (r, 5.0f);

        // Subtle highlight gradient at top
        g.setColour (juce::Colour (0x18ffffff));
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.3f), 5.0f);

        // Label
        g.setColour (highlighted ? juce::Colour (0xffe4e4e7) : juce::Colour (0xffa1a1aa));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::String::fromUTF8 (blackKeyNames[i]),
                    r.withTrimmedTop (r.getHeight() * 0.55f),
                    juce::Justification::centred);
    }
}

int PianoKeyboard::getNoteAtPosition (juce::Point<float> pos) const
{
    // Check black keys first (they're on top)
    for (int i = 0; i < 5; ++i)
        if (getBlackKeyRect (i).contains (pos))
            return blackNoteNumbers[i];

    for (int i = 0; i < 7; ++i)
        if (getWhiteKeyRect (i).contains (pos))
            return whiteNoteNumbers[i];

    return -1;
}

void PianoKeyboard::mouseDown (const juce::MouseEvent& e)
{
    int midiNote = getNoteAtPosition (e.position);
    if (midiNote < 0) return;

    int pc = midiNote % 12;
    bool isCurrentlySelected = highlightedPitchClasses.count (pc) > 0;

    if (isCurrentlySelected)
    {
        // Deselect — no audio, just update scale detection
        processorRef.togglePitchClassOff (pc);
    }
    else
    {
        // Select — play audio (mono: stops previous) and update scale detection
        processorRef.togglePitchClassOn (pc);
        processorRef.triggerNoteOnMono (midiNote, 0.8f);
        lastPlayedNote = midiNote;
    }
}

void PianoKeyboard::mouseUp (const juce::MouseEvent&)
{
    // Stop the currently playing note (audio stops, but selection persists)
    if (lastPlayedNote >= 0)
    {
        processorRef.triggerNoteOff (lastPlayedNote);
        lastPlayedNote = -1;
    }
}

void PianoKeyboard::setHighlightedNotes (const std::set<int>& notes)
{
    if (highlightedPitchClasses != notes)
    {
        highlightedPitchClasses = notes;
        repaint();
    }
}

void PianoKeyboard::clearSelection()
{
    highlightedPitchClasses.clear();
    lastPlayedNote = -1;
    repaint();
}

// ═══════════════════════════════════════════════════════════════════════════
// ScaleFinderEditor
// ═══════════════════════════════════════════════════════════════════════════

ScaleFinderEditor::ScaleFinderEditor (ScaleFinderProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), pianoKeyboard (p)
{
    setSize (460, 660);

    // ── Title labels ──────────────────────────────────────────────────
    titleLabel1.setText ("SCALE FINDER", juce::dontSendNotification);
    titleLabel1.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    titleLabel1.setColour (juce::Label::textColourId, purpleText);
    titleLabel1.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel1);

    titleLabel2.setText ("S T U D I O", juce::dontSendNotification);
    titleLabel2.setFont (juce::FontOptions (13.0f));
    titleLabel2.setColour (juce::Label::textColourId, mutedText);
    titleLabel2.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel2);

    // ── Piano ─────────────────────────────────────────────────────────
    addAndMakeVisible (pianoKeyboard);

    // ── Reset button ──────────────────────────────────────────────────
    resetButton.setColour (juce::TextButton::buttonColourId, accentColour);
    resetButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    resetButton.onClick = [this]() {
        processorRef.clearNotes();
        pianoKeyboard.clearSelection();
        keyDropdown.setSelectedId (0, juce::dontSendNotification);
        updateUI();
    };
    addAndMakeVisible (resetButton);

    // ── Key dropdown ──────────────────────────────────────────────────
    keyDropdown.setTextWhenNothingSelected ("Select key...");
    keyDropdown.setColour (juce::ComboBox::backgroundColourId, cardColour);
    keyDropdown.setColour (juce::ComboBox::textColourId, textColour);
    keyDropdown.setColour (juce::ComboBox::outlineColourId, borderColour);
    keyDropdown.onChange = [this]()
    {
        int id = keyDropdown.getSelectedId();
        if (id <= 0) return;

        auto allKeys = MusicTheory::allKeys();
        int idx = id - 1;
        if (idx >= 0 && idx < (int) allKeys.size())
            onKeyButtonClicked (allKeys[(size_t) idx].name);
    };
    addAndMakeVisible (keyDropdown);
    populateKeyDropdown();

    updateUI();
    startTimerHz (30);
}

ScaleFinderEditor::~ScaleFinderEditor()
{
    stopTimer();
}

void ScaleFinderEditor::paint (juce::Graphics& g)
{
    // Gradient background matching web app
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f,
                                              bgBottom, 0.0f, (float) getHeight(),
                                              false));
    g.fillAll();

    auto result = processorRef.getCurrentResult();

    // ── Detected keys section ─────────────────────────────────────────
    int keysStartY = 395;

    if (result.status == "some")
    {
        bool hasMajor = false, hasMinor = false;
        for (auto& k : result.keys)
        {
            if (k.type == "Major") hasMajor = true;
            if (k.type == "Minor") hasMinor = true;
        }

        int keysY = keysStartY;
        if (hasMajor)
        {
            g.setColour (mutedText);
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText ("MAJOR", 30, keysY, getWidth() - 60, 16, juce::Justification::centredLeft);
            keysY += 18;

            int x = 30;
            for (auto& btn : majorButtons)
            {
                int w = juce::Font (13.0f).getStringWidth (btn->getButtonText()) + 24;
                if (x + w > getWidth() - 30) { x = 30; keysY += 30; }
                btn->setBounds (x, keysY, w, 26);
                x += w + 6;
            }
            keysY += 34;
        }

        if (hasMinor)
        {
            g.setColour (mutedText);
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText ("MINOR", 30, keysY, getWidth() - 60, 16, juce::Justification::centredLeft);
            keysY += 18;

            int x = 30;
            for (auto& btn : minorButtons)
            {
                int w = juce::Font (13.0f).getStringWidth (btn->getButtonText()) + 24;
                if (x + w > getWidth() - 30) { x = 30; keysY += 30; }
                btn->setBounds (x, keysY, w, 26);
                x += w + 6;
            }
            keysY += 34;
        }

        // Chord card below detected keys
        if (processorRef.selectedKey.isNotEmpty() && ! processorRef.currentChords.empty())
        {
            float cardX = 20.0f;
            float cardY = (float) keysY + 5.0f;
            float cardW = (float) getWidth() - 40.0f;
            float cardH = 120.0f;
            auto chordArea = juce::Rectangle<float> (cardX, cardY, cardW, cardH);

            // Card background
            g.setColour (cardColour.withAlpha (0.5f));
            g.fillRoundedRectangle (chordArea, 10.0f);
            g.setColour (borderColour.withAlpha (0.4f));
            g.drawRoundedRectangle (chordArea, 10.0f, 1.0f);

            // Key title at top of card
            juce::String displayKey = MusicTheory::getKeyDisplayName (processorRef.selectedKey);
            g.setColour (accentColour);
            g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
            g.drawText (displayKey, (int) cardX, (int) cardY + 6, (int) cardW, 18,
                        juce::Justification::centred);

            // Determine if major or minor for degree names
            bool isMajor = processorRef.selectedKey.contains ("Major");
            const char** degreeNames = isMajor ? MAJOR_DEGREE_NAMES : MINOR_DEGREE_NAMES;

            // 7 equal columns
            float colW = cardW / 7.0f;
            float contentX = cardX;
            float row1Y = cardY + 28.0f;   // Chord names
            float row2Y = cardY + 50.0f;   // Roman numerals
            float row3Y = cardY + 68.0f;   // Function names

            for (int i = 0; i < (int) processorRef.currentChords.size() && i < 7; ++i)
            {
                auto& chord = processorRef.currentChords[(size_t) i];
                float cx = contentX + i * colW;

                // Separator line between columns (skip first)
                if (i > 0)
                {
                    g.setColour (borderColour.withAlpha (0.25f));
                    g.drawVerticalLine ((int) cx, cardY + 26.0f, cardY + cardH - 8.0f);
                }

                // Chord name (bold, white)
                g.setColour (textColour);
                g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
                g.drawText (chord.name, (int) cx, (int) row1Y, (int) colW, 20,
                            juce::Justification::centred);

                // Roman numeral (muted purple)
                g.setColour (purpleText.withAlpha (0.8f));
                g.setFont (juce::FontOptions (11.0f));
                g.drawText (chord.roman, (int) cx, (int) row2Y, (int) colW, 16,
                            juce::Justification::centred);

                // Function name (dim, small)
                g.setColour (mutedText.withAlpha (0.5f));
                g.setFont (juce::FontOptions (8.5f));
                g.drawFittedText (degreeNames[i], (int) cx + 2, (int) row3Y, (int) colW - 4, 30,
                                  juce::Justification::centredTop, 2);
            }
        }
        else
        {
            // "Click a key to see chords" hint
            g.setColour (mutedText.withAlpha (0.6f));
            g.setFont (juce::FontOptions (13.0f));
            g.drawText ("Click a key to see chords",
                        30, keysY + 10, getWidth() - 60, 20,
                        juce::Justification::centred);
        }
    }
    else if (result.status == "all-visible")
    {
        // No notes played yet — show hint with drop zone info
        g.setColour (mutedText.withAlpha (0.5f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("Play notes or drop an audio file",
                    30, keysStartY + 25, getWidth() - 60, 24,
                    juce::Justification::centred);
        g.setColour (mutedText.withAlpha (0.3f));
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (".wav  .mp3  .aiff  .flac",
                    30, keysStartY + 48, getWidth() - 60, 18,
                    juce::Justification::centred);
    }
    else if (result.status == "none")
    {
        g.setColour (mutedText.withAlpha (0.5f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("No matching keys found",
                    30, keysStartY + 30, getWidth() - 60, 24,
                    juce::Justification::centred);
    }
    else if (result.status == "all")
    {
        g.setColour (mutedText.withAlpha (0.5f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("All 12 notes selected (chromatic)",
                    30, keysStartY + 30, getWidth() - 60, 24,
                    juce::Justification::centred);
    }

    // ── Analysis status text ─────────────────────────────────────────────
    if (analysisStatusText.isNotEmpty() && ! isDragOver)
    {
        g.setColour (accentColour.withAlpha (0.8f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText (analysisStatusText,
                    30, keysStartY + 30, getWidth() - 60, 24,
                    juce::Justification::centred);
    }

    // ── Drag overlay (on top of everything) ──────────────────────────────
    if (isDragOver)
    {
        auto bounds = getLocalBounds().toFloat();

        // Semi-transparent background
        g.setColour (bgTop.withAlpha (0.88f));
        g.fillRect (bounds);

        // Dashed-style border (using rounded rect with accent colour)
        auto dropZone = bounds.reduced (20.0f);
        g.setColour (accentColour.withAlpha (0.6f));
        g.drawRoundedRectangle (dropZone, 12.0f, 2.0f);

        // Inner dashed effect (slightly smaller rect)
        auto innerZone = dropZone.reduced (4.0f);
        float dashLen = 10.0f;
        float gapLen = 6.0f;
        g.setColour (accentColour.withAlpha (0.3f));
        g.drawRoundedRectangle (innerZone, 10.0f, 1.0f);

        // Main text
        g.setColour (purpleText);
        g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
        g.drawText ("Drop Audio File",
                    bounds, juce::Justification::centred);

        // Subtitle with supported formats
        g.setColour (mutedText);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (".wav  .mp3  .aiff  .flac",
                    0, (int) bounds.getCentreY() + 18, getWidth(), 20,
                    juce::Justification::centred);
    }
}

void ScaleFinderEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title
    titleLabel1.setBounds (bounds.getX(), 15, bounds.getWidth(), 32);
    titleLabel2.setBounds (bounds.getX(), 45, bounds.getWidth(), 18);

    // Piano keyboard
    pianoKeyboard.setBounds (30, 80, getWidth() - 60, 220);

    // Reset button (below piano)
    resetButton.setBounds ((getWidth() - 200) / 2, 310, 200, 32);

    // Key dropdown (below reset, matching web app order)
    keyDropdown.setBounds (55, 355, getWidth() - 110, 28);

    // Chords are custom-painted in paint()
}

void ScaleFinderEditor::timerCallback()
{
    if (processorRef.needsUIUpdate.exchange (false))
        updateUI();

    // Check if audio analysis finished
    if (audioAnalyzer.isAnalysisComplete())
    {
        auto pitchClasses = audioAnalyzer.getDetectedPitchClasses();
        if (! pitchClasses.empty())
        {
            processorRef.setAccumulatedNotes (pitchClasses);
            analysisStatusText = "";
        }
        else
        {
            analysisStatusText = "No pitches detected";
        }
        updateUI();
    }
}

void ScaleFinderEditor::updateUI()
{
    auto accumulated = processorRef.getAccumulatedNotes();
    pianoKeyboard.setHighlightedNotes (accumulated);

    auto result = processorRef.getCurrentResult();
    rebuildKeyButtons (result);
    updateChordsDisplay();
    repaint();
}

void ScaleFinderEditor::rebuildKeyButtons (const KeyResult& result)
{
    for (auto& btn : majorButtons) removeChildComponent (btn.get());
    for (auto& btn : minorButtons) removeChildComponent (btn.get());
    majorButtons.clear();
    minorButtons.clear();

    if (result.status != "some")
        return;

    for (auto& key : result.keys)
    {
        auto btn = std::make_unique<juce::TextButton> (key.displayName);

        bool isSelected = (processorRef.selectedKey == key.name);
        btn->setColour (juce::TextButton::buttonColourId,
                        isSelected ? accentColour : cardColour);
        btn->setColour (juce::TextButton::textColourOffId, textColour);

        juce::String keyName = key.name;
        btn->onClick = [this, keyName]() { onKeyButtonClicked (keyName); };
        addAndMakeVisible (btn.get());

        if (key.type == "Major")
            majorButtons.push_back (std::move (btn));
        else
            minorButtons.push_back (std::move (btn));
    }
}

void ScaleFinderEditor::onKeyButtonClicked (const juce::String& keyName)
{
    if (processorRef.selectedKey == keyName)
    {
        // Deselect — clear everything
        processorRef.selectedKey = "";
        processorRef.currentChords.clear();
        processorRef.clearNotes();
        pianoKeyboard.clearSelection();
    }
    else
    {
        // Select — highlight scale notes on piano
        processorRef.selectedKey = keyName;
        processorRef.currentChords = MusicTheory::getChordProgressions (keyName);

        // Find the KeyInfo to get pitch classes and highlight on piano
        auto allKeys = MusicTheory::allKeys();
        for (auto& k : allKeys)
        {
            if (k.name == keyName)
            {
                processorRef.setAccumulatedNotes (k.pitchClasses);
                break;
            }
        }
    }
    updateUI();
}

void ScaleFinderEditor::updateChordsDisplay()
{
    // All chord rendering is done in paint() — just trigger repaint
    repaint();
}

// ═══════════════════════════════════════════════════════════════════════════
// Drag & Drop
// ═══════════════════════════════════════════════════════════════════════════

bool ScaleFinderEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File (f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".aiff" || ext == ".aif"
            || ext == ".flac" || ext == ".ogg")
            return true;
    }
    return false;
}

void ScaleFinderEditor::fileDragEnter (const juce::StringArray&, int, int)
{
    isDragOver = true;
    repaint();
}

void ScaleFinderEditor::fileDragExit (const juce::StringArray&)
{
    isDragOver = false;
    repaint();
}

void ScaleFinderEditor::filesDropped (const juce::StringArray& files, int, int)
{
    isDragOver = false;
    if (files.isEmpty()) return;

    juce::File audioFile (files[0]);

    // Clear previous state
    processorRef.clearNotes();
    pianoKeyboard.clearSelection();
    keyDropdown.setSelectedId (0, juce::dontSendNotification);

    analysisStatusText = "Analyzing...";
    repaint();

    audioAnalyzer.analyzeFile (audioFile, processorRef.getAnalysisSampleRate());
}

void ScaleFinderEditor::populateKeyDropdown()
{
    keyDropdown.clear();
    auto allKeys = MusicTheory::allKeys();

    keyDropdown.addSectionHeading ("Major");
    for (int i = 0; i < 12; ++i)
        keyDropdown.addItem (allKeys[(size_t) i].displayName, i + 1);

    keyDropdown.addSectionHeading ("Minor");
    for (int i = 12; i < 24; ++i)
        keyDropdown.addItem (allKeys[(size_t) i].displayName, i + 1);
}
