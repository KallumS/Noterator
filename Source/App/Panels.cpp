#include "Panels.h"

#include "Spelling.h"
#include "Theme.h"

namespace nt
{

//==============================================================================

GlyphButton::GlyphButton (const juce::String& name, juce::juce_wchar g, float s) : juce::Button (name), glyph (g), scale (s)
{
    setClickingTogglesState (false);
}

void GlyphButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = theme::focusRing (g, *this, getLocalBounds().toFloat()).reduced (0.5f);
    juce::Colour fill = getToggleState() ? theme::accent : theme::control;
    if (down) fill = getToggleState() ? theme::shade (theme::accent, -0.18f) : theme::controlHeld;
    else if (highlighted) fill = getToggleState() ? theme::shade (theme::accent, 0.18f) : theme::controlHover;
    g.setColour (fill);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (theme::ink);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    auto& gl = Glyphs::get();
    const auto b = gl.bounds (glyph);
    if (b.isEmpty()) return;
    const float sp = std::min ((r.getHeight() - 8.0f) / std::max (1.0f, b.getHeight()), (r.getWidth() - 8.0f) / std::max (1.0f, b.getWidth())) * scale;
    const float x = r.getCentreX() - (b.getX() + b.getWidth() * 0.5f) * sp;
    const float y = r.getCentreY() - (b.getY() + b.getHeight() * 0.5f) * sp;
    gl.draw (g, glyph, x, y, sp);
}

//==============================================================================

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    getLookAndFeel().drawButtonBackground (g, *this, findColour (juce::TextButton::buttonColourId), highlighted, down);
    juce::Path p;
    if (kind == Kind::start || kind == Kind::end)
    {
        // A bar and a triangle pointing at it: back to the start, or on to the end.
        const auto r = getLocalBounds().toFloat().withSizeKeepingCentre (14.0f, 12.0f);
        if (kind == Kind::start)
        {
            p.addRectangle (r.getX(), r.getY(), 2.5f, r.getHeight());
            p.addTriangle (r.getRight(), r.getY(), r.getRight(), r.getBottom(), r.getX() + 3.5f, r.getCentreY());
        }
        else
        {
            p.addRectangle (r.getRight() - 2.5f, r.getY(), 2.5f, r.getHeight());
            p.addTriangle (r.getX(), r.getY(), r.getX(), r.getBottom(), r.getRight() - 3.5f, r.getCentreY());
        }
        g.setColour (isEnabled() ? theme::ink : theme::ink.withAlpha (0.4f));
        g.fillPath (p);
        return;
    }
    // A chevron up or down, or a cross, stroked as the Mac's symbols are.
    const auto r = getLocalBounds().toFloat().withSizeKeepingCentre (9.0f, 9.0f);
    if (kind == Kind::up)
    {
        p.startNewSubPath (r.getX(), r.getBottom() - 2.0f);
        p.lineTo (r.getCentreX(), r.getY() + 2.0f);
        p.lineTo (r.getRight(), r.getBottom() - 2.0f);
    }
    else if (kind == Kind::down)
    {
        p.startNewSubPath (r.getX(), r.getY() + 2.0f);
        p.lineTo (r.getCentreX(), r.getBottom() - 2.0f);
        p.lineTo (r.getRight(), r.getY() + 2.0f);
    }
    else
    {
        p.startNewSubPath (r.getX(), r.getY());
        p.lineTo (r.getRight(), r.getBottom());
        p.startNewSubPath (r.getRight(), r.getY());
        p.lineTo (r.getX(), r.getBottom());
    }
    g.setColour (isEnabled() ? theme::ink : theme::ink.withAlpha (0.4f));
    g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PlayButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    getLookAndFeel().drawButtonBackground (g, *this, findColour (juce::TextButton::buttonColourId), highlighted, down);
    const auto font = getLookAndFeel().getTextButtonFont (*this, getHeight());
    const float textW = juce::GlyphArrangement::getStringWidth (font, getButtonText());
    const float iconW = 10.0f, gap = 6.0f;
    const float x = (static_cast<float> (getWidth()) - (iconW + gap + textW)) * 0.5f;
    const auto icon = juce::Rectangle<float> (x, static_cast<float> (getHeight()) * 0.5f - 5.0f, iconW, 10.0f);
    g.setColour (theme::ink);
    if (getToggleState()) g.fillRect (icon.reduced (0.5f));
    else
    {
        juce::Path p;
        p.addTriangle (icon.getX(), icon.getY(), icon.getX(), icon.getBottom(), icon.getRight(), icon.getCentreY());
        g.fillPath (p);
    }
    g.setFont (font);
    g.drawText (getButtonText(), juce::Rectangle<float> (icon.getRight() + gap, 0.0f, textW + 2.0f, static_cast<float> (getHeight())),
                juce::Justification::centredLeft, false);
}

//==============================================================================

namespace
{
struct Duration { const char* name; Tick ticks; juce::juce_wchar glyph; const char* key; };
const Duration durationList[] = {
    { "Whole note", 4 * PPQ, 0xE1D2, "7" }, { "Half note", 2 * PPQ, 0xE1D3, "6" }, { "Quarter note", PPQ, 0xE1D5, "5" },
    { "Eighth note", PPQ / 2, 0xE1D7, "4" }, { "Sixteenth note", PPQ / 4, 0xE1D9, "3" }, { "Thirty-second note", PPQ / 8, 0xE1DB, "2" },
};
} // namespace

Toolbar::Toolbar (Controller& c) : controller (c)
{
    for (auto* b : std::initializer_list<juce::Button*> { &startButton, &endButton }) addAndMakeVisible (b);
    for (auto* b : std::initializer_list<juce::Component*> { &fileButton, &playButton, &followButton,
                     &voiceButton, &transposeButton, &clearButton, &zoomOut, &zoomIn })
        addAndMakeVisible (b);

    fileButton.setTooltip ("New, Open, Save, Export, Undo and Redo, score settings, sound, Note Input and Dark Page");
    playButton.setTooltip (ui ("Play from the cursor, or stop (⇧Space). Space plays from bar 1."));
    startButton.setTooltip (ui ("Go to the start (Home). If the music is playing, it plays on from bar 1."));
    endButton.setTooltip ("Go to the end of the music (End).");
    followButton.setTooltip ("Scroll the page with the music as it plays. To turn a page at a time, use the Play menu.");
    voiceButton.setTooltip ("Choose the voice to write in: 1 has stems up, 2 has stems down. Press V to swap the selection's voice.");
    transposeButton.setTooltip ("Show the score at concert pitch, or as transposing instruments read it.");
    zoomIn.setTooltip (ui ("Zoom in (⌘=)."));
    zoomOut.setTooltip (ui ("Zoom out (⌘−)."));
    clearButton.setTooltip ("Clear every note from every part. The parts, tempo, key and bars stay. It asks first, and you can undo it.");
    clearButton.setTitle ("Clear All Notes");
    clearButton.onClick = [this] { if (onClear) onClear(); };
    // What VoiceOver says for buttons whose face is a picture or a sign.
    zoomIn.setTitle ("Zoom In");
    zoomOut.setTitle ("Zoom Out");
    followButton.setToggleable (true);
    playButton.setToggleable (true);

    fileButton.onClick = [this] { if (onFile) onFile(); };
    playButton.onClick = [this] { controller.togglePlay(); };
    startButton.onClick = [this] { if (onStart) onStart(); };
    endButton.onClick = [this] { if (onEnd) onEnd(); };
    followButton.onClick = [this] { controller.toggleFollow(); };
    voiceButton.onClick = [this] { controller.setVoice (1 - controller.input.voice); };
    transposeButton.onClick = [this] { controller.transposedScore = ! controller.transposedScore; controller.viewChanged(); };
    zoomOut.onClick = [this] { controller.zoom = std::max (5.0f, controller.zoom / 1.15f); controller.viewChanged(); };
    zoomIn.onClick = [this] { controller.zoom = std::min (24.0f, controller.zoom * 1.15f); controller.viewChanged(); };

    for (const auto& d : durationList)
    {
        auto b = std::make_unique<GlyphButton> (d.name, d.glyph, 0.95f);
        b->setRadioGroupId (1, juce::dontSendNotification);
        b->setTooltip (juce::String (d.name) + " (" + d.key + ")");
        const Tick t = d.ticks;
        b->onClick = [this, t] { controller.setDuration (t); };
        addAndMakeVisible (*b);
        durations.push_back (std::move (b));
    }
    for (auto* b : { &dotButton, &tripletButton, &restButton }) addAndMakeVisible (b);
    dotButton.setToggleable (true);
    tripletButton.setToggleable (true);
    dotButton.setTooltip ("Dot (.)");
    tripletButton.setTooltip ("Triplet (T)");
    restButton.setTooltip ("Write a rest at the cursor (0)");
    dotButton.onClick = [this] { controller.toggleDot(); };
    tripletButton.onClick = [this] { controller.toggleTriplet(); };
    restButton.onClick = [this] { controller.typeRest(); };

    controller.addChangeListener (this);
    theme::clicksKeepFocus (*this);
    refresh();
}

Toolbar::~Toolbar() { controller.removeChangeListener (this); }

void Toolbar::paint (juce::Graphics& g)
{
    g.fillAll (theme::popup);
    g.setColour (theme::rule);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

// The writing tools - note values, dot, triplet, rest, voice - are the
// second row when one row is not wide enough.
int Toolbar::leftWidth() const { return 66 + 30 + 2 + 72 + 2 + 30 + 6 + 60 + 14; }
int Toolbar::rightWidth() const { return 76 + 10 + 106 + 10 + 28 + 2 + 28; }

namespace { constexpr int toolsWidth = 6 * 32 + 4 + 3 * 32 + 4 + 66; }

int Toolbar::heightFor (int width) const
{
    return width - 16 >= leftWidth() + toolsWidth + 20 + rightWidth() ? rowHeight : 2 * rowHeight - 8;
}

void Toolbar::resized()
{
    const bool twoRows = getHeight() > rowHeight;
    auto all = getLocalBounds().reduced (8, 7);
    auto r = twoRows ? all.removeFromTop (rowHeight - 14) : all;
    auto place = [] (juce::Rectangle<int>& row, juce::Component& c, int w, int gap = 4) { c.setBounds (row.removeFromLeft (w)); row.removeFromLeft (gap); };
    place (r, fileButton, 52, 14);
    place (r, startButton, 30, 2);
    place (r, playButton, 72, 2);
    place (r, endButton, 30, 6);
    place (r, followButton, 60, 14);
    auto right = r;
    zoomIn.setBounds (right.removeFromRight (28));
    right.removeFromRight (2);
    zoomOut.setBounds (right.removeFromRight (28));
    right.removeFromRight (10);
    transposeButton.setBounds (right.removeFromRight (106));
    right.removeFromRight (10);
    clearButton.setBounds (right.removeFromRight (76));

    auto tools = twoRows ? all.withTrimmedTop (6) : r;
    for (auto& d : durations) place (tools, *d, 30, 2);
    tools.removeFromLeft (4);
    place (tools, dotButton, 30, 2); place (tools, tripletButton, 30, 2); place (tools, restButton, 30, 6);
    place (tools, voiceButton, 66, 14);
}

void Toolbar::changeListenerCallback (juce::ChangeBroadcaster*) { refresh(); }

void Toolbar::refresh()
{
    playButton.setButtonText (controller.audio.isPlaying() ? "Stop" : "Play");
    playButton.setToggleState (controller.audio.isPlaying(), juce::dontSendNotification);
    followButton.setToggleState (controller.followPlayback, juce::dontSendNotification);
    for (size_t i = 0; i < durations.size(); ++i)
        durations[i]->setToggleState (durationList[i].ticks == controller.input.base, juce::dontSendNotification);
    dotButton.setToggleState (controller.input.dotted, juce::dontSendNotification);
    tripletButton.setToggleState (controller.input.triplet, juce::dontSendNotification);
    voiceButton.setButtonText ("Voice " + juce::String (controller.input.voice + 1));
    transposeButton.setButtonText (controller.transposedScore ? "Transposed" : "Concert Pitch");
    repaint();
}

//==============================================================================

struct PartsPanel::Row : public juce::Component
{
    Row (Controller& c, uint32_t id) : controller (c), partId (id)
    {
        for (auto* comp : std::initializer_list<juce::Component*> { &name, &instrument, &mute, &solo, &autoCC, &volume, &up, &down, &remove })
            addAndMakeVisible (comp);
        name.setFont (juce::FontOptions (14.0f));
        name.onReturnKey = name.onFocusLost = [this]
        {
            const auto text = name.getText().trim().toStdString();
            const auto* p = controller.score.partById (partId);
            if (p != nullptr && ! text.empty() && text != p->name)
                controller.edit ("Renamed a part", [this, text] (Score& s) { if (auto* q = s.partById (partId)) q->name = text; });
        };
        int itemId = 1;
        for (const auto& fam : instrumentFamilies())
        {
            instrument.addSectionHeading (fam);
            for (const auto& inst : instruments())
                if (inst.family == fam) { instrument.addItem (inst.name, itemId); ids.push_back (inst.id); ++itemId; }
        }
        instrument.onChange = [this]
        {
            const int i = instrument.getSelectedId() - 1;
            if (i >= 0 && i < static_cast<int> (ids.size())) controller.setPartInstrument (partId, ids[static_cast<size_t> (i)]);
        };
        mute.setButtonText ("M");
        solo.setButtonText ("S");
        autoCC.setButtonText ("AutoCC");
        mute.setTooltip ("Mute");
        solo.setTooltip ("Solo");
        // VoiceOver reads these rather than the letters on the buttons.
        name.setTitle ("Part name");
        instrument.setTitle ("Instrument");
        mute.setTitle ("Mute");
        solo.setTitle ("Solo");
        volume.setTitle ("Volume");
        mute.setToggleable (true);
        solo.setToggleable (true);
        autoCC.setTooltip ("Draw this part's dynamics, expression and vibrato curves automatically.");
        mute.onClick = [this] { toggle ([] (Part& p) { p.mute = ! p.mute; }); };
        solo.onClick = [this] { toggle ([] (Part& p) { p.solo = ! p.solo; }); };
        autoCC.onClick = [this] { toggle ([] (Part& p) { p.autoCC = ! p.autoCC; }); };
        volume.setSliderStyle (juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        volume.setRange (0.0, 1.0);
        volume.setTooltip ("Volume");
        volume.onDragEnd = [this] { commitVolume(); };
        // The arrow keys move it too, once Tab has reached it: each step is an edit.
        volume.onValueChange = [this] { if (! volume.isMouseButtonDown()) commitVolume(); };
        up.onClick = [this] { controller.movePart (partId, -1); };
        down.onClick = [this] { controller.movePart (partId, 1); };
        remove.onClick = [this]
        {
            const auto* p = controller.score.partById (partId);
            if (p == nullptr) return;
            if (p->notes.empty()) { controller.removePart (partId); return; }
            // The Mac's own alert (0048), its button saying what it does (0054).
            const auto options = juce::MessageBoxOptions()
                                     .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                     .withTitle (ui ("Remove “") + juce::String (p->name) + ui ("”?"))
                                     .withMessage ("Its notes are removed too. You can undo this.")
                                     .withButton ("Remove")
                                     .withButton ("Cancel")
                                     .withAssociatedComponent (this);
            juce::NativeMessageBox::showAsync (options, [safe = juce::Component::SafePointer<Row> (this)] (int r)
            {
                if (safe != nullptr && r == 0) safe->controller.removePart (safe->partId);
            });
        };
        up.setTooltip ("Move up");
        down.setTooltip ("Move down");
        remove.setTooltip ("Remove this part");
        theme::clicksKeepFocus (*this);
    }

    void commitVolume()
    {
        const auto v = static_cast<float> (volume.getValue());
        const auto* p = controller.score.partById (partId);
        if (p == nullptr || std::abs (p->volume - v) < 0.0001f) return;
        controller.edit ("Changed a volume", [this, v] (Score& s) { if (auto* q = s.partById (partId)) q->volume = v; });
    }

    void toggle (const std::function<void (Part&)>& fn)
    {
        controller.edit ("Changed a part", [this, fn] (Score& s) { if (auto* p = s.partById (partId)) fn (*p); });
    }

    void refresh()
    {
        const auto* p = controller.score.partById (partId);
        if (p == nullptr) return;
        if (! name.hasKeyboardFocus (true)) name.setText (p->name, juce::dontSendNotification);
        for (size_t i = 0; i < ids.size(); ++i)
            if (ids[i] == p->instrument) instrument.setSelectedId (static_cast<int> (i) + 1, juce::dontSendNotification);
        mute.setToggleState (p->mute, juce::dontSendNotification);
        solo.setToggleState (p->solo, juce::dontSendNotification);
        autoCC.setToggleState (p->autoCC, juce::dontSendNotification);
        autoCC.setEnabled (instrumentById (p->instrument).cc != CCShape::none);
        volume.setValue (p->volume, juce::dontSendNotification);
        current = controller.isPartChosen (partId);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (current ? theme::frameActive : theme::sunken);
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f);
        if (current)
        {
            g.setColour (theme::accent);
            g.fillRect (1, 4, 3, getHeight() - 8);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 6);
        auto top = r.removeFromTop (24);
        remove.setBounds (top.removeFromRight (24));
        top.removeFromRight (3);
        down.setBounds (top.removeFromRight (24));
        top.removeFromRight (3);
        up.setBounds (top.removeFromRight (24));
        top.removeFromRight (6);
        name.setBounds (top);
        r.removeFromTop (4);
        auto mid = r.removeFromTop (24);
        instrument.setBounds (mid);
        r.removeFromTop (4);
        auto bottom = r.removeFromTop (24);
        mute.setBounds (bottom.removeFromLeft (28));
        bottom.removeFromLeft (3);
        solo.setBounds (bottom.removeFromLeft (28));
        bottom.removeFromLeft (8);
        autoCC.setBounds (bottom.removeFromLeft (80));
        bottom.removeFromLeft (6);
        volume.setBounds (bottom);
    }

    void mouseDown (const juce::MouseEvent& e) override { controller.clickPart (partId, e.mods.isCommandDown(), e.mods.isShiftDown()); }

    Controller& controller;
    uint32_t partId;
    bool current = false;
    std::vector<std::string> ids;
    juce::TextEditor name;
    juce::ComboBox instrument;
    juce::TextButton mute, solo;
    IconButton up { "Move Up", IconButton::Kind::up }, down { "Move Down", IconButton::Kind::down }, remove { "Remove", IconButton::Kind::remove };
    juce::ToggleButton autoCC;
    juce::Slider volume;
};

PartsPanel::PartsPanel (Controller& c) : controller (c)
{
    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&rows, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (addButton);
    addButton.onClick = [this] { showAddMenu(); };
    addButton.setMouseClickGrabsKeyboardFocus (false);
    viewport.setTitle ("Parts");
    addAndMakeVisible (info);
    info.setJustificationType (juce::Justification::topLeft);
    info.setFont (juce::FontOptions (13.0f));
    info.setColour (juce::Label::textColourId, theme::text);
    info.setTitle ("About this instrument");
    controller.addChangeListener (this);
    rebuild();
}

PartsPanel::~PartsPanel() { controller.removeChangeListener (this); }

void PartsPanel::paint (juce::Graphics& g) { g.fillAll (theme::ground); }

void PartsPanel::resized()
{
    // Add Instrument at the top, away from the window's bottom edge.
    auto r = getLocalBounds().reduced (8);
    addButton.setBounds (r.removeFromTop (28));
    r.removeFromTop (8);
    info.setBounds (r.removeFromBottom (130));
    r.removeFromBottom (6);
    viewport.setBounds (r);
    layoutRows();
}

void PartsPanel::layoutRows()
{
    const int w = viewport.getWidth() - viewport.getScrollBarThickness();
    int y = 0;
    for (auto& row : rowList)
    {
        row->setBounds (0, y, w, 96);
        y += 100;
    }
    rows.setSize (w, y);
}

void PartsPanel::rebuild()
{
    rowList.clear();
    for (const auto& p : controller.score.parts)
    {
        auto row = std::make_unique<Row> (controller, p.id);
        rows.addAndMakeVisible (*row);
        row->refresh();
        rowList.push_back (std::move (row));
    }
    lastCount = controller.score.parts.size();
    layoutRows();
}

void PartsPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Rows are rebuilt when the parts themselves change, refreshed otherwise.
    bool same = controller.score.parts.size() == rowList.size();
    for (size_t i = 0; same && i < rowList.size(); ++i) same = rowList[i]->partId == controller.score.parts[i].id;
    if (! same) rebuild();
    else for (auto& r : rowList) r->refresh();

    if (const auto* p = controller.caretPartPtr())
    {
        const auto& inst = instrumentById (p->instrument);
        const auto ctx = keyContext (0, 0);
        juce::StringArray lines;
        lines.add (juce::String (inst.name) + "  (" + juce::String (inst.family) + ")");
        lines.add ("Range " + juce::String (pitchName (inst.low, ctx)) + " to " + juce::String (pitchName (inst.high, ctx))
                   + ", at its best " + juce::String (pitchName (inst.sweetLow, ctx)) + " to " + juce::String (pitchName (inst.sweetHigh, ctx)));
        const char* role = inst.role == 'S' ? "soprano" : inst.role == 'A' ? "alto" : inst.role == 'T' ? "tenor" : "bass";
        lines.add (juce::String (inst.monophonic() ? "Plays one line" : "Plays up to " + juce::String (inst.poly) + " notes at once")
                   + ", takes the " + role + " in harmony");
        lines.add ("Moves cleanly every " + juce::String (inst.fast * 1000.0, 0) + " ms, leaps up to " + juce::String (inst.leap) + " semitones"
                   + (inst.breath ? ", needs to breathe" : ""));
        if (inst.transposition != 0) lines.add ("Transposing instrument " + juce::String (inst.transposedName));
        if (inst.octave != 0) lines.add (juce::String ("Written an octave ") + (inst.octave > 0 ? "above" : "below") + " where it sounds");
        const char* cc = inst.cc == CCShape::strings ? "Strings" : inst.cc == CCShape::brass ? "Brass" : inst.cc == CCShape::woodwinds ? "Woodwinds"
                       : inst.cc == CCShape::neutral ? "Default" : "None (it doesn't swell)";
        lines.add ("AutoCC shape: " + juce::String (cc));
        info.setText (lines.joinIntoString ("\n"), juce::dontSendNotification);
    }
}

void PartsPanel::showAddMenu()
{
    juce::PopupMenu menu;
    int itemId = 1;
    std::vector<std::string> ids;
    for (const auto& fam : instrumentFamilies())
    {
        juce::PopupMenu sub;
        for (const auto& inst : instruments())
            if (inst.family == fam) { sub.addItem (itemId++, inst.name); ids.push_back (inst.id); }
        menu.addSubMenu (fam, sub);
    }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addButton), [this, ids] (int r)
    {
        if (r > 0 && r <= static_cast<int> (ids.size())) controller.addPart (ids[static_cast<size_t> (r - 1)]);
    });
}

//==============================================================================

StatusBar::StatusBar (Controller& c) : controller (c)
{
    setTitle ("Status");
    controller.addChangeListener (this);
    startTimerHz (2);
}

StatusBar::~StatusBar() { controller.removeChangeListener (this); }

juce::String StatusBar::message() const
{
    if (controller.input.noteInput)
        return ui ("Note input: click the staff, type A–G (⇧ adds to the chord) or play a MIDI keyboard. Press Esc to stop.");
    return controller.status;
}

void StatusBar::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // A new message is read out as well as shown, for VoiceOver.
    const auto now = message();
    if (now != announced && now.isNotEmpty())
        juce::AccessibilityHandler::postAnnouncement (now, juce::AccessibilityHandler::AnnouncementPriority::medium);
    announced = now;
    setTooltip (now);
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> StatusBar::createAccessibilityHandler()
{
    // Read as text: what the line says now.
    struct Value final : public juce::AccessibilityTextValueInterface
    {
        explicit Value (StatusBar& b) : bar (b) {}
        bool isReadOnly() const override { return true; }
        juce::String getCurrentValueAsString() const override { return bar.message(); }
        void setValueAsString (const juce::String&) override {}
        StatusBar& bar;
    };
    return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::staticText,
                                                         juce::AccessibilityActions(),
                                                         juce::AccessibilityHandler::Interfaces { std::make_unique<Value> (*this) });
}

void StatusBar::paint (juce::Graphics& g)
{
    g.fillAll (theme::popup);
    g.setColour (theme::rule);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
    auto r = getLocalBounds().reduced (10, 0);
    g.setFont (juce::FontOptions (13.0f));

    // Right: what is making the sound and what is listening.
    const int midiIns = juce::MidiInput::getAvailableDevices().size();
    const juce::String right = controller.audio.synthName() + "   |   "
                             + (midiIns == 0 ? juce::String ("No MIDI keyboard") : juce::String (midiIns) + " MIDI input" + (midiIns > 1 ? "s" : ""));

    // Middle: where the caret is and what is selected.
    juce::String where;
    if (const auto* p = controller.caretPartPtr())
    {
        const auto& s = controller.score;
        const int bar = s.barAt (controller.caret);
        const Tick inBar = controller.caret - s.barStart (bar);
        const auto beat = s.meterAtBar (bar).beatTicks();
        // With no part chosen (decision 0049) the caret is only a bar and beat.
        where = (controller.noPartChosen ? juce::String ("Every part")
                 : controller.chosenParts.size() > 1 ? juce::String (static_cast<int> (controller.chosenParts.size())) + " parts"
                 : juce::String (p->name)) + ", bar " + juce::String (bar + 1) + " beat " + juce::String (1.0 + static_cast<double> (inBar) / static_cast<double> (beat), 2);
        if (! controller.selection.empty()) where += "   |   " + juce::String (static_cast<int> (controller.selection.size())) + " selected";
    }

    // Left: the last thing that happened, or the mode. It comes first: when
    // the line is short of room - a narrow window, a large interface - the
    // sound goes, then the cursor's place moves over, before the message is
    // cut (and the whole of it is the line's tooltip).
    const auto left = message();
    const auto& font = g.getCurrentFont();
    // Measured with some to spare: text drawn at 150% or 200% runs a little wider.
    const auto width = [&font] (const juce::String& t) { return juce::roundToInt (juce::GlyphArrangement::getStringWidth (font, t) * 1.05f) + 6; };
    const int gap = 24, leftW = width (left), whereW = width (where), rightW = width (right);
    const bool showRight = leftW + whereW + rightW + 2 * gap <= r.getWidth();
    auto rest = r;
    if (showRight)
    {
        g.setColour (theme::textDim);
        g.drawText (right, rest.removeFromRight (rightW), juce::Justification::centredRight);
        rest.removeFromRight (gap);
    }
    // Where the cursor is: in the middle when there is room, at the right when not.
    auto middle = r.withSizeKeepingCentre (whereW, r.getHeight());
    if (middle.getX() < r.getX() + leftW + gap || middle.getRight() > rest.getRight())
        middle = rest.withLeft (std::max (rest.getX(), rest.getRight() - whereW));
    g.setColour (theme::text);
    g.drawText (where, middle, juce::Justification::centredLeft, true);
    const int leftEnd = where.isEmpty() ? rest.getRight() : middle.getX() - gap;

    g.setColour (controller.input.noteInput ? theme::accent : theme::text);
    g.drawText (left, r.withRight (std::max (r.getX() + 80, leftEnd)), juce::Justification::centredLeft, true);
}

} // namespace nt
