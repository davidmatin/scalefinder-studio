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

// ── Shared neumorphic pill background for controls row buttons ──────────
static void drawNeumorphicPill (juce::Graphics& g, juce::Rectangle<float> bounds,
                                 juce::Colour borderCol)
{
    float r = bounds.getHeight() * 0.5f;

    // Drop shadow (neumorphic depth)
    g.setColour (juce::Colour (0x20000000));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.5f).expanded (0.5f), r + 0.5f);
    g.setColour (juce::Colour (0x10000000));
    g.fillRoundedRectangle (bounds.translated (0.0f, 3.0f).expanded (1.0f), r + 1.0f);

    // Top-edge highlight
    g.setColour (juce::Colour (0x0affffff));
    g.drawRoundedRectangle (bounds.translated (0.0f, -0.5f), r, 0.5f);

    // Gradient fill
    juce::ColourGradient grad (juce::Colour (0xff242840), 0.0f, bounds.getY(),
                               juce::Colour (0xff1C2030), 0.0f, bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, r);

    // Border
    g.setColour (borderCol);
    g.drawRoundedRectangle (bounds, r, 0.75f);
}

void DropdownButtonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                        const juce::Colour&, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto borderCol = button.findColour (juce::ComboBox::outlineColourId);
    drawNeumorphicPill (g, bounds, borderCol);
}

void DropdownButtonLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                                  bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto textCol = button.findColour (juce::TextButton::textColourOffId);

    // Text (left-aligned with padding)
    g.setColour (textCol);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (button.getButtonText(), bounds.reduced (14.0f, 0.0f).withTrimmedRight (24.0f),
                juce::Justification::centredLeft);

    // Arrow (right side)
    float arrowX = bounds.getRight() - 20.0f;
    float arrowY = bounds.getCentreY();
    juce::Path arrow;
    arrow.addTriangle (arrowX - 3.0f, arrowY - 1.5f,
                       arrowX + 3.0f, arrowY - 1.5f,
                       arrowX, arrowY + 2.0f);
    g.setColour (textCol.withAlpha (0.5f));
    g.fillPath (arrow);
}

void ResetButtonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                     const juce::Colour&, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto borderCol = button.findColour (juce::ComboBox::outlineColourId);
    drawNeumorphicPill (g, bounds, borderCol);
}

void BrowseIconLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                     const juce::Colour&, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto borderCol = button.findColour (juce::ComboBox::outlineColourId);
    drawNeumorphicPill (g, bounds, borderCol);
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

// ── BPM pill LookAndFeel ─────────────────────────────────────────────────
void BpmPillLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                const juce::Colour&, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto borderCol = button.findColour (juce::ComboBox::outlineColourId);
    drawNeumorphicPill (g, bounds, borderCol);
}

void BpmPillLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                          bool, bool)
{
    auto textCol = button.findColour (juce::TextButton::textColourOffId);
    g.setColour (textCol);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (button.getButtonText(), button.getLocalBounds(),
                juce::Justification::centred);
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
// InstrumentButton
// ═══════════════════════════════════════════════════════════════════════════

InstrumentButton::InstrumentButton()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

constexpr const char* InstrumentButton::shortNames[];

void InstrumentButton::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    float r = bounds.getHeight() * 0.5f;  // full capsule radius

    // ── Drop shadow (neumorphic depth) ──────────────────────────────
    g.setColour (juce::Colour (0x20000000));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.5f).expanded (0.5f), r + 0.5f);
    g.setColour (juce::Colour (0x10000000));
    g.fillRoundedRectangle (bounds.translated (0.0f, 3.0f).expanded (1.0f), r + 1.0f);

    // ── Top-edge highlight ───────────────────────────────────────────
    g.setColour (juce::Colour (0x0affffff));
    g.drawRoundedRectangle (bounds.translated (0.0f, -0.5f), r, 0.5f);

    // ── Fill with gradient ───────────────────────────────────────────
    {
        juce::ColourGradient grad (juce::Colour (0xff242840), 0.0f, bounds.getY(),
                                   juce::Colour (0xff1C2030), 0.0f, bounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, r);
    }

    // ── Border ───────────────────────────────────────────────────────
    auto borderCol = (hovered || popupOpen) ? Theme::borderSubtle() : Theme::borderFaint();
    g.setColour (borderCol);
    g.drawRoundedRectangle (bounds, r, 0.75f);

    // ── Text color ───────────────────────────────────────────────────
    auto textCol = popupOpen ? Theme::accent()
                 : hovered   ? Theme::textPrimary()
                             : Theme::textSecondary();

    // ── Instrument name (left-aligned) ───────────────────────────────
    int idx = juce::jlimit (0, 3, selectedIndex);
    g.setColour (textCol);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (shortNames[idx],
                bounds.reduced (10.0f, 0.0f).withTrimmedRight (16.0f),
                juce::Justification::centredLeft);

    // ── Dropdown arrow (right side) ──────────────────────────────────
    float arrowX = bounds.getRight() - 12.0f;
    float arrowY = bounds.getCentreY();
    juce::Path arrow;
    arrow.addTriangle (arrowX - 3.0f, arrowY - 1.5f,
                       arrowX + 3.0f, arrowY - 1.5f,
                       arrowX, arrowY + 2.0f);
    g.setColour (popupOpen ? Theme::accent() : Theme::textMuted());
    g.fillPath (arrow);
}

void InstrumentButton::mouseDown (const juce::MouseEvent&)
{
    if (onClick) onClick();
}

void InstrumentButton::mouseEnter (const juce::MouseEvent&)
{
    hovered = true;
    repaint();
}

void InstrumentButton::mouseExit (const juce::MouseEvent&)
{
    hovered = false;
    repaint();
}

// ═══════════════════════════════════════════════════════════════════════════
// InstrumentPopup
// ═══════════════════════════════════════════════════════════════════════════

static const juce::String instrumentNames[] = { "Synth", "Live Piano", "E-Piano", "Guitar" };
static constexpr int numInstruments = 4;

InstrumentPopup::InstrumentPopup()
{
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    buildItems();
}

void InstrumentPopup::buildItems()
{
    items.clear();

    float padX = 8.0f, padY = 8.0f;
    float itemH = 32.0f;
    float itemW = 140.0f;
    float y = padY;

    for (int i = 0; i < numInstruments; ++i)
    {
        items.push_back ({ instrumentNames[i], i,
                           { padX, y, itemW, itemH } });
        y += itemH;
    }

    setSize ((int) (padX * 2.0f + itemW), (int) (y + padY));
}

int InstrumentPopup::getItemAtPosition (juce::Point<float> pos) const
{
    for (int i = 0; i < (int) items.size(); ++i)
        if (items[(size_t) i].bounds.contains (pos))
            return i;
    return -1;
}

void InstrumentPopup::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour (juce::Colour (0xf91a1a2e));
    g.fillRoundedRectangle (bounds, 6.0f);

    // Border
    g.setColour (juce::Colour (0x14ffffff));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // Items
    for (int i = 0; i < (int) items.size(); ++i)
    {
        auto& item = items[(size_t) i];
        auto r = item.bounds.reduced (4.0f, 1.0f);
        bool isHovered = (i == hoveredIndex);
        bool isSelected = (i == selectedIndex);

        // Hover highlight
        if (isHovered)
        {
            g.setColour (juce::Colour (0xff222238));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (Theme::accent());
            g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
        }

        // Checkmark for selected item
        if (isSelected)
        {
            float checkX = item.bounds.getX() + 12.0f;
            float checkY = item.bounds.getCentreY();

            juce::Path tick;
            tick.startNewSubPath (checkX, checkY);
            tick.lineTo (checkX + 3.0f, checkY + 3.0f);
            tick.lineTo (checkX + 9.0f, checkY - 3.0f);

            g.setColour (Theme::accent());
            g.strokePath (tick, juce::PathStrokeType (1.5f));
        }

        // Text
        float textX = item.bounds.getX() + 26.0f;
        g.setColour (isSelected ? Theme::accent() : Theme::textPrimary());
        g.setFont (juce::FontOptions (14.0f));
        g.drawText (item.text, juce::Rectangle<float> (textX, item.bounds.getY(),
                    item.bounds.getRight() - textX - 8.0f, item.bounds.getHeight()),
                    juce::Justification::centredLeft);
    }
}

void InstrumentPopup::mouseDown (const juce::MouseEvent& e)
{
    int index = getItemAtPosition (e.position);
    if (index >= 0 && onItemSelected)
        onItemSelected (items[(size_t) index].instrumentId);
}

void InstrumentPopup::mouseMove (const juce::MouseEvent& e)
{
    int index = getItemAtPosition (e.position);
    if (index != hoveredIndex)
    {
        hoveredIndex = index;
        repaint();
    }
}

void InstrumentPopup::mouseExit (const juce::MouseEvent&)
{
    if (hoveredIndex != -1)
    {
        hoveredIndex = -1;
        repaint();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// VolumeKnob
// ═══════════════════════════════════════════════════════════════════════════

VolumeKnob::VolumeKnob()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void VolumeKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = size * 0.5f;

    // ── Drop shadow (neumorphic depth) ──────────────────────────────
    {
        float sR1 = radius + 0.5f;
        g.setColour (juce::Colour (0x20000000));
        g.fillEllipse (cx - sR1, cy - sR1 + 1.5f, sR1 * 2.0f, sR1 * 2.0f);
        float sR2 = radius + 1.0f;
        g.setColour (juce::Colour (0x10000000));
        g.fillEllipse (cx - sR2, cy - sR2 + 3.0f, sR2 * 2.0f, sR2 * 2.0f);
    }

    // ── Neumorphic shadow well (inset ring) ─────────────────────────
    {
        float wellR = radius - 1.0f;
        g.setColour (juce::Colour (0x30000000));
        g.drawEllipse (cx - wellR - 0.5f, cy - wellR - 0.5f, wellR * 2.0f, wellR * 2.0f, 1.5f);
        g.setColour (juce::Colour (0x10ffffff));
        g.drawEllipse (cx - wellR + 0.5f, cy - wellR + 0.5f, wellR * 2.0f, wellR * 2.0f, 0.5f);
    }

    // ── Segmented arc (20 ticks, 270° sweep: 7 o'clock → 5 o'clock) ──
    constexpr int numTicks = 20;
    constexpr float startAngle = juce::MathConstants<float>::pi * 0.75f;   // 135° (7 o'clock)
    constexpr float endAngle   = juce::MathConstants<float>::pi * 2.25f;   // 405° (5 o'clock)
    constexpr float sweep = endAngle - startAngle;                          // 270°
    float arcR = radius - 3.0f;
    float tickInner = arcR - 3.0f;
    float tickOuter = arcR;

    int activeCount = muted ? 0 : (int) std::round (volume * (float) numTicks);

    for (int i = 0; i < numTicks; ++i)
    {
        float t = (float) i / (float) (numTicks - 1);
        float angle = startAngle + t * sweep;
        float cosA = std::cos (angle);
        float sinA = std::sin (angle);

        float x1 = cx + tickInner * cosA;
        float y1 = cy + tickInner * sinA;
        float x2 = cx + tickOuter * cosA;
        float y2 = cy + tickOuter * sinA;

        if (i < activeCount && !muted)
        {
            g.setColour (Theme::accent().withAlpha (0.9f));
            // Subtle glow behind active tick
            g.setColour (Theme::accent().withAlpha (0.15f));
            g.drawLine (x1, y1, x2, y2, 3.0f);
            g.setColour (Theme::accent());
        }
        else
        {
            g.setColour (muted ? Theme::textMuted().withAlpha (0.4f) : Theme::borderSubtle());
        }
        g.drawLine (x1, y1, x2, y2, 1.5f);
    }

    // ── Center knob (flat circle with subtle gradient) ────────────────
    float knobR = radius * 0.55f;
    {
        juce::ColourGradient grad (juce::Colour (0xff242840), cx, cy - knobR,
                                   juce::Colour (0xff1C2030), cx, cy + knobR, false);
        g.setGradientFill (grad);
        g.fillEllipse (cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f);

        // Border
        g.setColour (Theme::borderFaint());
        g.drawEllipse (cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f, 0.75f);
    }

    // ── Position dot (on center circle edge) ──────────────────────────
    {
        float dotAngle = startAngle + volume * sweep;
        float dotR = knobR - 3.0f;
        float dotX = cx + dotR * std::cos (dotAngle);
        float dotY = cy + dotR * std::sin (dotAngle);
        float dotSize = 2.5f;

        g.setColour (muted ? Theme::textMuted() : Theme::accent());
        g.fillEllipse (dotX - dotSize, dotY - dotSize, dotSize * 2.0f, dotSize * 2.0f);
    }

    // ── Muted state: diagonal strike-through ──────────────────────────
    if (muted)
    {
        g.setColour (Theme::textMuted().withAlpha (0.6f));
        float strikeR = knobR * 0.55f;
        g.drawLine (cx - strikeR, cy + strikeR, cx + strikeR, cy - strikeR, 1.5f);
    }
}

void VolumeKnob::mouseDown (const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().toFloat();
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float dist = e.position.getDistanceFrom ({ cx, cy });
    float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // Always prepare for drag; mute toggle deferred to mouseUp
    dragStartValue = volume;
    clickedCenter = (dist < radius * 0.4f);
}

void VolumeKnob::mouseUp (const juce::MouseEvent& e)
{
    // Toggle mute only if clicked center without dragging
    if (clickedCenter && e.getDistanceFromDragStart() < 3)
    {
        muted = !muted;
        repaint();
        if (onMuteToggle) onMuteToggle();
    }
}

void VolumeKnob::mouseDrag (const juce::MouseEvent& e)
{
    float sensitivity = 200.0f;
    float delta = -(float) e.getDistanceFromDragStartY() / sensitivity;
    float newVal = juce::jlimit (0.0f, 1.0f, dragStartValue + delta);

    if (newVal != volume)
    {
        volume = newVal;
        repaint();
        if (onValueChange) onValueChange();
    }
}

void VolumeKnob::mouseDoubleClick (const juce::MouseEvent&)
{
    volume = 0.75f;
    muted = false;
    repaint();
    if (onValueChange) onValueChange();
    if (onMuteToggle) onMuteToggle();
}

void VolumeKnob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    float step = 0.05f;
    float newVal = juce::jlimit (0.0f, 1.0f, volume + wheel.deltaY * step * 4.0f);

    if (newVal != volume)
    {
        volume = newVal;
        repaint();
        if (onValueChange) onValueChange();
    }
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
        entry.displayText = key.displayName;
        entry.chords = MusicTheory::getChordProgressions (key.name);

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

    // Detect relative major/minor pair (exactly 1 major + 1 minor)
    isRelativePair = (cards.size() == 2 && majorCount == 1);

    layoutCards ((float) getWidth());
    repaint();
}

void ScaleResultsPanel::layoutCards (float availableWidth)
{
    if (cards.empty() || availableWidth <= 0.0f)
    {
        relLabelY = 0.0f;
        selectedCardIdx = -1;
        setSize ((int) availableWidth, 1);
        return;
    }

    // Find selected card index
    selectedCardIdx = -1;
    for (int i = 0; i < (int) cards.size(); ++i)
    {
        if (cards[(size_t) i].key.name == selectedKeyName)
        {
            selectedCardIdx = i;
            break;
        }
    }

    // ── Special layout: relative pair (1 major + 1 minor) ──────────────
    if (isRelativePair && cards.size() == 2)
    {
        // Determine primary (selected key, or major if none selected)
        int primaryIdx = 0, secondaryIdx = 1;
        if (selectedKeyName.isNotEmpty() && cards[1].key.name == selectedKeyName)
        {
            primaryIdx = 1;
            secondaryIdx = 0;
        }

        // If a key is selected, show both as full-width cards (no floating label)
        if (selectedCardIdx >= 0)
        {
            float y = 4.0f;

            // Primary: full-width card (larger)
            cards[(size_t) primaryIdx].bounds = { 0.0f, y, availableWidth, cardPrimaryH };
            y += cardPrimaryH + cardGap;

            // Secondary: full-width card (standard) — "relative minor/major" shown as category label inside
            cards[(size_t) secondaryIdx].bounds = { 0.0f, y, availableWidth, cardHeight };
            y += cardHeight + 4.0f;

            setSize ((int) availableWidth, juce::jmax ((int) y, 1));
            return;
        }

        // No selection: both as centered chips with label between
        float contentH = 32.0f + 1.0f + 13.0f + 1.0f + 28.0f;  // primary pill + gaps + label + secondary pill
        float panelH = (float) getParentHeight();
        if (panelH < contentH) panelH = contentH + 4.0f;
        float topY = (panelH - contentH) * 0.5f;
        float y = topY;

        // Primary pill (slightly larger chip)
        juce::Font pFont { juce::FontOptions (19.0f, juce::Font::bold) };
        float pTextW = pFont.getStringWidthFloat (cards[(size_t) primaryIdx].displayText);
        float pW = pTextW + 18.0f * 2.0f;
        float pX = (availableWidth - pW) * 0.5f;
        cards[(size_t) primaryIdx].bounds = { pX, y, pW, 32.0f };
        y += 32.0f + 1.0f;

        relLabelY = y;
        y += 13.0f + 1.0f;

        // Secondary pill
        juce::Font sFont { juce::FontOptions (15.0f) };
        float sTextW = sFont.getStringWidthFloat (cards[(size_t) secondaryIdx].displayText);
        float sW = sTextW + 14.0f * 2.0f;
        float sX = (availableWidth - sW) * 0.5f;
        cards[(size_t) secondaryIdx].bounds = { sX, y, sW, 28.0f };

        setSize ((int) availableWidth, juce::jmax ((int) panelH, 1));
        return;
    }

    // ── Hybrid layout: card for selected key, chips for the rest ────────
    float y = 4.0f;
    juce::Font font { juce::FontOptions (chipFontSize) };

    // If a key is selected, place its card at the top
    if (selectedCardIdx >= 0)
    {
        cards[(size_t) selectedCardIdx].bounds = { 0.0f, y, availableWidth, cardHeight };
        y += cardHeight + cardGap + 4.0f;  // extra breathing room before chips
    }

    chipSectionY = y;

    // Count major/minor chips (excluding selected card)
    int majorChipCount = 0;
    for (int i = 0; i < majorCount; ++i)
        if (i != selectedCardIdx) ++majorChipCount;

    // Layout Major chips
    float cursorX = 0.0f;
    for (int i = 0; i < majorCount && i < (int) cards.size(); ++i)
    {
        if (i == selectedCardIdx) continue;

        float textW = font.getStringWidthFloat (cards[(size_t) i].chipText);
        float chipW = textW + chipPadX * 2.0f;

        if (cursorX + chipW > availableWidth && cursorX > 0.0f)
        {
            cursorX = 0.0f;
            y += chipHeight + chipGap;
        }

        cards[(size_t) i].bounds = { cursorX, y, chipW, chipHeight };
        cursorX += chipW + chipGap;
    }

    // Center each Major chip row
    {
        float rowY = chipSectionY;
        int rowStart = -1;
        for (int i = 0; i < majorCount && i < (int) cards.size(); ++i)
        {
            if (i == selectedCardIdx) continue;
            if (rowStart < 0) { rowStart = i; rowY = cards[(size_t) i].bounds.getY(); }

            bool newRow = (cards[(size_t) i].bounds.getY() != rowY);
            if (newRow)
            {
                // Center the previous row
                float rowRight = 0.0f;
                for (int j = rowStart; j < i; ++j)
                    if (j != selectedCardIdx)
                        rowRight = juce::jmax (rowRight, cards[(size_t) j].bounds.getRight());
                float offset = (availableWidth - rowRight) * 0.5f;
                for (int j = rowStart; j < i; ++j)
                    if (j != selectedCardIdx)
                        cards[(size_t) j].bounds.translate (offset, 0.0f);

                rowStart = i;
                rowY = cards[(size_t) i].bounds.getY();
            }
        }
        // Center last Major row
        if (rowStart >= 0)
        {
            float rowRight = 0.0f;
            for (int j = rowStart; j < majorCount && j < (int) cards.size(); ++j)
                if (j != selectedCardIdx)
                    rowRight = juce::jmax (rowRight, cards[(size_t) j].bounds.getRight());
            float offset = (availableWidth - rowRight) * 0.5f;
            for (int j = rowStart; j < majorCount && j < (int) cards.size(); ++j)
                if (j != selectedCardIdx)
                    cards[(size_t) j].bounds.translate (offset, 0.0f);
        }
    }

    // Separator between Major and Minor chip groups
    bool hasMinorChips = false;
    for (int i = majorCount; i < (int) cards.size(); ++i)
        if (i != selectedCardIdx) { hasMinorChips = true; break; }

    if (majorChipCount > 0 && hasMinorChips)
    {
        y += chipHeight + separatorGap;
        relLabelY = y;  // reuse for separator line position
        y += separatorGap;
    }
    else
    {
        relLabelY = 0.0f;
    }

    // Layout Minor chips
    cursorX = 0.0f;
    for (int i = majorCount; i < (int) cards.size(); ++i)
    {
        if (i == selectedCardIdx) continue;

        float textW = font.getStringWidthFloat (cards[(size_t) i].chipText);
        float chipW = textW + chipPadX * 2.0f;

        if (cursorX + chipW > availableWidth && cursorX > 0.0f)
        {
            cursorX = 0.0f;
            y += chipHeight + chipGap;
        }

        cards[(size_t) i].bounds = { cursorX, y, chipW, chipHeight };
        cursorX += chipW + chipGap;
    }

    // Center each Minor chip row
    {
        float rowY = -1.0f;
        int rowStart = -1;
        for (int i = majorCount; i < (int) cards.size(); ++i)
        {
            if (i == selectedCardIdx) continue;
            if (rowStart < 0) { rowStart = i; rowY = cards[(size_t) i].bounds.getY(); }

            bool newRow = (cards[(size_t) i].bounds.getY() != rowY);
            if (newRow)
            {
                float rowRight = 0.0f;
                for (int j = rowStart; j < i; ++j)
                    if (j != selectedCardIdx)
                        rowRight = juce::jmax (rowRight, cards[(size_t) j].bounds.getRight());
                float offset = (availableWidth - rowRight) * 0.5f;
                for (int j = rowStart; j < i; ++j)
                    if (j != selectedCardIdx)
                        cards[(size_t) j].bounds.translate (offset, 0.0f);

                rowStart = i;
                rowY = cards[(size_t) i].bounds.getY();
            }
        }
        // Center last Minor row
        if (rowStart >= 0)
        {
            float rowRight = 0.0f;
            for (int j = rowStart; j < (int) cards.size(); ++j)
                if (j != selectedCardIdx)
                    rowRight = juce::jmax (rowRight, cards[(size_t) j].bounds.getRight());
            float offset = (availableWidth - rowRight) * 0.5f;
            for (int j = rowStart; j < (int) cards.size(); ++j)
                if (j != selectedCardIdx)
                    cards[(size_t) j].bounds.translate (offset, 0.0f);
        }
    }

    // Check if any minor chips were laid out to get the last y
    float lastChipBottom = chipSectionY;
    for (int i = 0; i < (int) cards.size(); ++i)
    {
        if (i == selectedCardIdx) continue;
        lastChipBottom = juce::jmax (lastChipBottom, cards[(size_t) i].bounds.getBottom());
    }

    int totalH = (int) (lastChipBottom + 4.0f);
    // If only the selected card (no chips), use card bottom
    if (selectedCardIdx >= 0 && (int) cards.size() == 1)
        totalH = (int) (cards[(size_t) selectedCardIdx].bounds.getBottom() + 4.0f);

    setSize ((int) availableWidth, juce::jmax (totalH, 1));
}

void ScaleResultsPanel::resized()
{
    if (! cards.empty())
        layoutCards ((float) getWidth());
}

void ScaleResultsPanel::setSelectedKey (const juce::String& keyName)
{
    if (selectedKeyName != keyName)
    {
        selectedKeyName = keyName;
        layoutCards ((float) getWidth());  // Layout changes with selection
        repaint();
    }
}

void ScaleResultsPanel::paint (juce::Graphics& g)
{
    // ── Helper: draw a full-width detail card (selected key) ────────────
    auto drawDetailCard = [&] (const CardEntry& card, int idx, float titleSize,
                               const juce::String& categoryOverride = {})
    {
        auto r = card.bounds;
        bool isSel = (card.key.name == selectedKeyName);
        bool isHov = (idx == hoveredCardIndex);

        // ── Drop shadow (neumorphic depth) ──────────────────────────
        {
            auto shadowRect = r.translated (0.0f, 2.0f).expanded (1.0f);
            g.setColour (juce::Colour (0x30000000));
            g.fillRoundedRectangle (shadowRect, cardRadius + 1.0f);

            auto shadowRect2 = r.translated (0.0f, 4.0f).expanded (2.0f);
            g.setColour (juce::Colour (0x18000000));
            g.fillRoundedRectangle (shadowRect2, cardRadius + 2.0f);
        }

        // ── Top-edge highlight (subtle light from above) ────────────
        g.setColour (juce::Colour (0x0affffff));
        g.drawRoundedRectangle (r.reduced (0.5f).translated (0.0f, -0.5f), cardRadius, 0.5f);

        // ── Gradient fill ───────────────────────────────────────────
        if (isSel)
        {
            g.setColour (Theme::accent().withAlpha (0.15f));
            g.fillRoundedRectangle (r, cardRadius);
        }
        else
        {
            juce::ColourGradient grad (
                isHov ? juce::Colour (0xff2A2E48) : juce::Colour (0xff242840),
                0.0f, r.getY(),
                isHov ? juce::Colour (0xff222238) : juce::Colour (0xff1C2030),
                0.0f, r.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r, cardRadius);
        }

        // ── Border ──────────────────────────────────────────────────
        if (isSel)
            g.setColour (Theme::accent());
        else if (isHov)
            g.setColour (Theme::borderSubtle());
        else
            g.setColour (Theme::borderFaint());
        g.drawRoundedRectangle (r.reduced (0.5f), cardRadius, 0.75f);

        // ── Content ─────────────────────────────────────────────────
        float cx = r.getX() + cardPadX;
        float cy = r.getY() + cardPadY;
        float cw = r.getWidth() - cardPadX * 2.0f;

        // Category label
        juce::String category = categoryOverride.isNotEmpty()
                                    ? categoryOverride
                                    : card.key.type.toUpperCase();
        g.setColour (isSel ? Theme::accent().withAlpha (0.5f) : Theme::textMuted());
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText (category, (int) cx, (int) cy, (int) cw, 11,
                    juce::Justification::centredLeft);
        cy += 12.0f;

        // Key name
        g.setColour (isSel ? Theme::accent() : Theme::textPrimary());
        g.setFont (juce::FontOptions (titleSize, juce::Font::bold));
        g.drawText (card.displayText, (int) cx, (int) cy, (int) cw, 22,
                    juce::Justification::centredLeft);
    };

    // ── Helper: draw a chip (unselected key) ────────────────────────────
    auto drawChip = [&] (const CardEntry& card, int idx)
    {
        auto& r = card.bounds;
        bool isHov = (idx == hoveredCardIndex);

        // Chip background
        if (isHov)
            g.setColour (juce::Colour (0xff222238));
        else
            g.setColour (Theme::cardBg());
        g.fillRoundedRectangle (r, chipRadius);

        // Chip border
        if (isHov)
            g.setColour (Theme::accent().withAlpha (0.6f));
        else
            g.setColour (Theme::borderFaint());
        g.drawRoundedRectangle (r.reduced (0.5f), chipRadius, 1.0f);

        // Chip text
        g.setColour (isHov ? Theme::textPrimary() : Theme::textPrimary());
        g.setFont (juce::FontOptions (chipFontSize));
        g.drawText (card.chipText, r, juce::Justification::centred);
    };

    // ── Special paint: relative pair ─────────────────────────────────────
    if (isRelativePair && cards.size() == 2)
    {
        int primaryIdx = 0, secondaryIdx = 1;
        if (selectedKeyName.isNotEmpty() && cards[1].key.name == selectedKeyName)
        {
            primaryIdx = 1;
            secondaryIdx = 0;
        }

        if (selectedCardIdx >= 0)
        {
            // Selected: both as full-width detail cards
            drawDetailCard (cards[(size_t) primaryIdx], primaryIdx, 19.0f);

            // Secondary card with "RELATIVE MINOR"/"RELATIVE MAJOR" as category
            juce::String relCategory = (cards[(size_t) secondaryIdx].key.type == "Major")
                                           ? "RELATIVE MAJOR"
                                           : "RELATIVE MINOR";
            drawDetailCard (cards[(size_t) secondaryIdx], secondaryIdx, 17.0f, relCategory);
        }
        else
        {
            // No selection: both as pills (original relative pair style)
            // Primary pill
            {
                auto& card = cards[(size_t) primaryIdx];
                auto& r = card.bounds;
                bool isHov = (primaryIdx == hoveredCardIndex);

                g.setColour (isHov ? juce::Colour (0xff222238) : Theme::cardBg());
                g.fillRoundedRectangle (r, 16.0f);
                g.setColour (isHov ? Theme::accent().withAlpha (0.6f) : Theme::accent().withAlpha (0.3f));
                g.drawRoundedRectangle (r.reduced (0.5f), 16.0f, 1.0f);
                g.setColour (Theme::textPrimary());
                g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
                g.drawText (card.displayText, r, juce::Justification::centred);
            }

            // Relationship label
            juce::String label = (cards[(size_t) primaryIdx].key.type == "Major")
                                     ? "relative minor"
                                     : "relative major";
            g.setColour (Theme::textSecondary().withAlpha (0.5f));
            g.setFont (juce::FontOptions (11.0f));
            g.drawText (label, 0, (int) relLabelY, getWidth(), 13,
                        juce::Justification::centred);

            // Secondary pill
            {
                auto& card = cards[(size_t) secondaryIdx];
                auto& r = card.bounds;
                bool isHov = (secondaryIdx == hoveredCardIndex);

                g.setColour (isHov ? juce::Colour (0xff222238) : Theme::cardBg());
                g.fillRoundedRectangle (r, 14.0f);
                g.setColour (isHov ? Theme::accent().withAlpha (0.4f) : Theme::borderFaint());
                g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);
                g.setColour (isHov ? Theme::textPrimary() : Theme::textSecondary());
                g.setFont (juce::FontOptions (15.0f));
                g.drawText (card.displayText, r, juce::Justification::centred);
            }
        }
        return;
    }

    // ── Standard hybrid rendering ────────────────────────────────────────

    // Draw selected key as detail card
    if (selectedCardIdx >= 0)
        drawDetailCard (cards[(size_t) selectedCardIdx], selectedCardIdx, 17.0f);

    // Separator line between Major and Minor chip groups
    int majorChipCount = 0;
    for (int i = 0; i < majorCount; ++i)
        if (i != selectedCardIdx) ++majorChipCount;
    bool hasMinorChips = false;
    for (int i = majorCount; i < (int) cards.size(); ++i)
        if (i != selectedCardIdx) { hasMinorChips = true; break; }

    if (majorChipCount > 0 && hasMinorChips && relLabelY > 0.0f)
    {
        g.setColour (Theme::borderSubtle());
        g.drawLine (0.0f, relLabelY, (float) getWidth(), relLabelY, 1.0f);
    }

    // Draw unselected keys as chips
    for (int i = 0; i < (int) cards.size(); ++i)
    {
        if (i == selectedCardIdx) continue;
        drawChip (cards[(size_t) i], i);
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
    int controlsBottom = 4 + 28 + 12 + 280 + 12 + 32;
    int resultsStartY = controlsBottom + 12;

    // ── Status area (non-"some" states) ──────────────────────────────────
    if (resultStatus == "all-visible")
    {
        bool hov = emptyStateHovered;
        float w_f = (float) getWidth();
        float radius = 8.0f;
        float boxH = (float) getHeight() - (float) resultsStartY - 8.0f;
        auto emptyArea = juce::Rectangle<float> ((float) margin, (float) resultsStartY,
                                                  w_f - margin * 2.0f, boxH);

        // Vertically center content block within the box
        // Content: icon(20) + gap(6) + text(14) + gap(3) + link(12) + gap(4) + formats(10) = 69
        float contentH = 69.0f;
        float topY = emptyArea.getY() + (boxH - contentH) * 0.5f;
        float curY = topY;

        // Music note icon
        g.setColour (hov ? Theme::textSecondary() : Theme::textMuted());
        g.setFont (juce::FontOptions (20.0f));
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x99\xab"),
                    (int) emptyArea.getX(), (int) curY, (int) emptyArea.getWidth(), 20,
                    juce::Justification::centred);
        curY += 26.0f;

        // Main text
        g.setColour (hov ? Theme::textPrimary().withAlpha (0.7f) : Theme::textSecondary());
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Play notes or drop an audio file",
                    (int) emptyArea.getX(), (int) curY, (int) emptyArea.getWidth(), 14,
                    juce::Justification::centred);
        curY += 17.0f;

        // Browse link
        g.setColour (hov ? Theme::accent().withAlpha (0.9f) : Theme::accent().withAlpha (0.5f));
        g.setFont (juce::FontOptions (10.5f));
        g.drawText ("or click to browse",
                    (int) emptyArea.getX(), (int) curY, (int) emptyArea.getWidth(), 12,
                    juce::Justification::centred);
        curY += 16.0f;

        // File formats
        g.setColour (hov ? Theme::textMuted().brighter (0.3f) : Theme::textMuted());
        g.setFont (juce::FontOptions (8.5f)
            .withName (juce::Font::getDefaultMonospacedFontName()));
        g.drawText (".wav  .mp3  .aiff  .flac",
                    (int) emptyArea.getX(), (int) curY, (int) emptyArea.getWidth(), 10,
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
        g.setFont (juce::FontOptions (9.5f));
        g.drawText ("Also try:", margin, altKeyY, 46, altKeyH,
                    juce::Justification::centredLeft);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// DragOverlay (leaf component — file-drag visual)
// ═══════════════════════════════════════════════════════════════════════════

void DragOverlay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dim background (fully opaque to cover window rounded corners)
    g.setColour (juce::Colour (0xff0f0a1a));
    g.fillRect (bounds);

    // Drop zone box
    auto dropZone = bounds.reduced (30.0f, 120.0f);
    float dzRadius = 8.0f;

    // Subtle accent glow behind drop zone
    g.setColour (Theme::accent().withAlpha (0.06f));
    g.fillRoundedRectangle (dropZone.expanded (16.0f), dzRadius + 8.0f);
    g.setColour (Theme::accent().withAlpha (0.10f));
    g.fillRoundedRectangle (dropZone.expanded (6.0f), dzRadius + 3.0f);

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
    setSize (460, 460);

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
        bpmPill.setButtonText ("\xe2\x80\x93 BPM");
        bpmPill.setColour (juce::TextButton::textColourOffId, Theme::textMuted());
        bpmPill.setColour (juce::ComboBox::outlineColourId,   juce::Colour (0x0fffffff));
        bpmPill.repaint();
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
        btn.setColour (juce::TextButton::textColourOffId, Theme::textSecondary());
        btn.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x18ffffff));
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

    // ── BPM pill (read-only indicator, populated after analysis) ─────────
    bpmPill.setLookAndFeel (&bpmPillLF);
    bpmPill.setColour (juce::TextButton::buttonColourId,  juce::Colour (0x00000000));
    bpmPill.setColour (juce::TextButton::textColourOffId, Theme::textMuted());
    bpmPill.setColour (juce::ComboBox::outlineColourId,   juce::Colour (0x0fffffff));
    bpmPill.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (bpmPill);

    // ── Instrument selector (neumorphic circle, opens popup) ───────────
    instrumentButton.setSelectedIndex (processorRef.currentInstrument.load (std::memory_order_relaxed));
    instrumentButton.onClick = [this]() { showInstrumentPopup(); };
    instrumentButton.setTooltip ("Instrument");
    addAndMakeVisible (instrumentButton);

    // ── Volume knob (rotary control with mute toggle) ──────────────────
    volumeKnob.setValue (processorRef.masterVolume.load (std::memory_order_relaxed));
    volumeKnob.setMuted (processorRef.isMuted.load (std::memory_order_relaxed));
    volumeKnob.onValueChange = [this]() {
        processorRef.masterVolume.store (volumeKnob.getValue(), std::memory_order_relaxed);
    };
    volumeKnob.onMuteToggle = [this]() {
        processorRef.isMuted.store (volumeKnob.isMuted(), std::memory_order_relaxed);
    };
    volumeKnob.setTooltip ("Volume (double-click to reset)");
    addAndMakeVisible (volumeKnob);

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
    bpmPill.setLookAndFeel (nullptr);
    dismissKeyGridPopup();
    dismissOptionsPopup();
    dismissInstrumentPopup();

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

    // Dashed border for empty state drop zone (drawn over children)
    if (browseButton.isVisible())
    {
        bool hov = chordsDisplay.emptyStateHovered;
        int margin = 16;
        float radius = 8.0f;
        auto emptyArea = browseButton.getBounds().toFloat();

        float dashLen = 8.0f;
        float gapLen = 6.0f;

        // Flatten the rounded rect into a polyline, then resample into even dashes
        juce::Path roundedRect;
        roundedRect.addRoundedRectangle (emptyArea.reduced (0.5f), radius);

        // Collect all points along the path
        std::vector<juce::Point<float>> pts;
        std::vector<float> cumDist;
        {
            juce::PathFlatteningIterator iter (roundedRect, juce::AffineTransform(), 0.5f);
            float total = 0.0f;
            while (iter.next())
            {
                juce::Point<float> pt ((float) iter.x2, (float) iter.y2);
                if (! pts.empty())
                    total += pts.back().getDistanceFrom (pt);
                pts.push_back (pt);
                cumDist.push_back (total);
            }
        }

        if (pts.size() > 1)
        {
            float totalLen = cumDist.back();
            // Adjust dash/gap to fit evenly around perimeter
            int numDashes = juce::jmax (1, (int) std::round (totalLen / (dashLen + gapLen)));
            float segLen = totalLen / (float) numDashes;
            float actualDash = segLen * (dashLen / (dashLen + gapLen));
            float actualGap = segLen - actualDash;

            // Helper: interpolate point at distance d along the polyline
            auto pointAtDist = [&] (float d) -> juce::Point<float>
            {
                if (d <= 0.0f) return pts.front();
                if (d >= totalLen) return pts.back();
                auto it = std::lower_bound (cumDist.begin(), cumDist.end(), d);
                size_t idx = (size_t) (it - cumDist.begin());
                if (idx == 0) return pts[0];
                float seg = cumDist[idx] - cumDist[idx - 1];
                float t = (seg > 0.0f) ? (d - cumDist[idx - 1]) / seg : 0.0f;
                return pts[idx - 1] + (pts[idx] - pts[idx - 1]) * t;
            };

            juce::Path dashedPath;
            float cursor = 0.0f;
            for (int i = 0; i < numDashes; ++i)
            {
                float dashStart = cursor;
                float dashEnd = juce::jmin (cursor + actualDash, totalLen);
                dashedPath.startNewSubPath (pointAtDist (dashStart));
                // Sample several points along the dash for curved corners
                int steps = juce::jmax (2, (int) ((dashEnd - dashStart) / 1.5f));
                for (int s = 1; s <= steps; ++s)
                {
                    float d = dashStart + (dashEnd - dashStart) * ((float) s / (float) steps);
                    dashedPath.lineTo (pointAtDist (d));
                }
                cursor += segLen;
            }

            g.setColour (hov ? Theme::accent().withAlpha (0.45f) : Theme::textMuted().withAlpha (0.45f));
            g.strokePath (dashedPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
        }
    }
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
    int pianoY = titleY + titleH + 12;
    int pianoH = 280;
    pianoKeyboard.setBounds (margin, pianoY, w - margin * 2, pianoH);

    // ── Controls row (secondary — compact, side by side) ─────────────
    int controlsY = pianoY + pianoH + 12;
    int controlsH = 32;
    int bpmW = 80;
    int resetW = 72;
    int browseW = 32;
    int instW = 72;
    int volW = 32;
    int gap = 8;
    int dropdownW = w - margin * 2 - bpmW - browseW - resetW - instW - volW - gap * 5;

    keyDropdown.setBounds      (margin, controlsY, dropdownW, controlsH);
    bpmPill.setBounds          (margin + dropdownW + gap, controlsY, bpmW, controlsH);
    browseIconButton.setBounds (margin + dropdownW + gap + bpmW + gap, controlsY, browseW, controlsH);
    resetButton.setBounds      (margin + dropdownW + gap + bpmW + gap + browseW + gap, controlsY, resetW, controlsH);
    instrumentButton.setBounds (margin + dropdownW + gap + bpmW + gap + browseW + gap + resetW + gap, controlsY, instW, controlsH);
    volumeKnob.setBounds       (w - margin - volW, controlsY, volW, controlsH);

    // ── Results viewport (scrollable card list) ───────────────────────
    int resultsY = controlsY + controlsH + 12;
    bool hasAlts = altKeyButton1.isVisible();
    int altRowH = hasAlts ? 26 : 0;
    int resultsH = getHeight() - resultsY - altRowH - 8;
    resultsViewport.setBounds (margin, resultsY, w - margin * 2, juce::jmax (resultsH, 60));
    resultsPanel.setSize (resultsViewport.getWidth() - (resultsViewport.isVerticalScrollBarShown() ? 6 : 0),
                          resultsPanel.getHeight());

    // ── Browse button (covers the empty state area) ──────────────────
    browseButton.setBounds (margin, resultsY, w - margin * 2, getHeight() - resultsY - 8);

    // ── Alternative key buttons (below results) ──────────────────────
    if (hasAlts)
    {
        int altY = resultsViewport.getBottom() + 4;
        int labelW = 46;   // "Also try:" label width (drawn by ChordsDisplay)
        int btnW = (w - margin * 2 - labelW - 8 - 8) / 2;  // two buttons, 8px gaps
        int btnH = 20;
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

            // Auto-select the detected primary key
            auto detectedName = audioAnalyzer.getDetectedKeyName();
            if (detectedName.isNotEmpty())
            {
                processorRef.selectedKey = detectedName;
                processorRef.currentChords = MusicTheory::getChordProgressions (detectedName);
            }

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

        // Update BPM pill
        float bpm = audioAnalyzer.getDetectedBPM();
        if (bpm >= 60.0f && bpm <= 200.0f)
        {
            bpmPill.setButtonText (juce::String ((int) std::round (bpm)) + " BPM");
            bpmPill.setColour (juce::TextButton::textColourOffId, Theme::textPrimary());
            bpmPill.setColour (juce::ComboBox::outlineColourId,   Theme::accent());
        }
        else
        {
            bpmPill.setButtonText ("\xe2\x80\x93 BPM");
            bpmPill.setColour (juce::TextButton::textColourOffId, Theme::textMuted());
            bpmPill.setColour (juce::ComboBox::outlineColourId,   juce::Colour (0x0fffffff));
        }
        bpmPill.repaint();

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

    dismissOptionsPopup();
    dismissInstrumentPopup();

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

    dismissKeyGridPopup();
    dismissInstrumentPopup();

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

void ScaleFinderEditor::showInstrumentPopup()
{
    if (instrumentPopup != nullptr)
    {
        dismissInstrumentPopup();
        return;  // toggle off
    }

    dismissKeyGridPopup();
    dismissOptionsPopup();

    instrumentPopup = std::make_unique<InstrumentPopup>();
    instrumentPopup->setSelectedIndex (processorRef.currentInstrument.load (std::memory_order_relaxed));
    instrumentPopup->onItemSelected = [this] (int instrumentId)
    {
        processorRef.currentInstrument.store (instrumentId, std::memory_order_relaxed);
        instrumentButton.setSelectedIndex (instrumentId);
        dismissInstrumentPopup();
    };

    // Position above the button
    auto btnBounds = instrumentButton.getBounds();
    int popupW = instrumentPopup->getWidth();
    int popupH = instrumentPopup->getHeight();
    int popupX = btnBounds.getCentreX() - popupW / 2;
    int popupY = btnBounds.getY() - popupH - 4;

    // Clamp to editor bounds
    if (popupX < 8) popupX = 8;
    if (popupX + popupW > getWidth() - 8) popupX = getWidth() - 8 - popupW;
    if (popupY < 8) popupY = btnBounds.getBottom() + 4;  // flip below if no room above

    instrumentPopup->setBounds (popupX, popupY, popupW, popupH);
    addAndMakeVisible (*instrumentPopup);
    instrumentPopup->toFront (true);

    instrumentButton.setPopupOpen (true);
}

void ScaleFinderEditor::dismissInstrumentPopup()
{
    if (instrumentPopup != nullptr)
    {
        removeChildComponent (instrumentPopup.get());
        instrumentPopup.reset();
    }
    instrumentButton.setPopupOpen (false);
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

    // Dismiss instrument popup if click is outside it
    if (instrumentPopup != nullptr)
    {
        auto localPos = instrumentPopup->getLocalPoint (this, e.position.toInt());
        if (! instrumentPopup->getLocalBounds().contains (localPos))
            dismissInstrumentPopup();
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
    dragOverlay.setAlpha (0.0f);
    dragOverlay.setVisible (true);
    dragOverlay.toFront (false);
    juce::Desktop::getInstance().getAnimator().animateComponent (
        &dragOverlay, dragOverlay.getBounds(), 1.0f, 150, false, 1.0, 1.0);
    updateChordsDisplay();
}

void ScaleFinderEditor::fileDragExit (const juce::StringArray&)
{
    isDragOver = false;
    juce::Desktop::getInstance().getAnimator().fadeOut (&dragOverlay, 150);
    updateChordsDisplay();
}

void ScaleFinderEditor::filesDropped (const juce::StringArray& files, int, int)
{
    isDragOver = false;
    juce::Desktop::getInstance().getAnimator().fadeOut (&dragOverlay, 150);
    if (files.isEmpty()) return;

    juce::File audioFile (files[0]);

    // Clear previous state
    processorRef.clearNotes();
    pianoKeyboard.clearSelection();
    keyDropdown.setButtonText ("select key...");
    bpmPill.setButtonText ("\xe2\x80\x93 BPM");
    bpmPill.setColour (juce::TextButton::textColourOffId, Theme::textMuted());
    bpmPill.setColour (juce::ComboBox::outlineColourId,   juce::Colour (0x0fffffff));
    bpmPill.repaint();

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
            bpmPill.setButtonText ("\xe2\x80\x93 BPM");
            bpmPill.setColour (juce::TextButton::textColourOffId, Theme::textMuted());
            bpmPill.setColour (juce::ComboBox::outlineColourId,   juce::Colour (0x0fffffff));
            bpmPill.repaint();

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

