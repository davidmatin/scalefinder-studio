#include "PluginEditor.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

static const char* whiteKeyNames[7] = { "c", "d", "e", "f", "g", "a", "b" };
static const char* blackKeyNames[5] = { "c#", "e\xe2\x99\xad", "f#", "a\xe2\x99\xad", "b\xe2\x99\xad" };

// ═══════════════════════════════════════════════════════════════════════════
// PianoKeyboard
// ═══════════════════════════════════════════════════════════════════════════

constexpr int PianoKeyboard::whiteNoteNumbers[7];
constexpr int PianoKeyboard::blackNoteNumbers[5];
constexpr int PianoKeyboard::blackKeyPositions[5];

// ═══════════════════════════════════════════════════════════════════════════
// Title bar LookAndFeels
// ═══════════════════════════════════════════════════════════════════════════

// ── Purple-themed document window button ─────────────────────────────
class PurpleWindowButton final : public juce::Button
{
public:
    PurpleWindowButton (const juce::String& name, juce::Colour c,
                        const juce::Path& normal, const juce::Path& toggled)
        : Button (name), colour (c), normalShape (normal), toggledShape (toggled) {}

    void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override
    {
        auto background = juce::Colours::grey;
        if (auto* rw = findParentComponentOfClass<juce::ResizableWindow>())
            if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*> (&rw->getLookAndFeel()))
                background = lf->getCurrentColourScheme().getUIColour (
                    juce::LookAndFeel_V4::ColourScheme::widgetBackground);

        g.fillAll (background);
        g.setColour ((! isEnabled() || isDown) ? colour.withAlpha (0.6f) : colour);

        if (isHighlighted)
        {
            g.fillAll();
            g.setColour (background);
        }

        auto& p = getToggleState() ? toggledShape : normalShape;
        auto reducedRect = juce::Justification (juce::Justification::centred)
                              .appliedToRectangle (juce::Rectangle<int> (getHeight(), getHeight()), getLocalBounds())
                              .toFloat()
                              .reduced ((float) getHeight() * 0.3f);
        g.fillPath (p, p.getTransformToScaleToFit (reducedRect, true));
    }

private:
    juce::Colour colour;
    juce::Path normalShape, toggledShape;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PurpleWindowButton)
};

juce::Button* TitleBarLookAndFeel::createDocumentWindowButton (int buttonType)
{
    juce::Path shape;
    auto crossThickness = 0.15f;
    juce::Colour purple (0xff8b5cf6);

    if (buttonType == juce::DocumentWindow::closeButton)
    {
        shape.addLineSegment ({ 0.0f, 0.0f, 1.0f, 1.0f }, crossThickness);
        shape.addLineSegment ({ 1.0f, 0.0f, 0.0f, 1.0f }, crossThickness);
        return new PurpleWindowButton ("close", purple, shape, shape);
    }
    if (buttonType == juce::DocumentWindow::minimiseButton)
    {
        shape.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);
        return new PurpleWindowButton ("minimise", purple.withAlpha (0.6f), shape, shape);
    }
    if (buttonType == juce::DocumentWindow::maximiseButton)
    {
        shape.addLineSegment ({ 0.5f, 0.0f, 0.5f, 1.0f }, crossThickness);
        shape.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);
        return new PurpleWindowButton ("maximise", purple.withAlpha (0.6f), shape, shape);
    }
    jassertfalse;
    return nullptr;
}

void TitleBarLookAndFeel::positionDocumentWindowButtons (juce::DocumentWindow&,
    int titleBarX, int titleBarY, int /*titleBarW*/, int titleBarH,
    juce::Button* minimiseButton, juce::Button* maximiseButton,
    juce::Button* closeButton, bool)
{
    auto buttonW = titleBarH - titleBarH / 8;
    auto y = titleBarY + (titleBarH - buttonW) / 2;
    auto x = titleBarX + 6;

    if (closeButton != nullptr)
    {
        closeButton->setBounds (x, y, buttonW, buttonW);
        x += buttonW + 2;
    }
    if (minimiseButton != nullptr)
    {
        minimiseButton->setBounds (x, y, buttonW, buttonW);
        x += buttonW + 2;
    }
    if (maximiseButton != nullptr)
        maximiseButton->setBounds (x, y, buttonW, buttonW);
}

void DropdownButtonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                        const juce::Colour&, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto outlineCol = button.findColour (juce::ComboBox::outlineColourId);
    g.setColour (outlineCol);
    g.drawRoundedRectangle (bounds, 6.0f, 1.5f);
}

void DropdownButtonLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                                  bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto textCol = button.findColour (juce::TextButton::textColourOffId);

    // Text (left-aligned with padding)
    g.setColour (textCol);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (button.getButtonText(), bounds.reduced (12.0f, 0.0f).withTrimmedRight (24.0f),
                juce::Justification::centredLeft);

    // Arrow (right side)
    float arrowX = bounds.getRight() - 24.0f;
    float arrowY = bounds.getCentreY();
    juce::Path arrow;
    arrow.addTriangle (arrowX - 4.0f, arrowY - 2.0f,
                       arrowX + 4.0f, arrowY - 2.0f,
                       arrowX, arrowY + 3.0f);
    g.setColour (textCol);
    g.fillPath (arrow);
}

void ResetButtonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                     const juce::Colour&, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto outlineCol = button.findColour (juce::ComboBox::outlineColourId);
    g.setColour (outlineCol);
    g.drawRoundedRectangle (bounds, 6.0f, 1.5f);
}

void BrowseIconLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                     const juce::Colour&, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto outlineCol = button.findColour (juce::ComboBox::outlineColourId);
    g.setColour (outlineCol);
    g.drawRoundedRectangle (bounds, 6.0f, 1.5f);
}

void BrowseIconLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                              bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (7.0f, 7.0f);
    auto outlineCol = button.findColour (juce::ComboBox::outlineColourId);
    g.setColour (outlineCol);

    float x = bounds.getX(), y = bounds.getY();
    float w = bounds.getWidth(), h = bounds.getHeight();
    float tabW = w * 0.4f, tabH = h * 0.2f, r = 1.5f;

    juce::Path folder;
    folder.startNewSubPath (x + r, y + h);
    folder.quadraticTo (x, y + h, x, y + h - r);
    folder.lineTo (x, y + tabH + r);
    folder.quadraticTo (x, y, x + r, y);
    folder.lineTo (x + tabW - r, y);
    folder.quadraticTo (x + tabW, y, x + tabW, y + tabH);
    folder.lineTo (x + w - r, y + tabH);
    folder.quadraticTo (x + w, y + tabH, x + w, y + tabH + r);
    folder.lineTo (x + w, y + h - r);
    folder.quadraticTo (x + w, y + h, x + w - r, y + h);
    folder.closeSubPath();

    g.strokePath (folder, juce::PathStrokeType (1.3f));
}

void OptionsIconLookAndFeel::drawButtonBackground (juce::Graphics&, juce::Button&,
                                                     const juce::Colour&, bool, bool) {}

void OptionsIconLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                               bool isHighlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (3.0f, 3.0f);
    auto col = isActive       ? juce::Colour (0xff8b5cf6)
             : isHighlighted  ? juce::Colour (0xff8B90A0)
                              : juce::Colour (0xff4A4F62);

    // Three vertical dots — clean, minimal
    float dotR = 2.0f;
    float cx = bounds.getCentreX();
    float y1 = bounds.getY() + bounds.getHeight() * 0.22f;
    float y2 = bounds.getCentreY();
    float y3 = bounds.getY() + bounds.getHeight() * 0.78f;

    g.setColour (col);
    g.fillEllipse (cx - dotR, y1 - dotR, dotR * 2.0f, dotR * 2.0f);
    g.fillEllipse (cx - dotR, y2 - dotR, dotR * 2.0f, dotR * 2.0f);
    g.fillEllipse (cx - dotR, y3 - dotR, dotR * 2.0f, dotR * 2.0f);
}

// ── Keyboard toggle icon LookAndFeel ──────────────────────────────────────

void KeyboardIconLookAndFeel::drawButtonBackground (juce::Graphics&, juce::Button&,
                                                      const juce::Colour&, bool, bool) {}

void KeyboardIconLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                                bool isHighlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (4.0f, 4.0f);

    juce::Colour col;
    if (isEnabled)
        col = isHighlighted ? juce::Colour (0xffa78bfa) : juce::Colour (0xff8b5cf6);
    else
        col = isHighlighted ? juce::Colour (0xff8B90A0) : juce::Colour (0xff4A4F62);

    // Square piano icon — 3 white keys + 2 black keys
    float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    float x0 = bounds.getCentreX() - size / 2.0f;
    float y0 = bounds.getCentreY() - size / 2.0f;
    float border = size * 0.1f;

    // Outer border
    g.setColour (col);
    g.drawRect (x0, y0, size, size, border);

    // Inner area (inside the border)
    float ix = x0 + border;
    float iy = y0 + border;
    float iw = size - border * 2.0f;
    float ih = size - border * 2.0f;

    // 3 white keys with thin gaps
    float gap = 0.5f;
    float whiteKeyW = (iw - gap * 2.0f) / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        float kx = ix + i * (whiteKeyW + gap);
        g.setColour (col);
        g.drawRect (kx, iy, whiteKeyW, ih, 0.5f);
    }

    // 2 black keys (60% height, between white keys)
    float blackH = ih * 0.55f;
    float blackW = whiteKeyW * 0.55f;

    for (int i = 0; i < 2; ++i)
    {
        float kx = ix + (i + 1) * (whiteKeyW + gap) - blackW / 2.0f;
        g.setColour (col);
        g.fillRect (kx, iy, blackW, blackH);
    }
}

// ── App-wide popup menu LookAndFeel ────────────────────────────────────────

AppMenuLookAndFeel::AppMenuLookAndFeel()
{
    auto scheme = getDarkColourScheme();
    scheme.setUIColour (ColourScheme::widgetBackground, juce::Colour (0xff0f0a1a));
    scheme.setUIColour (ColourScheme::windowBackground, juce::Colour (0xff0f0a1a));
    scheme.setUIColour (ColourScheme::outline, juce::Colour (0xff0f0a1a));
    setColourScheme (scheme);

    // Slightly transparent so MenuWindow becomes non-opaque (enables rounded corners)
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xf91a1a2e));
    setColour (juce::PopupMenu::textColourId,       juce::Colour (0xffE8EAF0));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff222238));
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colour (0xffE8EAF0));
    setColour (juce::PopupMenu::headerTextColourId,            juce::Colour (0xff8B90A0));

    // Audio/MIDI Settings dialog theming
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff1a1a2e));
    setColour (juce::Label::textColourId,                 juce::Colour (0xffE8EAF0));
    setColour (juce::Label::outlineColourId,              juce::Colour (0x00000000));
    setColour (juce::TextButton::buttonColourId,          juce::Colour (0xff252540));
    setColour (juce::TextButton::buttonOnColourId,        juce::Colour (0xff3730a3));
    setColour (juce::TextButton::textColourOffId,         juce::Colour (0xffE8EAF0));
    setColour (juce::TextButton::textColourOnId,          juce::Colour (0xffE8EAF0));
    setColour (juce::ComboBox::backgroundColourId,        juce::Colour (0xff1C2030));
    setColour (juce::ComboBox::textColourId,              juce::Colour (0xffE8EAF0));
    setColour (juce::ComboBox::outlineColourId,           juce::Colour (0x14ffffff));
    setColour (juce::ComboBox::arrowColourId,             juce::Colour (0xff8B90A0));
    setColour (juce::ListBox::backgroundColourId,         juce::Colour (0xff1C2030));
    setColour (juce::ListBox::textColourId,               juce::Colour (0xffE8EAF0));
    setColour (juce::ToggleButton::textColourId,          juce::Colour (0xffE8EAF0));
    setColour (juce::ToggleButton::tickColourId,          juce::Colour (0xff8b5cf6));
    setColour (juce::TextEditor::backgroundColourId,      juce::Colour (0xff1C2030));
    setColour (juce::TextEditor::textColourId,            juce::Colour (0xffE8EAF0));
    setColour (juce::TextEditor::outlineColourId,         juce::Colour (0x14ffffff));
    setColour (juce::Slider::backgroundColourId,          juce::Colour (0xff1C2030));
    setColour (juce::Slider::thumbColourId,               juce::Colour (0xff8b5cf6));
    setColour (juce::Slider::trackColourId,               juce::Colour (0xff6366f1));
}

void AppMenuLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setColour (juce::Colour (0xf91a1a2e));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (juce::Colour (0x14ffffff));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
}

void AppMenuLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                             bool isSeparator, bool isActive, bool isHighlighted,
                                             bool /*isTicked*/, bool /*hasSubMenu*/,
                                             const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                             const juce::Drawable* /*icon*/, const juce::Colour* /*textColourToUse*/)
{
    if (isSeparator)
    {
        auto sepArea = area.reduced (8, 0).withSizeKeepingCentre (area.getWidth() - 16, 1);
        g.setColour (juce::Colour (0x14ffffff));
        g.fillRect (sepArea);
        return;
    }

    auto r = area.reduced (4, 1);

    if (isHighlighted && isActive)
    {
        g.setColour (juce::Colour (0xff222238));
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff8b5cf6));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 4.0f, 1.0f);
    }

    g.setColour (isActive ? juce::Colour (0xffE8EAF0) : juce::Colour (0xff4A4F62));
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (text, r.reduced (10, 0), juce::Justification::centredLeft);
}

int AppMenuLookAndFeel::getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&)
{
    return 6;
}

void AppMenuLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text,
                                       int width, int height)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    // Fill entire rect first (covers native window corners)
    g.fillAll (Theme::cardBg());

    g.setColour (Theme::cardBg());
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (Theme::borderSubtle());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    g.setColour (Theme::textSecondary());
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (text, bounds.reduced (6.0f, 3.0f), juce::Justification::centredLeft);
}

// ═══════════════════════════════════════════════════════════════════════════
// PianoKeyboard
// ═══════════════════════════════════════════════════════════════════════════

PianoKeyboard::PianoKeyboard (ScaleFinderProcessor& p) : processorRef (p)
{
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (true);
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
    float cornerRadius = 12.0f;

    // Monospace font helper for note names
    auto monoFont = [] (float size, int style = juce::Font::plain) {
        return juce::FontOptions (size, style)
            .withName (juce::Font::getDefaultMonospacedFontName());
    };

    // Piano card background — dark so rounded key bottoms create visible gaps
    g.setColour (juce::Colour (0xff1a1a2e));
    g.fillRoundedRectangle (bounds, cornerRadius);

    // Clip everything to the rounded card shape so nothing bleeds outside
    juce::Path clipPath;
    clipPath.addRoundedRectangle (bounds, cornerRadius);
    g.reduceClipRegion (clipPath);

    // Helper lambda: create a path with sharp top corners, rounded bottom corners
    auto makeBottomRoundedPath = [] (juce::Rectangle<float> area, float radius) -> juce::Path
    {
        juce::Path p;
        float x = area.getX(), y = area.getY();
        float w = area.getWidth(), h = area.getHeight();
        float r = juce::jmin (radius, w * 0.5f, h * 0.5f);
        p.startNewSubPath (x, y);                               // top-left (sharp)
        p.lineTo (x + w, y);                                    // top-right (sharp)
        p.lineTo (x + w, y + h - r);                            // down right side
        p.quadraticTo (x + w, y + h, x + w - r, y + h);        // bottom-right round
        p.lineTo (x + r, y + h);                                // bottom edge
        p.quadraticTo (x, y + h, x, y + h - r);                // bottom-left round
        p.closeSubPath();
        return p;
    };

    // ── Draw white keys (fill pass) ─────────────────────────────────────
    for (int i = 0; i < 7; ++i)
    {
        auto r = getWhiteKeyRect (i);
        int pc = whiteNoteNumbers[i] % 12;
        bool highlighted = highlightedPitchClasses.count (pc) > 0;
        bool isRoot = highlighted && (pc == rootPitchClass);
        bool pressed = pressedPitchClasses.count (pc) > 0;
        bool hovered = (pc == hoveredPitchClass);

        auto keyArea = r;
        if (pressed)
            keyArea = keyArea.translated (0.0f, 2.0f);

        auto keyPath = makeBottomRoundedPath (keyArea, cornerRadius);

        if (highlighted)
        {
            if (isRoot)
            {
                // Root note: lighter purple — brighter on hover
                auto topCol = hovered ? juce::Colour (0xff5b54ef) : juce::Colour (0xff4f46e5);
                auto btmCol = hovered ? juce::Colour (0xff4f46d6) : juce::Colour (0xff4338ca);
                juce::ColourGradient rootGrad (topCol, keyArea.getX(), keyArea.getY(),
                                                btmCol, keyArea.getX(), keyArea.getBottom(), false);
                g.setGradientFill (rootGrad);
            }
            else
            {
                // Scale tone: standard purple — brighter on hover
                g.setColour (hovered ? juce::Colour (0xff4338b4) : juce::Colour (0xff3730a3));
            }
            g.fillPath (keyPath);
        }
        else
        {
            // Normal: softer gradient — lighter on hover
            auto topCol = hovered ? juce::Colour (0xffE5E6E9) : juce::Colour (0xffEDEEF0);
            auto btmCol = hovered ? juce::Colour (0xffD8DADD) : juce::Colour (0xffE0E2E5);
            juce::ColourGradient whiteGrad (topCol, keyArea.getX(), keyArea.getY(),
                                             btmCol, keyArea.getX(), keyArea.getBottom(), false);
            g.setGradientFill (whiteGrad);
            g.fillPath (keyPath);

            if (pressed)
            {
                g.setColour (juce::Colour (0x0c000000));
                g.fillPath (keyPath);
            }
        }

        // Separator lines — thin gray between non-selected adjacent keys
        if (i > 0)
        {
            int prevPc = whiteNoteNumbers[i - 1] % 12;
            bool prevHighlighted = highlightedPitchClasses.count (prevPc) > 0;

            if (! highlighted && ! prevHighlighted)
            {
                g.setColour (juce::Colour (0x14000000));
                g.fillRect (r.getX() - 0.25f, bounds.getY(), 0.5f, bounds.getHeight() - cornerRadius);
            }
        }
    }

    // ── Draw white key borders & labels (second pass, on top of fills) ───
    for (int i = 0; i < 7; ++i)
    {
        auto r = getWhiteKeyRect (i);
        int pc = whiteNoteNumbers[i] % 12;
        bool highlighted = highlightedPitchClasses.count (pc) > 0;
        bool isRoot = highlighted && (pc == rootPitchClass);
        bool pressed = pressedPitchClasses.count (pc) > 0;

        auto keyArea = r;
        if (pressed)
            keyArea = keyArea.translated (0.0f, 2.0f);

        if (highlighted)
        {
            auto keyPath = makeBottomRoundedPath (keyArea, cornerRadius);

            // Outer glow
            auto glowPath = makeBottomRoundedPath (keyArea.expanded (1.0f), cornerRadius + 1.0f);
            g.setColour (isRoot ? juce::Colour (0x664f46e5) : juce::Colour (0x663730a3));
            g.strokePath (glowPath, juce::PathStrokeType (2.0f));

            // Border: 2px solid #1e1b4b (indigo-950)
            g.setColour (juce::Colour (0xff1e1b4b));
            g.strokePath (keyPath, juce::PathStrokeType (2.0f));

            // Inset shadow: inset 0 2px 4px rgba(0,0,0,0.2)
            {
                juce::ColourGradient insetShadow (juce::Colour (0x33000000),
                                                    keyArea.getX(), keyArea.getY(),
                                                    juce::Colour (0x00000000),
                                                    keyArea.getX(), keyArea.getY() + 5.0f,
                                                    false);
                g.setGradientFill (insetShadow);
                auto insetPath = makeBottomRoundedPath (keyArea.reduced (2.0f), cornerRadius - 2.0f);
                g.fillPath (insetPath);
            }

            // Inner highlight: inset 0 0 0 1px rgba(255,255,255,0.1)
            g.setColour (juce::Colour (0x1affffff));
            auto innerPath = makeBottomRoundedPath (keyArea.reduced (2.0f), cornerRadius - 2.0f);
            g.strokePath (innerPath, juce::PathStrokeType (1.0f));
        }

        // Key label (monospace, plain weight)
        auto labelArea = juce::Rectangle<float> (keyArea.getX(), keyArea.getBottom() - 28.0f,
                                                  keyArea.getWidth(), 24.0f);
        g.setColour (highlighted ? juce::Colours::white : juce::Colour (0xff4A4F62));
        g.setFont (monoFont (11.0f));
        g.drawText (whiteKeyNames[i], labelArea, juce::Justification::centred);
    }

    // ── Draw black keys (on top) ─────────────────────────────────────────
    for (int i = 0; i < 5; ++i)
    {
        auto r = getBlackKeyRect (i);
        int pc = blackNoteNumbers[i] % 12;
        bool highlighted = highlightedPitchClasses.count (pc) > 0;
        bool isRoot = highlighted && (pc == rootPitchClass);
        bool pressed = pressedPitchClasses.count (pc) > 0;
        bool hovered = (pc == hoveredPitchClass);

        auto keyRect = r;
        if (pressed)
            keyRect = keyRect.translated (0.0f, 2.0f);

        // Path with only bottom corners rounded (borderRadius: "0 0 12px 12px")
        juce::Path blackKeyPath;
        float bx = keyRect.getX(), by = keyRect.getY();
        float bw = keyRect.getWidth(), bh = keyRect.getHeight();
        float br = cornerRadius;  // 12px — matches web borderRadius: "0 0 12px 12px"
        blackKeyPath.startNewSubPath (bx, by);
        blackKeyPath.lineTo (bx + bw, by);
        blackKeyPath.lineTo (bx + bw, by + bh - br);
        blackKeyPath.quadraticTo (bx + bw, by + bh, bx + bw - br, by + bh);
        blackKeyPath.lineTo (bx + br, by + bh);
        blackKeyPath.quadraticTo (bx, by + bh, bx, by + bh - br);
        blackKeyPath.closeSubPath();

        if (highlighted)
        {
            // Outer glow — brighter on hover
            auto glowCol = isRoot ? juce::Colour (hovered ? 0x995b54ef : 0x804f46e5)
                                  : juce::Colour (hovered ? 0x994338b4 : 0x803730a3);
            g.setColour (glowCol);
            g.strokePath (blackKeyPath, juce::PathStrokeType (3.0f));

            // Fill: lighter purple for root, standard purple for scale tones — brighter on hover
            if (isRoot)
            {
                auto topCol = hovered ? juce::Colour (0xff5b54ef) : juce::Colour (0xff4f46e5);
                auto btmCol = hovered ? juce::Colour (0xff4f46d6) : juce::Colour (0xff4338ca);
                juce::ColourGradient rootGrad (topCol, keyRect.getX(), keyRect.getY(),
                                                btmCol, keyRect.getX(), keyRect.getBottom(), false);
                g.setGradientFill (rootGrad);
            }
            else
            {
                g.setColour (hovered ? juce::Colour (0xff4338b4) : juce::Colour (0xff3730a3));
            }
            g.fillPath (blackKeyPath);

            // Border: 2px solid #0c0a2a
            g.setColour (juce::Colour (0xff0c0a2a));
            g.strokePath (blackKeyPath, juce::PathStrokeType (2.0f));

            // Inset shadow: inset 0 2px 4px rgba(0,0,0,0.3)
            {
                juce::ColourGradient insetShadow (juce::Colour (0x4c000000),
                                                    keyRect.getX(), keyRect.getY(),
                                                    juce::Colour (0x00000000),
                                                    keyRect.getX(), keyRect.getY() + 5.0f,
                                                    false);
                g.setGradientFill (insetShadow);
                g.fillPath (blackKeyPath);
            }

            // Inner highlight: inset 0 0 0 1px rgba(255,255,255,0.05)
            g.setColour (juce::Colour (0x0dffffff));
            {
                juce::Path innerPath;
                float inset = 2.5f;
                float ibx = bx + inset, iby = by + inset;
                float ibw = bw - inset * 2.0f, ibh = bh - inset * 2.0f;
                float ibr = br - inset;
                if (ibr < 1.0f) ibr = 1.0f;
                innerPath.startNewSubPath (ibx, iby);
                innerPath.lineTo (ibx + ibw, iby);
                innerPath.lineTo (ibx + ibw, iby + ibh - ibr);
                innerPath.quadraticTo (ibx + ibw, iby + ibh, ibx + ibw - ibr, iby + ibh);
                innerPath.lineTo (ibx + ibr, iby + ibh);
                innerPath.quadraticTo (ibx, iby + ibh, ibx, iby + ibh - ibr);
                innerPath.closeSubPath();
                g.strokePath (innerPath, juce::PathStrokeType (1.0f));
            }
        }
        else
        {
            // Normal: gradient — lighter on hover
            auto topCol = hovered ? juce::Colour (0xff353535) : juce::Colour (0xff2A2A2A);
            auto btmCol = hovered ? juce::Colour (0xff252525) : juce::Colour (0xff1A1A1A);
            juce::ColourGradient blackGrad (topCol, keyRect.getX(), keyRect.getY(),
                                             btmCol, keyRect.getX(), keyRect.getBottom(), false);
            g.setGradientFill (blackGrad);
            g.fillPath (blackKeyPath);

            // Border: hairline — barely visible
            g.setColour (juce::Colour (0x1e000000));
            g.strokePath (blackKeyPath, juce::PathStrokeType (0.5f));

            if (pressed)
            {
                g.setColour (juce::Colour (0x18ffffff));
                g.fillPath (blackKeyPath);
            }
        }

        // Label (monospace, plain weight)
        auto labelArea = keyRect.withTrimmedTop (keyRect.getHeight() * 0.55f);
        g.setColour (highlighted ? juce::Colours::white : juce::Colour (0xff8B90A0));
        g.setFont (monoFont (11.0f));
        g.drawText (juce::String::fromUTF8 (blackKeyNames[i]),
                    labelArea, juce::Justification::centred);
    }

    // Clean up corner anti-aliasing artifacts at rounded clip boundary
    g.setColour (juce::Colour (0xff0f0a1a));
    juce::Path borderPath;
    borderPath.addRoundedRectangle (bounds, cornerRadius);
    g.strokePath (borderPath, juce::PathStrokeType (2.0f));
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

    // Track pressed state for visual feedback
    pressedPitchClasses.insert (pc);
    repaint();

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
    // Clear pressed visual state
    pressedPitchClasses.clear();
    repaint();

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

void PianoKeyboard::setRootNote (int pitchClass)
{
    if (rootPitchClass != pitchClass)
    {
        rootPitchClass = pitchClass;
        repaint();
    }
}

void PianoKeyboard::clearSelection()
{
    highlightedPitchClasses.clear();
    rootPitchClass = -1;
    lastPlayedNote = -1;
    repaint();
}

void PianoKeyboard::mouseMove (const juce::MouseEvent& e)
{
    int midiNote = getNoteAtPosition (e.position);
    int newHover = (midiNote >= 0) ? (midiNote % 12) : -1;

    if (newHover != hoveredPitchClass)
    {
        hoveredPitchClass = newHover;
        repaint();
    }
}

void PianoKeyboard::mouseExit (const juce::MouseEvent&)
{
    if (hoveredPitchClass != -1)
    {
        hoveredPitchClass = -1;
        repaint();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ScaleResultsPanel
// ═══════════════════════════════════════════════════════════════════════════

ScaleResultsPanel::ScaleResultsPanel()
{
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ScaleResultsPanel::setResults (const std::vector<KeyInfo>& keys, int selectedNoteCount)
{
    cards.clear();
    majorCount = 0;

    for (auto& key : keys)
    {
        CardEntry entry;
        entry.key = key;
        entry.isExactMatch = ((int) key.pitchClasses.size() == selectedNoteCount);

        // Abbreviated chip text: "C Maj", "A min"
        if (key.type == "Major")
            entry.chipText = key.displayName.replace (" Major", " Maj");
        else
            entry.chipText = key.displayName.replace (" Minor", " min");

        cards.push_back (std::move (entry));
    }

    // Sort: exact matches first, then Major before Minor, then by root
    std::stable_sort (cards.begin(), cards.end(),
        [] (const CardEntry& a, const CardEntry& b)
        {
            if (a.isExactMatch != b.isExactMatch)
                return a.isExactMatch;
            if (a.key.type != b.key.type)
                return a.key.type == "Major";
            return a.key.root < b.key.root;
        });

    // Count Major keys for separator placement
    majorCount = 0;
    for (auto& c : cards)
        if (c.key.type == "Major") ++majorCount;

    layoutChips ((float) getWidth());
    repaint();
}

void ScaleResultsPanel::layoutChips (float availableWidth)
{
    if (cards.empty() || availableWidth <= 0.0f)
    {
        separatorY = 0.0f;
        setSize ((int) availableWidth, 1);
        return;
    }

    juce::Font font { juce::FontOptions (chipFontSize) };
    float cursorX = 0.0f;
    float cursorY = 4.0f;  // small top padding

    // Layout Major chips
    for (int i = 0; i < majorCount && i < (int) cards.size(); ++i)
    {
        float textW = font.getStringWidthFloat (cards[(size_t) i].chipText);
        float chipW = textW + chipPadX * 2.0f;

        if (cursorX + chipW > availableWidth && cursorX > 0.0f)
        {
            cursorX = 0.0f;
            cursorY += chipHeight + chipGap;
        }

        cards[(size_t) i].bounds = { cursorX, cursorY, chipW, chipHeight };
        cursorX += chipW + chipGap;
    }

    // Separator between Major and Minor groups
    bool hasMinors = majorCount < (int) cards.size();
    if (majorCount > 0 && hasMinors)
    {
        cursorY += chipHeight + separatorGap;
        separatorY = cursorY;
        cursorY += separatorGap;
    }
    else
    {
        separatorY = 0.0f;
    }

    // Layout Minor chips
    cursorX = 0.0f;
    for (int i = majorCount; i < (int) cards.size(); ++i)
    {
        float textW = font.getStringWidthFloat (cards[(size_t) i].chipText);
        float chipW = textW + chipPadX * 2.0f;

        if (cursorX + chipW > availableWidth && cursorX > 0.0f)
        {
            cursorX = 0.0f;
            cursorY += chipHeight + chipGap;
        }

        cards[(size_t) i].bounds = { cursorX, cursorY, chipW, chipHeight };
        cursorX += chipW + chipGap;
    }

    int totalH = (int) (cursorY + chipHeight + 4.0f);
    setSize ((int) availableWidth, juce::jmax (totalH, 1));
}

void ScaleResultsPanel::resized()
{
    if (! cards.empty())
        layoutChips ((float) getWidth());
}

void ScaleResultsPanel::setSelectedKey (const juce::String& keyName)
{
    if (selectedKeyName != keyName)
    {
        selectedKeyName = keyName;
        repaint();
    }
}

void ScaleResultsPanel::paint (juce::Graphics& g)
{
    // Separator line between Major and Minor groups
    if (majorCount > 0 && majorCount < (int) cards.size() && separatorY > 0.0f)
    {
        g.setColour (Theme::borderSubtle());
        g.drawLine (12.0f, separatorY, (float) getWidth() - 12.0f, separatorY, 1.0f);
    }

    // Draw each chip
    for (int i = 0; i < (int) cards.size(); ++i)
    {
        auto& card = cards[(size_t) i];
        auto& r = card.bounds;
        bool isSelected = (card.key.name == selectedKeyName);
        bool isHovered = (i == hoveredCardIndex);

        // Chip background
        if (isSelected)
            g.setColour (Theme::accent().withAlpha (0.15f));
        else if (isHovered)
            g.setColour (juce::Colour (0xff222238));
        else
            g.setColour (Theme::cardBg());
        g.fillRoundedRectangle (r, chipRadius);

        // Chip border
        if (isSelected)
            g.setColour (Theme::accent());
        else if (isHovered)
            g.setColour (Theme::accent().withAlpha (0.6f));
        else
            g.setColour (Theme::borderFaint());
        g.drawRoundedRectangle (r.reduced (0.5f), chipRadius, 1.0f);

        // Chip text
        g.setColour (isSelected ? Theme::accent() : Theme::textPrimary());
        g.setFont (juce::FontOptions (chipFontSize, isSelected ? juce::Font::bold : juce::Font::plain));
        g.drawText (card.chipText, r, juce::Justification::centred);
    }
}

int ScaleResultsPanel::getCardAtPosition (juce::Point<float> pos) const
{
    for (int i = 0; i < (int) cards.size(); ++i)
        if (cards[(size_t) i].bounds.contains (pos))
            return i;
    return -1;
}

void ScaleResultsPanel::mouseDown (const juce::MouseEvent& e)
{
    int idx = getCardAtPosition (e.position);
    if (idx >= 0 && idx < (int) cards.size())
    {
        if (onCardClicked)
            onCardClicked (cards[(size_t) idx].key.name);
    }
}

void ScaleResultsPanel::mouseMove (const juce::MouseEvent& e)
{
    int newHover = getCardAtPosition (e.position);

    if (newHover != hoveredCardIndex)
    {
        hoveredCardIndex = newHover;
        repaint();
    }
}

void ScaleResultsPanel::mouseExit (const juce::MouseEvent&)
{
    if (hoveredCardIndex != -1)
    {
        hoveredCardIndex = -1;
        repaint();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// KeyGridPopup
// ═══════════════════════════════════════════════════════════════════════════

KeyGridPopup::KeyGridPopup()
{
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    buildCells();
}

void KeyGridPopup::buildCells()
{
    cells.clear();

    auto allKeys = MusicTheory::allKeys();  // 0..11 = Major, 12..23 = Minor

    // Layout constants
    float padX = 12.0f, padY = 10.0f;
    float cellW = 66.0f, cellH = 28.0f;
    float cellGapX = 4.0f, cellGapY = 4.0f;
    float headerH = 22.0f;
    float sectionGap = 8.0f;
    int cols = 6;

    // Chromatic order mapping: indices into allKeys (root 0..11)
    // Row 1: C(0), C#(1), D(2), Eb(3), E(4), F(5)
    // Row 2: F#(6), G(7), Ab(8), A(9), Bb(10), B(11)

    float y = padY;

    // ── Major section ───────────────────────────────────────────────────
    y += headerH;  // space for "major" header

    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            int rootIdx = row * cols + col;  // 0..11
            auto& key = allKeys[(size_t) rootIdx];
            KeyCell cell;
            cell.internalName = key.name;
            cell.displayName = key.displayName;
            // Strip " Major" for compact display
            cell.displayName = cell.displayName.replace (" Major", "");
            cell.bounds = { padX + col * (cellW + cellGapX), y, cellW, cellH };
            cells.push_back (std::move (cell));
        }
        y += cellH + cellGapY;
    }

    y += sectionGap;

    // ── Minor section ───────────────────────────────────────────────────
    y += headerH;  // space for "minor" header

    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            int rootIdx = 12 + row * cols + col;  // 12..23
            auto& key = allKeys[(size_t) rootIdx];
            KeyCell cell;
            cell.internalName = key.name;
            cell.displayName = key.displayName;
            // Strip " Minor" and add "m" suffix for compact display
            cell.displayName = cell.displayName.replace (" Minor", "m");
            cell.bounds = { padX + col * (cellW + cellGapX), y, cellW, cellH };
            cells.push_back (std::move (cell));
        }
        y += cellH + cellGapY;
    }

    // Total size
    float totalW = padX * 2.0f + cols * cellW + (cols - 1) * cellGapX;
    float totalH = y + padY - cellGapY;
    setSize ((int) totalW, (int) totalH);
}

void KeyGridPopup::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background (near-solid with subtle transparency)
    g.setColour (juce::Colour (0xf91a1a2e));
    g.fillRoundedRectangle (bounds, 6.0f);

    // Hairline border
    g.setColour (juce::Colour (0x14ffffff));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    float padX = 12.0f, padY = 10.0f;
    float headerH = 22.0f;
    float cellH = 28.0f, cellGapY = 4.0f;
    float sectionGap = 8.0f;

    // Section headers
    g.setColour (juce::Colour (0xff8B90A0));
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText ("major", (int) padX, (int) padY, 100, (int) headerH, juce::Justification::centredLeft);

    float minorHeaderY = padY + headerH + 2 * (cellH + cellGapY) + sectionGap;
    g.drawText ("minor", (int) padX, (int) minorHeaderY, 100, (int) headerH, juce::Justification::centredLeft);

    // Draw cells
    for (int i = 0; i < (int) cells.size(); ++i)
    {
        auto& cell = cells[(size_t) i];
        bool isSelected = (cell.internalName == selectedKeyName);
        bool isHovered = (i == hoveredIndex);

        // Cell background
        if (isSelected)
        {
            g.setColour (juce::Colour (0xff252540));
            g.fillRoundedRectangle (cell.bounds, 4.0f);
            g.setColour (juce::Colour (0xff8b5cf6));
            g.drawRoundedRectangle (cell.bounds.reduced (0.5f), 4.0f, 1.0f);
        }
        else if (isHovered)
        {
            g.setColour (juce::Colour (0xff222238));
            g.fillRoundedRectangle (cell.bounds, 4.0f);
            g.setColour (juce::Colour (0xff8b5cf6));
            g.drawRoundedRectangle (cell.bounds.reduced (0.5f), 4.0f, 1.0f);
        }

        // Cell text
        g.setColour (isSelected ? juce::Colour (0xff6366f1) : juce::Colour (0xffE8EAF0));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (cell.displayName, cell.bounds, juce::Justification::centred);
    }
}

int KeyGridPopup::getCellAtPosition (juce::Point<float> pos) const
{
    for (int i = 0; i < (int) cells.size(); ++i)
        if (cells[(size_t) i].bounds.contains (pos))
            return i;
    return -1;
}

void KeyGridPopup::mouseDown (const juce::MouseEvent& e)
{
    int idx = getCellAtPosition (e.position);
    if (idx >= 0 && idx < (int) cells.size())
    {
        if (onKeySelected)
            onKeySelected (cells[(size_t) idx].internalName);
    }
}

void KeyGridPopup::mouseMove (const juce::MouseEvent& e)
{
    int newHover = getCellAtPosition (e.position);
    if (newHover != hoveredIndex)
    {
        hoveredIndex = newHover;
        repaint();
    }
}

void KeyGridPopup::mouseExit (const juce::MouseEvent&)
{
    if (hoveredIndex != -1)
    {
        hoveredIndex = -1;
        repaint();
    }
}

void KeyGridPopup::setSelectedKey (const juce::String& keyName)
{
    if (selectedKeyName != keyName)
    {
        selectedKeyName = keyName;
        repaint();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// OptionsPopup
// ═══════════════════════════════════════════════════════════════════════════

OptionsPopup::OptionsPopup()
{
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    buildItems();
}

void OptionsPopup::buildItems()
{
    items.clear();

    float padX = 8.0f, padY = 8.0f;
    float itemH = 32.0f;
    float sepH = 9.0f;
    float itemW = 204.0f;
    float y = padY;

    auto addItem = [&] (const juce::String& text, int id) {
        items.push_back ({ text, id, { padX, y, itemW, itemH } });
        y += itemH;
    };

    auto addSep = [&] () {
        items.push_back ({ "", 0, { padX, y, itemW, sepH } });
        y += sepH;
    };

    addItem ("Audio/MIDI Settings...", 1);
    addSep();
    addItem ("Save current state...", 2);
    addItem ("Load a saved state...", 3);
    addSep();
    addItem ("Reset to default state", 4);

    float totalW = padX * 2.0f + itemW;
    float totalH = y + padY;
    setSize ((int) totalW, (int) totalH);
}

void OptionsPopup::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xf91a1a2e));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (juce::Colour (0x14ffffff));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    for (int i = 0; i < (int) items.size(); ++i)
    {
        auto& item = items[(size_t) i];

        if (item.itemId == 0)
        {
            auto sepArea = item.bounds.withSizeKeepingCentre (item.bounds.getWidth() - 8.0f, 1.0f);
            g.setColour (juce::Colour (0x14ffffff));
            g.fillRect (sepArea);
            continue;
        }

        bool isHovered = (i == hoveredIndex);

        if (isHovered)
        {
            g.setColour (juce::Colour (0xff222238));
            g.fillRoundedRectangle (item.bounds.reduced (2.0f, 1.0f), 4.0f);
            g.setColour (juce::Colour (0xff8b5cf6));
            g.drawRoundedRectangle (item.bounds.reduced (2.5f, 1.5f), 4.0f, 1.0f);
        }

        g.setColour (juce::Colour (0xffE8EAF0));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText (item.text, item.bounds.reduced (12.0f, 0.0f),
                    juce::Justification::centredLeft);
    }
}

int OptionsPopup::getItemAtPosition (juce::Point<float> pos) const
{
    for (int i = 0; i < (int) items.size(); ++i)
        if (items[(size_t) i].itemId != 0 && items[(size_t) i].bounds.contains (pos))
            return i;
    return -1;
}

void OptionsPopup::mouseDown (const juce::MouseEvent& e)
{
    int idx = getItemAtPosition (e.position);
    if (idx >= 0 && idx < (int) items.size())
    {
        if (onItemSelected)
            onItemSelected (items[(size_t) idx].itemId);
    }
}

void OptionsPopup::mouseMove (const juce::MouseEvent& e)
{
    int newHover = getItemAtPosition (e.position);
    if (newHover != hoveredIndex)
    {
        hoveredIndex = newHover;
        repaint();
    }
}

void OptionsPopup::mouseExit (const juce::MouseEvent&)
{
    if (hoveredIndex != -1)
    {
        hoveredIndex = -1;
        repaint();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ChordsDisplay (leaf component — status messages + chord card)
// ═══════════════════════════════════════════════════════════════════════════

void ChordsDisplay::setChords (const std::vector<ChordInfo>& c, const juce::String& key)
{
    chordList = c;
    keyName = key;
    repaint();
}

void ChordsDisplay::setStatus (const juce::String& status, const juce::String& rStatus)
{
    statusText = status;
    resultStatus = rStatus;
    repaint();
}

void ChordsDisplay::clear()
{
    chordList.clear();
    keyName = "";
    statusText = "";
    resultStatus = "";
    repaint();
}

void ChordsDisplay::paint (juce::Graphics& g)
{
    int margin = 16;
    int controlsBottom = 4 + 28 + 16 + 280 + 16 + 32;
    int resultsStartY = controlsBottom + 8;

    // ── Status area (non-"some" states) ──────────────────────────────────
    if (resultStatus == "all-visible")
    {
        bool hov = emptyStateHovered;
        float w_f = (float) getWidth();
        float radius = 8.0f;
        auto emptyArea = juce::Rectangle<float> ((float) margin, (float) resultsStartY + 8.0f,
                                                  w_f - margin * 2.0f, 100.0f);

        // Card fill — subtle lift on hover
        g.setColour (juce::Colour (hov ? 0x0cffffff : 0x06ffffff));
        g.fillRoundedRectangle (emptyArea, radius);

        // Border — faint accent tint on hover
        g.setColour (hov ? Theme::accent().withAlpha (0.18f) : Theme::borderGhost());
        g.drawRoundedRectangle (emptyArea.reduced (0.5f), radius, 1.0f);

        // Music note icon
        g.setColour (hov ? Theme::textSecondary() : Theme::textMuted());
        g.setFont (juce::FontOptions (20.0f));
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x99\xab"),
                    emptyArea.withHeight (24.0f).translated (0.0f, 12.0f),
                    juce::Justification::centred);

        // Main text
        g.setColour (hov ? Theme::textPrimary().withAlpha (0.7f) : Theme::textSecondary());
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Play notes or drop an audio file",
                    emptyArea.withHeight (18.0f).translated (0.0f, 40.0f),
                    juce::Justification::centred);

        // Browse link
        g.setColour (hov ? Theme::accent().withAlpha (0.9f) : Theme::accent().withAlpha (0.5f));
        g.setFont (juce::FontOptions (10.5f));
        g.drawText ("or click to browse",
                    emptyArea.withHeight (16.0f).translated (0.0f, 58.0f),
                    juce::Justification::centred);

        // File formats
        g.setColour (hov ? Theme::textMuted().brighter (0.3f) : Theme::textMuted());
        g.setFont (juce::FontOptions (8.5f)
            .withName (juce::Font::getDefaultMonospacedFontName()));
        g.drawText (".wav  .mp3  .aiff  .flac",
                    emptyArea.withHeight (14.0f).translated (0.0f, 76.0f),
                    juce::Justification::centred);
    }
    else if (resultStatus == "none")
    {
        g.setColour (Theme::textSecondary());
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("No matching keys found",
                    30, resultsStartY + 30, getWidth() - 60, 24,
                    juce::Justification::centred);
    }
    else if (resultStatus == "all")
    {
        g.setColour (Theme::textSecondary());
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("All 12 notes selected (chromatic)",
                    30, resultsStartY + 30, getWidth() - 60, 24,
                    juce::Justification::centred);
    }

    // Chord card removed — degrees are not shown in the plugin UI

    // ── Analysis status text ─────────────────────────────────────────────
    if (statusText.isNotEmpty())
    {
        g.setColour (Theme::accent());
        g.setFont (juce::FontOptions (14.0f));
        g.drawText (statusText,
                    30, resultsStartY + 30, getWidth() - 60, 24,
                    juce::Justification::centred);
    }

    // ── "Also try:" label for alternative keys ───────────────────────────
    if (altKeysVisible)
    {
        g.setColour (Theme::textSecondary());
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("Also try:", margin, altKeyY, 52, altKeyH,
                    juce::Justification::centredLeft);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// DragOverlay (leaf component — file-drag visual)
// ═══════════════════════════════════════════════════════════════════════════

void DragOverlay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dim background
    g.setColour (juce::Colour (0xe00f0a1a));
    g.fillRect (bounds);

    // Drop zone box
    auto dropZone = bounds.reduced (30.0f, 120.0f);
    float dzRadius = 8.0f;

    g.setColour (juce::Colour (0xff1e1b30));
    g.fillRoundedRectangle (dropZone, dzRadius);

    // Dashed border
    {
        float dashLen = 8.0f;
        float gapLen = 6.0f;
        juce::Path dashedPath;

        juce::Path roundedRect;
        roundedRect.addRoundedRectangle (dropZone.reduced (1.0f), dzRadius - 1.0f);

        juce::PathFlatteningIterator iter (roundedRect);
        float dist = 0.0f;
        bool drawing = true;

        juce::Point<float> prev;
        bool first = true;

        while (iter.next())
        {
            juce::Point<float> pt ((float) iter.x2, (float) iter.y2);
            if (first)
            {
                prev = pt;
                first = false;
                dashedPath.startNewSubPath (pt);
                continue;
            }

            float dx = pt.x - prev.x;
            float dy = pt.y - prev.y;
            float segLen = std::sqrt (dx * dx + dy * dy);
            dist += segLen;

            float threshold = drawing ? dashLen : gapLen;
            if (dist >= threshold)
            {
                dist = 0.0f;
                drawing = ! drawing;
                if (drawing)
                    dashedPath.startNewSubPath (pt);
            }

            if (drawing)
                dashedPath.lineTo (pt);

            prev = pt;
        }

        g.setColour (Theme::accent().withAlpha (0.7f));
        g.strokePath (dashedPath, juce::PathStrokeType (1.5f));
    }

    // File icon with + badge
    float iconCentreX = dropZone.getCentreX();
    float iconCentreY = dropZone.getCentreY() - 30.0f;

    float iconW = 36.0f, iconH = 44.0f;
    auto iconRect = juce::Rectangle<float> (iconCentreX - iconW / 2.0f,
                                              iconCentreY - iconH / 2.0f,
                                              iconW, iconH);
    g.setColour (juce::Colour (0xff3f3f56));
    g.fillRoundedRectangle (iconRect, 4.0f);

    // Folded corner
    {
        float foldSize = 10.0f;
        juce::Path fold;
        fold.startNewSubPath (iconRect.getRight() - foldSize, iconRect.getY());
        fold.lineTo (iconRect.getRight(), iconRect.getY() + foldSize);
        fold.lineTo (iconRect.getRight() - foldSize, iconRect.getY() + foldSize);
        fold.closeSubPath();
        g.setColour (juce::Colour (0xff2a2a3e));
        g.fillPath (fold);
    }

    // Music note symbol
    g.setColour (juce::Colour (0xff9090b0));
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x99\xab"),
                iconRect.translated (0.0f, 4.0f), juce::Justification::centred);

    // + badge
    float badgeSize = 18.0f;
    auto badgeCentre = juce::Point<float> (iconRect.getRight() - 2.0f,
                                            iconRect.getBottom() - 2.0f);
    g.setColour (Theme::accent());
    g.fillEllipse (badgeCentre.x - badgeSize / 2.0f, badgeCentre.y - badgeSize / 2.0f,
                    badgeSize, badgeSize);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("+", (int) (badgeCentre.x - badgeSize / 2.0f),
                (int) (badgeCentre.y - badgeSize / 2.0f),
                (int) badgeSize, (int) badgeSize, juce::Justification::centred);

    // Text
    float textY = iconCentreY + iconH / 2.0f + 16.0f;

    g.setColour (Theme::textPrimary());
    g.setFont (juce::FontOptions (14.0f));
    g.drawText ("drop audio file to analyze",
                (int) dropZone.getX(), (int) textY,
                (int) dropZone.getWidth(), 22,
                juce::Justification::centred);

    g.setColour (Theme::textSecondary());
    g.setFont (juce::FontOptions (10.0f)
        .withName (juce::Font::getDefaultMonospacedFontName()));
    g.drawText (".wav   .mp3   .aiff   .flac",
                (int) dropZone.getX(), (int) textY + 24,
                (int) dropZone.getWidth(), 18,
                juce::Justification::centred);
}

// ═══════════════════════════════════════════════════════════════════════════
// ScaleFinderEditor
// ═══════════════════════════════════════════════════════════════════════════

ScaleFinderEditor::ScaleFinderEditor (ScaleFinderProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), pianoKeyboard (p)
{
    setSize (460, 480);

    // ── Tooltip styling ───────────────────────────────────────────────
    tooltipWindow.setColour (juce::TooltipWindow::backgroundColourId, Theme::cardBg());
    tooltipWindow.setColour (juce::TooltipWindow::textColourId, Theme::textSecondary());
    tooltipWindow.setColour (juce::TooltipWindow::outlineColourId, Theme::borderSubtle());

    // ── Leaf paint components (behind everything / on top of everything) ──
    chordsDisplay.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (chordsDisplay);

    // ── Title labels (lowercase, elegant, quiet) ─────────────────────
    titleLabel1.setText ("scalefinder", juce::dontSendNotification);
    titleLabel1.setFont (juce::FontOptions (20.0f));
    titleLabel1.setColour (juce::Label::textColourId, Theme::accentPurple());
    titleLabel1.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (titleLabel1);

    titleLabel2.setText ("studio", juce::dontSendNotification);
    titleLabel2.setFont (juce::FontOptions (20.0f));
    titleLabel2.setColour (juce::Label::textColourId, Theme::textMuted());
    titleLabel2.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel2);

    // ── Piano ─────────────────────────────────────────────────────────
    addAndMakeVisible (pianoKeyboard);

    // ── Reset button (ghost style — transparent, hairline border) ─────
    resetButton.setLookAndFeel (&resetButtonLF);
    resetButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x00000000));
    resetButton.setColour (juce::TextButton::textColourOffId, Theme::textSecondary());
    resetButton.onClick = [this]() {
        processorRef.clearNotes();
        pianoKeyboard.clearSelection();
        keyDropdown.setButtonText ("select key...");
        dismissKeyGridPopup();
        currentAlternatives.clear();
        altKeyButton1.setVisible (false);
        altKeyButton2.setVisible (false);
        updateUI();
    };
    addAndMakeVisible (resetButton);

    // ── Key dropdown button (opens grid popup) ─────────────────────
    keyDropdown.setLookAndFeel (&dropdownLF);
    keyDropdown.setColour (juce::TextButton::buttonColourId, juce::Colour (0x00000000));
    keyDropdown.setColour (juce::TextButton::textColourOffId, Theme::textSecondary());
    keyDropdown.onClick = [this]() { showKeyGridPopup(); };
    addAndMakeVisible (keyDropdown);

    // ── Results panel inside scrollable viewport ──────────────────────
    resultsPanel.onCardClicked = [this] (const juce::String& keyName) { onKeyButtonClicked (keyName); };
    resultsViewport.setViewedComponent (&resultsPanel, false);
    resultsViewport.setScrollBarsShown (true, false);
    resultsViewport.setScrollBarThickness (6);
    resultsViewport.getVerticalScrollBar().setColour (juce::ScrollBar::thumbColourId, juce::Colour (0x40ffffff));
    addAndMakeVisible (resultsViewport);

    // ── Alternative key suggestion buttons ──────────────────────────────
    auto setupAltButton = [this] (juce::TextButton& btn, int index) {
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0x00000000));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0x00000000));
        btn.setColour (juce::TextButton::textColourOffId, Theme::accent());
        btn.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x30ffffff));
        btn.onClick = [this, index]() { applyAlternativeKey (index); };
        addChildComponent (btn);  // hidden by default (only shown after audio analysis)
    };
    setupAltButton (altKeyButton1, 0);
    setupAltButton (altKeyButton2, 1);

    // ── Browse button (invisible overlay, covers the empty state area) ─────
    browseButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x00000000));
    browseButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0x00000000));
    browseButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0x00000000));
    browseButton.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x00000000));
    browseButton.setLookAndFeel (&invisibleButtonLF);
    browseButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    browseButton.onClick = [this]() { openFileBrowser(); };
    browseButton.onStateChange = [this]()
    {
        bool hovered = browseButton.isOver() || browseButton.isDown();
        if (chordsDisplay.emptyStateHovered != hovered)
        {
            chordsDisplay.emptyStateHovered = hovered;
            chordsDisplay.repaint();
        }
    };
    addAndMakeVisible (browseButton);

    // ── Browse icon button (always-visible shortcut to file browser) ─────
    browseIconButton.setLookAndFeel (&browseIconLF);
    browseIconButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x00000000));
    browseIconButton.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x14ffffff));
    browseIconButton.setTooltip ("Open audio file");
    browseIconButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    browseIconButton.onClick = [this]() { openFileBrowser(); };
    addAndMakeVisible (browseIconButton);

    // Drag overlay (on top of everything, initially hidden)
    dragOverlay.setInterceptsMouseClicks (false, false);
    addChildComponent (dragOverlay);

    // Listen for mouse clicks on children so we can dismiss the key grid popup
    pianoKeyboard.addMouseListener (this, false);
    resultsViewport.addMouseListener (this, false);
    resetButton.addMouseListener (this, false);
    browseIconButton.addMouseListener (this, false);

    updateUI();
    startTimerHz (30);
}

ScaleFinderEditor::~ScaleFinderEditor()
{
    stopTimer();

    if (previousDefaultLF != nullptr)
        juce::LookAndFeel::setDefaultLookAndFeel (previousDefaultLF);

    pianoKeyboard.removeMouseListener (this);
    resultsViewport.removeMouseListener (this);
    resetButton.removeMouseListener (this);
    browseIconButton.removeMouseListener (this);
    resetButton.setLookAndFeel (nullptr);
    keyDropdown.setLookAndFeel (nullptr);
    browseButton.setLookAndFeel (nullptr);
    browseIconButton.setLookAndFeel (nullptr);
    dismissKeyGridPopup();
    dismissOptionsPopup();

    if (auto* window = dynamic_cast<juce::DocumentWindow*> (getTopLevelComponent()))
    {
        window->removeKeyListener (this);

        if (optionsButtonReplacement != nullptr)
        {
            optionsButtonReplacement->setLookAndFeel (nullptr);
            static_cast<juce::Component*> (window)->removeChildComponent (optionsButtonReplacement);
            delete optionsButtonReplacement;
            optionsButtonReplacement = nullptr;
        }

        if (keyboardToggleButton != nullptr)
        {
            keyboardToggleButton->setLookAndFeel (nullptr);
            static_cast<juce::Component*> (window)->removeChildComponent (keyboardToggleButton);
            delete keyboardToggleButton;
            keyboardToggleButton = nullptr;
        }

        for (int i = 0; i < window->getNumChildComponents(); ++i)
            if (auto* btn = dynamic_cast<juce::TextButton*> (window->getChildComponent (i)))
                btn->setLookAndFeel (nullptr);
    }
}

void ScaleFinderEditor::parentHierarchyChanged()
{
    if (auto* window = dynamic_cast<juce::DocumentWindow*> (getTopLevelComponent()))
    {
        // Set app-wide menu LookAndFeel (once, standalone only)
        if (previousDefaultLF == nullptr)
        {
            previousDefaultLF = &juce::LookAndFeel::getDefaultLookAndFeel();
            juce::LookAndFeel::setDefaultLookAndFeel (&appMenuLF);
        }

        window->setName (" ");
        window->setBackgroundColour (Theme::bgTop());

        // Match title bar to our theme via existing LookAndFeel colour scheme
        auto& lf = window->getLookAndFeel();
        if (auto* v4 = dynamic_cast<juce::LookAndFeel_V4*> (&lf))
        {
            auto scheme = v4->getCurrentColourScheme();
            scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::widgetBackground, Theme::bgTop());
            scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::windowBackground, Theme::bgTop());
            scheme.setUIColour (juce::LookAndFeel_V4::ColourScheme::outline, Theme::bgTop());
            v4->setColourScheme (scheme);
        }

        // Replace default close/minimize buttons with purple-themed ones (once)
        bool alreadyReplaced = false;
        for (int i = 0; i < window->getNumChildComponents(); ++i)
            if (auto* btn = dynamic_cast<PurpleWindowButton*> (window->getChildComponent (i)))
                { alreadyReplaced = true; break; }

        if (alreadyReplaced) return;

        juce::Array<juce::Button*> oldBtns;
        for (int i = 0; i < window->getNumChildComponents(); ++i)
        {
            auto* child = window->getChildComponent (i);
            if (dynamic_cast<juce::TextButton*> (child) == nullptr)
                if (auto* btn = dynamic_cast<juce::Button*> (child))
                    oldBtns.add (btn);
        }

        for (auto* old : oldBtns)
        {
            juce::Path shape;
            float t = 0.15f;
            juce::Colour purple (0xff8b5cf6);
            PurpleWindowButton* replacement = nullptr;

            if (old->getName() == "close")
            {
                shape.addLineSegment ({ 0.0f, 0.0f, 1.0f, 1.0f }, t);
                shape.addLineSegment ({ 1.0f, 0.0f, 0.0f, 1.0f }, t);
                replacement = new PurpleWindowButton ("close", purple, shape, shape);
            }
            else if (old->getName() == "minimise")
            {
                shape.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, t);
                replacement = new PurpleWindowButton ("minimise", purple.withAlpha (0.6f), shape, shape);
            }

            if (replacement != nullptr)
            {
                replacement->setBounds (old->getBounds());
                replacement->onClick = [old]() { old->triggerClick(); };
                old->setVisible (false);
                static_cast<juce::Component*> (window)->addAndMakeVisible (replacement);

                // Cache for efficient repositioning in timerCallback
                if (replacement->getName() == "close")    cachedCloseBtn = replacement;
                if (replacement->getName() == "minimise") cachedMinimiseBtn = replacement;
            }
        }

        titleBarButtonsCached = true;

        // Hide the original Options TextButton (its listener shows a poorly-positioned popup)
        for (int i = 0; i < window->getNumChildComponents(); ++i)
        {
            if (auto* btn = dynamic_cast<juce::TextButton*> (window->getChildComponent (i)))
            {
                if (btn->getName() == "keyboardToggle" || btn->getName() == "optionsReplacement")
                    continue;
                btn->setVisible (false);
            }
        }

        // Create our own options button with properly-positioned popup (once)
        if (optionsButtonReplacement == nullptr)
        {
            auto* btn = new juce::TextButton ("optionsReplacement");
            btn->setButtonText ("");
            btn->setTooltip ("Options");
            btn->setLookAndFeel (&optionsIconLF);
            btn->onClick = [this]() { showOptionsPopup(); };
            static_cast<juce::Component*> (window)->addAndMakeVisible (btn);
            optionsButtonReplacement = btn;
        }

        // Create computer keyboard toggle button (once)
        if (keyboardToggleButton == nullptr)
        {
            auto* btn = new juce::TextButton ("keyboardToggle");
            btn->setButtonText ("");
            btn->setTooltip ("Disable computer keyboard");
            btn->setLookAndFeel (&keyboardIconLF);
            btn->onClick = [this]()
            {
                computerKeyboardEnabled = ! computerKeyboardEnabled;
                keyboardIconLF.isEnabled = computerKeyboardEnabled;

                if (! computerKeyboardEnabled)
                {
                    // Release all held keyboard notes
                    for (int note : pressedKeyboardNotes)
                        processorRef.triggerNoteOff (note);
                    pressedKeyboardNotes.clear();
                }

                if (keyboardToggleButton != nullptr)
                {
                    keyboardToggleButton->setTooltip (computerKeyboardEnabled
                        ? "Disable computer keyboard" : "Enable computer keyboard");
                    keyboardToggleButton->repaint();
                }
            };
            static_cast<juce::Component*> (window)->addAndMakeVisible (btn);
            keyboardToggleButton = btn;

            // Register as KeyListener on the window
            window->addKeyListener (this);
        }

        // Lock window size (once, after all buttons are created)
        if (! windowSizeConfigured)
        {
            windowSizeConfigured = true;
            window->setResizable (false, false);
            window->setContentComponentSize (460, 520);

            // Re-hide any default buttons that reappeared after setResizable
            for (int i = 0; i < window->getNumChildComponents(); ++i)
            {
                auto* child = window->getChildComponent (i);
                if (dynamic_cast<PurpleWindowButton*> (child) != nullptr) continue;
                if (dynamic_cast<juce::TextButton*> (child) != nullptr)
                {
                    if (child->getName() == "keyboardToggle" || child->getName() == "optionsReplacement")
                        continue;
                }
                if (auto* btn = dynamic_cast<juce::Button*> (child))
                    if (btn->getName() == "close" || btn->getName() == "minimise" || btn->getName() == "maximise")
                        btn->setVisible (false);
            }
        }
    }
}

void ScaleFinderEditor::paint (juce::Graphics& g)
{
    // Gradient background only — all content is in child components
    g.setGradientFill (juce::ColourGradient (Theme::bgTop(), 0.0f, 0.0f,
                                              Theme::bgBottom(), 0.0f, (float) getHeight(),
                                              false));
    g.fillAll();
}

void ScaleFinderEditor::paintOverChildren (juce::Graphics& g)
{
    // Draw focus ring on whichever child component has keyboard focus
    auto drawFocusRing = [&] (juce::Component& comp, float radius)
    {
        if (comp.hasKeyboardFocus (true))
        {
            auto focusBounds = comp.getBounds().toFloat().expanded (2.0f);
            g.setColour (Theme::accent().withAlpha (0.6f));
            g.drawRoundedRectangle (focusBounds, radius + 2.0f, 2.0f);
        }
    };

    drawFocusRing (keyDropdown, 4.0f);
}

void ScaleFinderEditor::resized()
{
    int w = getWidth();
    int margin = 16;

    // ── Title bar (branding, centered with tight gap) ───────────────
    int titleY = 4;
    int titleH = 28;
    int halfGap = 2;  // 4px total gap between words
    int halfW = w / 2;
    titleLabel1.setBounds (halfW - 130, titleY, 130 - halfGap, titleH);
    titleLabel2.setBounds (halfW + halfGap, titleY, 100, titleH);

    // ── Piano keyboard (DOMINANT — ~45% of height) ───────────────────
    int pianoY = titleY + titleH + 16;
    int pianoH = 280;
    pianoKeyboard.setBounds (margin, pianoY, w - margin * 2, pianoH);

    // ── Controls row (secondary — compact, side by side) ─────────────
    int controlsY = pianoY + pianoH + 12;
    int controlsH = 32;
    int resetW = 72;
    int browseW = 32;
    int gap = 8;
    int dropdownW = w - margin * 2 - browseW - resetW - gap * 2;

    keyDropdown.setBounds (margin, controlsY, dropdownW, controlsH);
    browseIconButton.setBounds (margin + dropdownW + gap, controlsY, browseW, controlsH);
    resetButton.setBounds (margin + dropdownW + gap + browseW + gap, controlsY, resetW, controlsH);

    // ── Results viewport (scrollable card list) ───────────────────────
    int resultsY = controlsY + controlsH + 12;
    bool hasAlts = altKeyButton1.isVisible();
    int altRowH = hasAlts ? 32 : 0;
    int resultsH = getHeight() - resultsY - altRowH - 8;
    resultsViewport.setBounds (margin, resultsY, w - margin * 2, juce::jmax (resultsH, 60));
    resultsPanel.setSize (resultsViewport.getWidth() - (resultsViewport.isVerticalScrollBarShown() ? 6 : 0),
                          resultsPanel.getHeight());

    // ── Browse button (covers the empty state area) ──────────────────
    browseButton.setBounds (margin, resultsY + 8, w - margin * 2, 100);

    // ── Alternative key buttons (below results) ──────────────────────
    if (hasAlts)
    {
        int altY = resultsViewport.getBottom() + 4;
        int labelW = 52;   // "Also try:" label width (drawn by ChordsDisplay)
        int btnW = (w - margin * 2 - labelW - 8 - 8) / 2;  // two buttons, 8px gaps
        int btnH = 24;
        altKeyButton1.setBounds (margin + labelW + 8, altY + 4, btnW, btnH);
        altKeyButton2.setBounds (margin + labelW + 8 + btnW + 8, altY + 4, btnW, btnH);
    }

    // ── Leaf paint components (full-editor sized, coordinate system matches) ──
    chordsDisplay.setBounds (getLocalBounds());
    chordsDisplay.viewportBottom = resultsViewport.getBottom();
    chordsDisplay.altKeysVisible = hasAlts;
    if (hasAlts)
    {
        chordsDisplay.altKeyY = altKeyButton1.getY();
        chordsDisplay.altKeyH = altKeyButton1.getHeight();
    }
    dragOverlay.setBounds (getLocalBounds());
}

void ScaleFinderEditor::timerCallback()
{
    if (processorRef.needsUIUpdate.exchange (false))
    {
        // Recompute key results on UI thread (may have been triggered by
        // audio thread's atomic bitmask update from external MIDI)
        processorRef.recomputeResult();
        updateUI();
    }

    // Check if audio analysis finished
    if (audioAnalyzer.isAnalysisComplete())
    {
        auto pitchClasses = audioAnalyzer.getDetectedPitchClasses();
        if (! pitchClasses.empty())
        {
            processorRef.setAccumulatedNotes (pitchClasses);
            currentAlternatives = audioAnalyzer.getAlternativeKeys();
            analysisStatusText = "";

            // Update alt button labels
            if (currentAlternatives.size() >= 1)
            {
                altKeyButton1.setButtonText (MusicTheory::getKeyDisplayName (currentAlternatives[0].name));
                altKeyButton1.setVisible (true);
            }
            if (currentAlternatives.size() >= 2)
            {
                altKeyButton2.setButtonText (MusicTheory::getKeyDisplayName (currentAlternatives[1].name));
                altKeyButton2.setVisible (true);
            }
        }
        else
        {
            analysisStatusText = "No pitches detected";
            currentAlternatives.clear();
            altKeyButton1.setVisible (false);
            altKeyButton2.setVisible (false);
        }
        updateUI();
    }

    // Update reset button outline: purple when hovered or focused
    {
        bool active = resetButton.isOver() || resetButton.hasKeyboardFocus (false);
        auto col = active ? Theme::accent() : juce::Colour (0x14ffffff);
        if (resetButton.findColour (juce::ComboBox::outlineColourId) != col)
        {
            resetButton.setColour (juce::ComboBox::outlineColourId, col);
            resetButton.repaint();
        }
    }

    // Update dropdown outline + text: purple when a key is selected or hovered
    {
        bool hasKey = processorRef.selectedKey.isNotEmpty();
        bool hovered = keyDropdown.isOver() || keyDropdown.hasKeyboardFocus (false);
        auto outlineCol = (hasKey || hovered) ? Theme::accent() : juce::Colour (0x14ffffff);
        auto textCol = hasKey ? Theme::textPrimary() : Theme::textSecondary();
        if (keyDropdown.findColour (juce::ComboBox::outlineColourId) != outlineCol
            || keyDropdown.findColour (juce::TextButton::textColourOffId) != textCol)
        {
            keyDropdown.setColour (juce::ComboBox::outlineColourId, outlineCol);
            keyDropdown.setColour (juce::TextButton::textColourOffId, textCol);
            keyDropdown.repaint();
        }
    }

    // Update browse icon button outline: purple when hovered
    {
        bool active = browseIconButton.isOver() || browseIconButton.hasKeyboardFocus (false);
        auto col = active ? Theme::accent() : juce::Colour (0x14ffffff);
        if (browseIconButton.findColour (juce::ComboBox::outlineColourId) != col)
        {
            browseIconButton.setColour (juce::ComboBox::outlineColourId, col);
            browseIconButton.repaint();
        }
    }

    // Reposition title bar buttons and keep default ones hidden
    if (titleBarButtonsCached)
    {
        if (auto* window = dynamic_cast<juce::DocumentWindow*> (getTopLevelComponent()))
        {
            // Re-hide default buttons (StandaloneFilterWindow's resized() can make them visible again)
            for (int i = 0; i < window->getNumChildComponents(); ++i)
            {
                auto* child = window->getChildComponent (i);
                if (dynamic_cast<PurpleWindowButton*> (child) != nullptr) continue;
                if (auto* tb = dynamic_cast<juce::TextButton*> (child))
                {
                    if (tb->getName() == "keyboardToggle" || tb->getName() == "optionsReplacement")
                        continue;
                    tb->setVisible (false);
                }
                else if (auto* btn = dynamic_cast<juce::Button*> (child))
                {
                    if (btn == cachedCloseBtn || btn == cachedMinimiseBtn) continue;
                    btn->setVisible (false);
                }
            }

            int tbH = window->getTitleBarHeight();
            int btnW = tbH - tbH / 8;
            int btnY = (tbH - btnW) / 2;
            int leftX = 6;

            if (cachedCloseBtn != nullptr)
            {
                cachedCloseBtn->setBounds (leftX, btnY, btnW, btnW);
                leftX += btnW + 2;
            }
            if (cachedMinimiseBtn != nullptr)
            {
                cachedMinimiseBtn->setBounds (leftX, btnY, btnW, btnW);
            }

            // Position our custom title bar buttons: options (right), keyboard toggle (left of options)
            {
                int btnSize = tbH - 6;
                int optionsX = window->getWidth() - btnSize - 6;

                if (optionsButtonReplacement != nullptr)
                    optionsButtonReplacement->setBounds (optionsX, (tbH - btnSize) / 2, btnSize, btnSize);

                if (keyboardToggleButton != nullptr)
                {
                    int kbX = optionsX - btnSize - 4;
                    keyboardToggleButton->setBounds (kbX, (tbH - btnSize) / 2, btnSize, btnSize);
                }
            }
        }
    }
}

void ScaleFinderEditor::updateUI()
{
    auto accumulated = processorRef.getAccumulatedNotes();
    pianoKeyboard.setHighlightedNotes (accumulated);

    // Pass root note to piano when a key is selected
    if (processorRef.selectedKey.isNotEmpty())
    {
        auto allKeys = MusicTheory::allKeys();
        for (auto& k : allKeys)
        {
            if (k.name == processorRef.selectedKey)
            {
                pianoKeyboard.setRootNote (k.root);
                break;
            }
        }
    }
    else
    {
        pianoKeyboard.setRootNote (-1);
    }

    auto result = processorRef.getCurrentResult();

    // Update scale results cards
    if (result.status == "some")
    {
        resultsPanel.setResults (result.keys, (int) accumulated.size());
        resultsPanel.setSelectedKey (processorRef.selectedKey);
        resultsViewport.setVisible (true);
    }
    else
    {
        resultsPanel.setResults ({}, 0);
        resultsViewport.setVisible (false);
    }

    // Browse button only visible in empty state
    browseButton.setVisible (result.status == "all-visible");

    updateChordsDisplay();
    resized();  // Recalculate layout (chord card space depends on selection)
    repaint();
}

void ScaleFinderEditor::applyAlternativeKey (int index)
{
    if (index < 0 || index >= (int) currentAlternatives.size()) return;

    auto& alt = currentAlternatives[(size_t) index];
    processorRef.setAccumulatedNotes (alt.pitchClasses);
    currentAlternatives.clear();
    altKeyButton1.setVisible (false);
    altKeyButton2.setVisible (false);
    analysisStatusText = "";
    updateUI();
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
                pianoKeyboard.setRootNote (k.root);
                break;
            }
        }
    }
    updateUI();
}

void ScaleFinderEditor::updateChordsDisplay()
{
    auto result = processorRef.getCurrentResult();

    // Push data to ChordsDisplay child component
    chordsDisplay.setStatus (isDragOver ? "" : analysisStatusText, result.status);
    chordsDisplay.setChords (processorRef.currentChords, processorRef.selectedKey);
    chordsDisplay.viewportBottom = resultsViewport.getBottom();
    chordsDisplay.altKeysVisible = altKeyButton1.isVisible() && ! isDragOver;
    if (chordsDisplay.altKeysVisible)
    {
        chordsDisplay.altKeyY = altKeyButton1.getY();
        chordsDisplay.altKeyH = altKeyButton1.getHeight();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Key Grid Popup
// ═══════════════════════════════════════════════════════════════════════════

void ScaleFinderEditor::showKeyGridPopup()
{
    if (keyGridPopup != nullptr)
    {
        dismissKeyGridPopup();
        return;  // toggle off
    }

    keyGridPopup = std::make_unique<KeyGridPopup>();
    keyGridPopup->setSelectedKey (processorRef.selectedKey);
    keyGridPopup->onKeySelected = [this] (const juce::String& keyName)
    {
        onKeyButtonClicked (keyName);

        // Update dropdown display text
        keyDropdown.setButtonText (MusicTheory::getKeyDisplayName (keyName));

        dismissKeyGridPopup();
    };

    // Position below the dropdown
    auto ddBounds = keyDropdown.getBounds();
    int popupX = ddBounds.getX();
    int popupY = ddBounds.getBottom() + 4;

    // Clamp to stay inside the editor
    int popupW = keyGridPopup->getWidth();
    int popupH = keyGridPopup->getHeight();
    if (popupX + popupW > getWidth() - 8)
        popupX = getWidth() - 8 - popupW;
    if (popupY + popupH > getHeight() - 8)
        popupY = ddBounds.getY() - popupH - 4;  // flip above

    keyGridPopup->setBounds (popupX, popupY, popupW, popupH);
    addAndMakeVisible (*keyGridPopup);
    keyGridPopup->toFront (true);
}

void ScaleFinderEditor::dismissKeyGridPopup()
{
    if (keyGridPopup != nullptr)
    {
        removeChildComponent (keyGridPopup.get());
        keyGridPopup.reset();
    }
}

void ScaleFinderEditor::showOptionsPopup()
{
    if (optionsPopup != nullptr)
    {
        dismissOptionsPopup();
        return;  // toggle off
    }

    optionsMenuOpen = true;
    optionsIconLF.isActive = true;
    if (optionsButtonReplacement != nullptr)
    {
        optionsButtonReplacement->setTooltip ("");
        optionsButtonReplacement->repaint();
    }

    optionsPopup = std::make_unique<OptionsPopup>();
    optionsPopup->onItemSelected = [this] (int itemId)
    {
        dismissOptionsPopup();
        if (itemId == 0) return;

       #if JucePlugin_Build_Standalone
        if (auto* sfw = dynamic_cast<juce::StandaloneFilterWindow*> (getTopLevelComponent()))
            sfw->handleMenuResult (itemId);
       #endif
    };

    // Position at top-right of editor, right-aligned
    int popupW = optionsPopup->getWidth();
    int popupH = optionsPopup->getHeight();
    int popupX = getWidth() - popupW - 8;
    int popupY = 4;

    if (popupX < 8) popupX = 8;
    if (popupY + popupH > getHeight() - 8)
        popupY = getHeight() - 8 - popupH;

    optionsPopup->setBounds (popupX, popupY, popupW, popupH);
    addAndMakeVisible (*optionsPopup);
    optionsPopup->toFront (true);
}

void ScaleFinderEditor::dismissOptionsPopup()
{
    if (optionsPopup != nullptr)
    {
        removeChildComponent (optionsPopup.get());
        optionsPopup.reset();
    }

    optionsMenuOpen = false;
    optionsIconLF.isActive = false;
    if (optionsButtonReplacement != nullptr)
    {
        optionsButtonReplacement->setTooltip ("Options");
        optionsButtonReplacement->repaint();
    }
}

void ScaleFinderEditor::mouseDown (const juce::MouseEvent& e)
{
    // Dismiss key grid popup if click is outside it
    if (keyGridPopup != nullptr)
    {
        auto localPos = keyGridPopup->getLocalPoint (this, e.position.toInt());
        if (! keyGridPopup->getLocalBounds().contains (localPos))
            dismissKeyGridPopup();
    }

    // Dismiss options popup if click is outside it
    if (optionsPopup != nullptr)
    {
        auto localPos = optionsPopup->getLocalPoint (this, e.position.toInt());
        if (! optionsPopup->getLocalBounds().contains (localPos))
            dismissOptionsPopup();
    }
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
    dragOverlay.setVisible (true);
    dragOverlay.toFront (false);
    updateChordsDisplay();
}

void ScaleFinderEditor::fileDragExit (const juce::StringArray&)
{
    isDragOver = false;
    dragOverlay.setVisible (false);
    updateChordsDisplay();
}

void ScaleFinderEditor::filesDropped (const juce::StringArray& files, int, int)
{
    isDragOver = false;
    dragOverlay.setVisible (false);
    if (files.isEmpty()) return;

    juce::File audioFile (files[0]);

    // Clear previous state
    processorRef.clearNotes();
    pianoKeyboard.clearSelection();
    keyDropdown.setButtonText ("select key...");

    analysisStatusText = "Analyzing...";
    updateChordsDisplay();

    audioAnalyzer.analyzeFile (audioFile, processorRef.getAnalysisSampleRate());
}

void ScaleFinderEditor::openFileBrowser()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Select an audio file...",
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        "*.wav;*.mp3;*.aiff;*.aif;*.flac");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result == juce::File{})
                return;  // User cancelled

            // Same logic as filesDropped
            processorRef.clearNotes();
            pianoKeyboard.clearSelection();
            keyDropdown.setButtonText ("select key...");

            analysisStatusText = "Analyzing...";
            updateChordsDisplay();

            audioAnalyzer.analyzeFile (result, processorRef.getAnalysisSampleRate());
        });
}

// ═══════════════════════════════════════════════════════════════════════════
// Computer Keyboard MIDI
// ═══════════════════════════════════════════════════════════════════════════

static int getKeyboardMidiNote (int keyCode)
{
    // Ableton-style: bottom row = white keys, top row = black keys
    switch (keyCode)
    {
        case 'A': return 60;  // C4
        case 'W': return 61;  // C#4
        case 'S': return 62;  // D4
        case 'E': return 63;  // D#4
        case 'D': return 64;  // E4
        case 'F': return 65;  // F4
        case 'T': return 66;  // F#4
        case 'G': return 67;  // G4
        case 'Y': return 68;  // G#4
        case 'H': return 69;  // A4
        case 'U': return 70;  // A#4
        case 'J': return 71;  // B4
        case 'K': return 72;  // C5
        default:  return -1;
    }
}

bool ScaleFinderEditor::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (! computerKeyboardEnabled)
        return false;

    int midiNote = getKeyboardMidiNote (key.getKeyCode());
    if (midiNote < 0)
        return false;

    // Consume auto-repeat — note already held
    if (pressedKeyboardNotes.count (midiNote))
        return true;

    pressedKeyboardNotes.insert (midiNote);

    int pc = midiNote % 12;
    bool isCurrentlySelected = processorRef.getAccumulatedNotes().count (pc) > 0;

    if (isCurrentlySelected)
        processorRef.togglePitchClassOff (pc);
    else
        processorRef.togglePitchClassOn (pc);

    processorRef.triggerNoteOn (midiNote, 0.8f);
    return true;
}

bool ScaleFinderEditor::keyStateChanged (bool /*isKeyDown*/, juce::Component*)
{
    if (! computerKeyboardEnabled)
        return false;

    bool consumed = false;
    std::vector<int> released;

    for (int note : pressedKeyboardNotes)
    {
        // Map MIDI note back to key code to check if still held
        int keyCode = 0;
        switch (note)
        {
            case 60: keyCode = 'A'; break;
            case 61: keyCode = 'W'; break;
            case 62: keyCode = 'S'; break;
            case 63: keyCode = 'E'; break;
            case 64: keyCode = 'D'; break;
            case 65: keyCode = 'F'; break;
            case 66: keyCode = 'T'; break;
            case 67: keyCode = 'G'; break;
            case 68: keyCode = 'Y'; break;
            case 69: keyCode = 'H'; break;
            case 70: keyCode = 'U'; break;
            case 71: keyCode = 'J'; break;
            case 72: keyCode = 'K'; break;
            default: break;
        }

        if (keyCode != 0 && ! juce::KeyPress::isKeyCurrentlyDown (keyCode))
        {
            released.push_back (note);
            consumed = true;
        }
    }

    for (int note : released)
    {
        pressedKeyboardNotes.erase (note);
        processorRef.triggerNoteOff (note);
    }

    return consumed;
}

