#include "Panels.h"

#include "Spelling.h"

namespace nt
{

//==============================================================================

GlyphButton::GlyphButton (const juce::String& name, juce::juce_wchar g, float s) : juce::Button (name), glyph (g), scale (s)
{
    setClickingTogglesState (false);
}

void GlyphButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
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

void TransportButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    getLookAndFeel().drawButtonBackground (g, *this, findColour (juce::TextButton::buttonColourId), highlighted, down);
    // A bar and a triangle pointing at it: back to the start, or on to the end.
    const auto r = getLocalBounds().toFloat().withSizeKeepingCentre (14.0f, 12.0f);
    const bool start = kind == Kind::start;
    juce::Path p;
    if (start)
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
    for (auto* b : { &fileButton, &playButton, &followButton,
                     &voiceButton, &transposeButton, &zoomOut, &zoomIn })
        addAndMakeVisible (b);

    fileButton.setTooltip ("New, Open, Save, Export, Undo and Redo, the score's settings, the sound, note input and the dark page");
    playButton.setTooltip ("Play from the caret, or stop (Shift+Space; Space plays from bar 1)");
    startButton.setTooltip ("Return to the start (Home) - if it is playing, it plays on from bar 1");
    endButton.setTooltip ("Skip to the end of the music (End)");
    followButton.setTooltip ("Follow: while it plays, the page scrolls along with the music (or turns a page at a time - Play menu)");
    voiceButton.setTooltip ("Which voice notes are written in: 1 stems up, 2 stems down (V changes the selection's)");
    transposeButton.setTooltip ("Show the score at concert pitch, or as the transposing instruments read it");

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
        b->setTooltip (juce::String (d.name) + " (" + d.key + ")");
        const Tick t = d.ticks;
        b->onClick = [this, t] { controller.setDuration (t); };
        addAndMakeVisible (*b);
        durations.push_back (std::move (b));
    }
    for (auto* b : { &dotButton, &tripletButton, &restButton }) addAndMakeVisible (b);
    dotButton.setTooltip ("Dotted (.)");
    tripletButton.setTooltip ("Triplet (T)");
    restButton.setTooltip ("Write a rest at the caret (0)");
    dotButton.onClick = [this] { controller.toggleDot(); };
    tripletButton.onClick = [this] { controller.toggleTriplet(); };
    restButton.onClick = [this] { controller.typeRest(); };

    controller.addChangeListener (this);
    refresh();
}

Toolbar::~Toolbar() { controller.removeChangeListener (this); }

void Toolbar::paint (juce::Graphics& g)
{
    g.fillAll (theme::popup);
    g.setColour (theme::rule);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

void Toolbar::resized()
{
    auto r = getLocalBounds().reduced (8, 7);
    auto place = [&r] (juce::Component& c, int w, int gap = 4) { c.setBounds (r.removeFromLeft (w)); r.removeFromLeft (gap); };
    place (fileButton, 52, 14);
    place (startButton, 30, 2);
    place (playButton, 60, 2);
    place (endButton, 30, 6);
    place (followButton, 60, 14);
    for (auto& d : durations) place (*d, 30, 2);
    r.removeFromLeft (4);
    place (dotButton, 30, 2); place (tripletButton, 30, 2); place (restButton, 30, 6);
    place (voiceButton, 66, 14);
    auto right = r;
    zoomIn.setBounds (right.removeFromRight (28));
    right.removeFromRight (2);
    zoomOut.setBounds (right.removeFromRight (28));
    right.removeFromRight (10);
    transposeButton.setBounds (right.removeFromRight (106));
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
    transposeButton.setButtonText (controller.transposedScore ? "Transposed" : "Concert pitch");
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
        autoCC.setTooltip ("Draw this part's dynamics, expression and vibrato curves automatically (AutoCC)");
        mute.onClick = [this] { toggle ([] (Part& p) { p.mute = ! p.mute; }); };
        solo.onClick = [this] { toggle ([] (Part& p) { p.solo = ! p.solo; }); };
        autoCC.onClick = [this] { toggle ([] (Part& p) { p.autoCC = ! p.autoCC; }); };
        volume.setSliderStyle (juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        volume.setRange (0.0, 1.0);
        volume.setTooltip ("Volume");
        volume.onDragEnd = [this]
        {
            const auto v = static_cast<float> (volume.getValue());
            controller.edit ("Changed a volume", [this, v] (Score& s) { if (auto* p = s.partById (partId)) p->volume = v; });
        };
        up.onClick = [this] { controller.movePart (partId, -1); };
        down.onClick = [this] { controller.movePart (partId, 1); };
        remove.onClick = [this]
        {
            const auto* p = controller.score.partById (partId);
            if (p == nullptr) return;
            if (p->notes.empty()) { controller.removePart (partId); return; }
            juce::NativeMessageBox::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Remove " + juce::String (p->name) + "?",
                                                     "Its notes go with it. Undo brings them back.", this,
                                                     juce::ModalCallbackFunction::create ([this] (int r) { if (r != 0) controller.removePart (partId); }));
        };
        up.setTooltip ("Move up");
        down.setTooltip ("Move down");
        remove.setTooltip ("Remove this part");
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
        current = controller.caretPart == partId && ! controller.noPartChosen;
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

    void mouseDown (const juce::MouseEvent&) override { controller.choosePart (partId); }

    Controller& controller;
    uint32_t partId;
    bool current = false;
    std::vector<std::string> ids;
    juce::TextEditor name;
    juce::ComboBox instrument;
    juce::TextButton mute, solo, up { "^" }, down { "v" }, remove { "x" };
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
    addAndMakeVisible (info);
    info.setJustificationType (juce::Justification::topLeft);
    info.setFont (juce::FontOptions (13.0f));
    info.setColour (juce::Label::textColourId, theme::text);
    controller.addChangeListener (this);
    rebuild();
}

PartsPanel::~PartsPanel() { controller.removeChangeListener (this); }

void PartsPanel::paint (juce::Graphics& g) { g.fillAll (theme::ground); }

void PartsPanel::resized()
{
    auto r = getLocalBounds().reduced (8);
    info.setBounds (r.removeFromBottom (150));
    r.removeFromBottom (6);
    addButton.setBounds (r.removeFromBottom (28));
    r.removeFromBottom (8);
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
                       : inst.cc == CCShape::neutral ? "Default" : "none - it does not swell";
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
    controller.addChangeListener (this);
    startTimerHz (2);
}

StatusBar::~StatusBar() { controller.removeChangeListener (this); }

void StatusBar::paint (juce::Graphics& g)
{
    g.fillAll (theme::popup);
    g.setColour (theme::rule);
    g.fillRect (0, 0, getWidth(), 1);
    auto r = getLocalBounds().reduced (10, 0);
    g.setFont (juce::FontOptions (13.0f));

    // Right: what is making the sound and what is listening.
    const int midiIns = juce::MidiInput::getAvailableDevices().size();
    const juce::String right = controller.audio.synthName() + "   |   "
                             + (midiIns == 0 ? juce::String ("no MIDI keyboard") : juce::String (midiIns) + " MIDI input" + (midiIns > 1 ? "s" : ""));
    g.setColour (theme::textDim);
    g.drawText (right, r, juce::Justification::centredRight);

    // Middle: where the caret is and what is selected.
    juce::String where;
    if (const auto* p = controller.caretPartPtr())
    {
        const auto& s = controller.score;
        const int bar = s.barAt (controller.caret);
        const Tick inBar = controller.caret - s.barStart (bar);
        const auto beat = s.meterAtBar (bar).beatTicks();
        // With no part chosen (decision 0048) the caret is only a bar and beat.
        where = (controller.noPartChosen ? juce::String ("Every part") : juce::String (p->name)) + ", bar " + juce::String (bar + 1) + " beat " + juce::String (1.0 + static_cast<double> (inBar) / static_cast<double> (beat), 2);
        if (! controller.selection.empty()) where += "   |   " + juce::String (static_cast<int> (controller.selection.size())) + " selected";
    }
    g.setColour (theme::text);
    const auto middle = r.withTrimmedRight (320);
    g.drawText (where, middle, juce::Justification::centred);
    // The message on the left stops short of the middle, ending in "..." if
    // it is too long, rather than running into it.
    const int whereWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), where));
    const int leftEnd = where.isEmpty() ? r.getRight() - 320 : middle.getCentreX() - whereWidth / 2 - 16;

    // Left: the last thing that happened, or the mode.
    juce::String left = controller.status;
    if (controller.input.noteInput) left = "NOTE INPUT - click the staff, type A-G (Shift adds to the chord), or play a MIDI keyboard. Esc to stop.";
    g.setColour (controller.input.noteInput ? theme::accent : theme::text);
    g.drawText (left, r.withRight (std::max (r.getX() + 80, leftEnd)), juce::Justification::centredLeft, true);
}

} // namespace nt
