#include "MainComponent.h"

#include "Exporter.h"
#include "ScaleModel.h"
#include "Templates.h"

namespace nt
{

namespace
{
enum MenuIds
{
    menuNew = 1, menuOpen, menuImport, menuSave, menuSaveAs, menuExportMidi, menuExportMidiBars, menuExportAudio, menuExportAudioBars,
    menuExportXml, menuExportXmlBars,
    menuUndo = 100, menuRedo, menuCut, menuCopy, menuPaste, menuDelete, menuSelectAll, menuVoice,
    menuPlay = 200, menuPlayCaret, menuToStart, menuToEnd, menuFollow, menuFollowSmooth, menuFollowPage, menuNoteInput, menuLightPage, menuTransposed, menuZoomIn, menuZoomOut, menuAudioSettings, menuHelp,
    menuTitle = 400, menuTempo, menuMeterOther, menuKeyHeard, menuInsertBar, menuDeleteBars, menuAddBars, menuSoundApple, menuSoundBuiltIn,
    menuMeterBase = 500,
    menuTemplateBase = 1000,
    menuKeyBase = 2000           // + root x 32 + scale
};

// The time signatures offered in the menu; anything else is under Other...
const std::vector<std::pair<int, int>>& meterChoices()
{
    static const std::vector<std::pair<int, int>> m { { 2, 2 }, { 2, 4 }, { 3, 4 }, { 4, 4 }, { 5, 4 }, { 6, 4 }, { 7, 4 },
                                                     { 3, 8 }, { 5, 8 }, { 6, 8 }, { 7, 8 }, { 9, 8 }, { 12, 8 } };
    return m;
}

juce::String key (const juce::String& k)
{
   #if JUCE_MAC
    return k.replace ("Cmd", juce::String::charToString (0x2318));
   #else
    return k.replace ("Cmd", "Ctrl");
   #endif
}
} // namespace

MainComponent::MainComponent()
{
    addAndMakeVisible (toolbar);
    addAndMakeVisible (view);
    addAndMakeVisible (tabs);
    addAndMakeVisible (statusBar);
    tabs.setTabBarDepth (30);
    tabs.setOutline (0);
    tabs.addTab ("Generate", theme::control, &generatorPanel, false);
    tabs.addTab ("Blocks", theme::control, &blocksPanel, false);
    tabs.addTab ("Parts", theme::control, &partsPanel, false);
    tabs.setColour (juce::TabbedComponent::backgroundColourId, theme::ground);

    toolbar.onFile = [this] { showFileMenu(); };
    toolbar.onStart = [this] { returnToStart(); };
    toolbar.onEnd = [this] { skipToEnd(); };

    // A MIDI keyboard writes in note input, and is always heard.
    audio.onMidiInput = [this] (const juce::MidiMessage& m)
    {
        if (! m.isNoteOn() || ! controller.input.noteInput) return;
        // Keys pressed within a moment of each other are one chord.
        static double lastTime = 0;
        const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        const bool chord = now - lastTime < 0.06;
        lastTime = now;
        controller.writePitch (m.getNoteNumber(), chord);
    };
    audio.onPlaybackFinished = [this] { controller.stop(); };

    juce::PropertiesFile::Options prefs;
    prefs.applicationName = "Noterator";
    prefs.filenameSuffix = "settings";
    prefs.osxLibrarySubFolder = "Application Support";
    preferences.setStorageParameters (prefs);
    if (auto* p = preferences.getUserSettings())
    {
        controller.lightPage = ! p->getBoolValue ("darkPage", false);
        controller.followPlayback = p->getBoolValue ("followPlayback", true);
        controller.followStyle = p->getValue ("followStyle", "smooth") == "page" ? FollowStyle::page : FollowStyle::smooth;
        controller.zoom = static_cast<float> (juce::jlimit (5.0, 24.0, p->getDoubleValue ("zoom", controller.zoom)));
        controller.viewChanged();
    }

    controller.addChangeListener (this);
    setWantsKeyboardFocus (true);
    setSize (1440, 880);
   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (this);
   #endif
    updateTitle();
}

MainComponent::~MainComponent()
{
   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (nullptr);
   #endif
    controller.removeChangeListener (this);
    audio.stop();
}

void MainComponent::paint (juce::Graphics& g) { g.fillAll (theme::ground); }

void MainComponent::resized()
{
    auto r = getLocalBounds();
    toolbar.setBounds (r.removeFromTop (46));
    statusBar.setBounds (r.removeFromBottom (26));
    tabs.setBounds (r.removeFromRight (std::min (380, getWidth() / 3)));
    view.setBounds (r);
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (auto* p = preferences.getUserSettings())
    {
        if (p->getBoolValue ("darkPage", false) == controller.lightPage) p->setValue ("darkPage", ! controller.lightPage);
        if (p->getBoolValue ("followPlayback", true) != controller.followPlayback) p->setValue ("followPlayback", controller.followPlayback);
        const juce::String style = controller.followStyle == FollowStyle::page ? "page" : "smooth";
        if (p->getValue ("followStyle", "smooth") != style) p->setValue ("followStyle", style);
        if (std::abs (p->getDoubleValue ("zoom", 0.0) - controller.zoom) > 0.01) p->setValue ("zoom", controller.zoom);
    }
    updateTitle();
    menuItemsChanged();
}

void MainComponent::updateTitle()
{
    if (auto* w = findParentComponentOfClass<juce::DocumentWindow>())
        w->setName ((controller.dirty ? "* " : "") + juce::String (controller.score.title) + " - Noterator");
}

//==============================================================================
// Keys

bool MainComponent::keyPressed (const juce::KeyPress& k)
{
    auto& c = controller;
    const auto mods = k.getModifiers();
    // Letters can arrive as either case depending on the platform and the
    // modifiers held; compare them as capitals.
    const int rawCode = k.getKeyCode();
    const int code = (rawCode >= 'a' && rawCode <= 'z') ? rawCode - 'a' + 'A' : rawCode;
    const auto ch = k.getTextCharacter();
    const bool cmd = mods.isCommandDown();

    if (cmd)
    {
        if (code == 'Z') { mods.isShiftDown() ? c.redo() : c.undo(); return true; }
        if (code == 'Y') { c.redo(); return true; }
        if (code == 'S') { saveDialog (mods.isShiftDown()); return true; }
        if (code == 'O') { openDialog(); return true; }
        if (code == 'N') { showNewMenu(); return true; }
        if (code == 'E') { showExportMenu(); return true; }
        if (code == 'A') { c.selectAll(); return true; }
        if (code == 'C') { c.copySelection(); return true; }
        if (code == 'X') { c.cutSelection(); return true; }
        if (code == 'V') { c.paste(); return true; }
        if (code == '=' || code == '+') { view.zoomBy (1.15f); return true; }
        if (code == '-') { view.zoomBy (1.0f / 1.15f); return true; }
        if (code == juce::KeyPress::upKey) { c.transposeSelection (12); return true; }
        if (code == juce::KeyPress::downKey) { c.transposeSelection (-12); return true; }
        return false;
    }

    // Space from bar 1, Shift+Space from the caret (decision 0035).
    if (code == juce::KeyPress::spaceKey) { c.togglePlay (! mods.isShiftDown()); return true; }
    if (code == juce::KeyPress::homeKey) { returnToStart(); return true; }
    if (code == juce::KeyPress::endKey) { skipToEnd(); return true; }
    // Escape lets go of everything, the caret's part too (decision 0048).
    if (code == juce::KeyPress::escapeKey) { c.letGoOfEverything(); return true; }
    if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey) { c.deleteSelection(); return true; }
    if (code == juce::KeyPress::upKey)
    {
        if (mods.isAltDown()) c.caretToPart (-1);
        else c.transposeSelection (1);
        return true;
    }
    if (code == juce::KeyPress::downKey)
    {
        if (mods.isAltDown()) c.caretToPart (1);
        else c.transposeSelection (-1);
        return true;
    }
    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey)
    {
        const int dir = code == juce::KeyPress::leftKey ? -1 : 1;
        if (mods.isAltDown()) c.moveSelection (dir);
        else if (c.input.noteInput || c.selection.empty()) c.moveCaret (dir);
        else c.selectNext (dir, mods.isShiftDown());
        view.scrollToTick (c.caret);
        return true;
    }
    if (code == juce::KeyPress::returnKey)
    {
        c.toggleNoteInput();
        return true;
    }

    const auto lower = juce::CharacterFunctions::toLowerCase (ch);
    if (lower >= 'a' && lower <= 'g')
    {
        static const int letterOf[7] = { 5, 6, 0, 1, 2, 3, 4 };   // a..g -> A..G as C-based letters
        if (! c.input.noteInput) c.toggleNoteInput();
        c.typeLetter (letterOf[lower - 'a'], mods.isShiftDown());
        view.scrollToTick (c.caret);
        return true;
    }
    if (ch >= '1' && ch <= '7')
    {
        static const Tick values[7] = { PPQ / 16, PPQ / 8, PPQ / 4, PPQ / 2, PPQ, 2 * PPQ, 4 * PPQ };
        c.setDuration (values[ch - '1']);
        return true;
    }
    if (ch == '0') { c.typeRest(); view.scrollToTick (c.caret); return true; }
    if (ch == '.') { c.toggleDot(); return true; }
    if (lower == 't') { c.toggleTriplet(); return true; }
    if (lower == 'n') { c.toggleNoteInput(); return true; }
    if (lower == 'v') { c.toggleVoiceOfSelection(); return true; }
    if (lower == 'p') { c.previewSelection(); return true; }
    if (ch == '[' || ch == ']') { c.lengthenSelection (ch == ']' ? 1 : -1); return true; }
    if (lower == 'h' || ch == '?') { showHelp(); return true; }
    return false;
}

//==============================================================================
// Menus

juce::StringArray MainComponent::getMenuBarNames() { return { "File", "Edit", "View", "Play", "Help" }; }

juce::PopupMenu MainComponent::getMenuForIndex (int index, const juce::String&)
{
    juce::PopupMenu m;
    auto item = [&m] (int id, const juce::String& text, const juce::String& shortcut = {}, bool enabled = true)
    {
        juce::PopupMenu::Item i (text);
        i.itemID = id;
        i.isEnabled = enabled;
        if (shortcut.isNotEmpty()) i.shortcutKeyDescription = key (shortcut);
        m.addItem (i);
    };
    if (index == 0)
    {
        m.addSubMenu ("New", templateMenu());
        item (menuOpen, "Open...", "Cmd+O");
        item (menuImport, "Import MIDI or MusicXML into this score...");
        m.addSeparator();
        item (menuSave, "Save", "Cmd+S");
        item (menuSaveAs, "Save As...", "Shift+Cmd+S");
        m.addSeparator();
        item (menuExportMidi, "Export Score as MIDI...");
        item (menuExportMidiBars, "Export Selected Bars as MIDI...");
        item (menuExportAudio, "Export Score as Audio (WAV)...");
        item (menuExportAudioBars, "Export Selected Bars as Audio (WAV)...");
        item (menuExportXml, "Export Score as MusicXML...");
        item (menuExportXmlBars, "Export Selected Bars as MusicXML...");
        addScoreItems (m);
    }
    else if (index == 1)
    {
        item (menuUndo, "Undo", "Cmd+Z", controller.canUndo());
        item (menuRedo, "Redo", "Shift+Cmd+Z", controller.canRedo());
        m.addSeparator();
        item (menuCut, "Cut", "Cmd+X", ! controller.selection.empty());
        item (menuCopy, "Copy", "Cmd+C", ! controller.selection.empty());
        item (menuPaste, "Paste at the Caret", "Cmd+V");
        item (menuDelete, "Delete", "Delete", ! controller.selection.empty());
        item (menuSelectAll, "Select All", "Cmd+A");
        m.addSeparator();
        item (menuVoice, "Swap Voice of Selection", "V", ! controller.selection.empty());
    }
    else if (index == 2)
    {
        m.addItem (menuNoteInput, "Note Input", true, controller.input.noteInput);
        m.addItem (menuLightPage, "Dark Page", true, ! controller.lightPage);
        m.addItem (menuTransposed, "Transposed Score", true, controller.transposedScore);
        m.addSeparator();
        item (menuZoomIn, "Zoom In", "Cmd+=");
        item (menuZoomOut, "Zoom Out", "Cmd+-");
    }
    else if (index == 3)
    {
        if (controller.audio.isPlaying()) item (menuPlay, "Stop", "Space");
        else
        {
            item (menuPlay, "Play from the Start", "Space");
            item (menuPlayCaret, "Play from the Caret", "Shift+Space");
        }
        item (menuToStart, "Return to Start", "Home");
        item (menuToEnd, "Skip to End", "End");
        m.addSeparator();
        m.addItem (menuFollow, "Follow the Playhead", true, controller.followPlayback);
        m.addItem (menuFollowSmooth, "    Scroll Along with the Music", true, controller.followPlayback && controller.followStyle == FollowStyle::smooth);
        m.addItem (menuFollowPage, "    Turn a Page at a Time", true, controller.followPlayback && controller.followStyle == FollowStyle::page);
        m.addSeparator();
        item (menuAudioSettings, "Audio and MIDI Devices...");
    }
    else
    {
        item (menuHelp, "Keyboard Shortcuts", "H");
    }
    return m;
}

void MainComponent::menuItemSelected (int id, int)
{
    auto& c = controller;
    if (id >= menuMeterBase && id < menuMeterBase + static_cast<int> (meterChoices().size()))
    {
        const auto [num, den] = meterChoices()[static_cast<size_t> (id - menuMeterBase)];
        c.setMeterAt (c.score.barAt (c.caret), num, den);
        return;
    }
    if (id >= menuKeyBase)
    {
        c.setKeyAt (c.score.barAt (c.caret), (id - menuKeyBase) / 32, (id - menuKeyBase) % 32);
        return;
    }
    if (id >= menuTemplateBase)
    {
        const auto name = Controller::templates()[id - menuTemplateBase];
        checkSaved ([this, name] { controller.newScore (name); });
        return;
    }
    switch (id)
    {
        case menuOpen: openDialog(); break;
        case menuImport: importDialog(); break;
        case menuSave: saveDialog (false); break;
        case menuSaveAs: saveDialog (true); break;
        case menuExportMidi: exportDialog (ExportKind::midi, false); break;
        case menuExportMidiBars: exportDialog (ExportKind::midi, true); break;
        case menuExportAudio: exportDialog (ExportKind::audio, false); break;
        case menuExportAudioBars: exportDialog (ExportKind::audio, true); break;
        case menuExportXml: exportDialog (ExportKind::musicXml, false); break;
        case menuExportXmlBars: exportDialog (ExportKind::musicXml, true); break;
        case menuUndo: c.undo(); break;
        case menuRedo: c.redo(); break;
        case menuCut: c.cutSelection(); break;
        case menuCopy: c.copySelection(); break;
        case menuPaste: c.paste(); break;
        case menuDelete: c.deleteSelection(); break;
        case menuSelectAll: c.selectAll(); break;
        case menuVoice: c.toggleVoiceOfSelection(); break;
        case menuNoteInput: c.toggleNoteInput(); break;
        case menuLightPage: c.lightPage = ! c.lightPage; c.viewChanged(); break;
        case menuTransposed: c.transposedScore = ! c.transposedScore; c.viewChanged(); break;
        case menuZoomIn: view.zoomBy (1.15f); break;
        case menuZoomOut: view.zoomBy (1.0f / 1.15f); break;
        case menuPlay: c.togglePlay (true); break;
        case menuPlayCaret: c.togglePlay (false); break;
        case menuToStart: returnToStart(); break;
        case menuToEnd: skipToEnd(); break;
        case menuFollow: c.toggleFollow(); break;
        case menuFollowSmooth: c.setFollowStyle (FollowStyle::smooth); break;
        case menuFollowPage: c.setFollowStyle (FollowStyle::page); break;
        case menuAudioSettings: audioSettingsDialog(); break;
        case menuHelp: showHelp(); break;
        case menuTitle: titleDialog(); break;
        case menuTempo: tempoDialog(); break;
        case menuMeterOther: meterDialog(); break;
        case menuKeyHeard: c.useHeardKey(); break;
        case menuInsertBar: c.insertBarsAtCaret (1); break;
        case menuDeleteBars: c.deleteSelectedBars(); break;
        case menuAddBars: c.setBars (c.score.bars + 4); break;
        case menuSoundApple: audio.useBuiltInSynth (false); c.setStatus ("Sound: " + audio.synthName()); break;
        case menuSoundBuiltIn: audio.useBuiltInSynth (true); c.setStatus ("Sound: " + audio.synthName()); break;
        default: break;
    }
}

juce::PopupMenu MainComponent::templateMenu()
{
    // Grouped under headings, each with how many parts it starts with.
    juce::PopupMenu m;
    std::string group = "-";
    const auto& list = scoreTemplates();
    for (size_t i = 0; i < list.size(); ++i)
    {
        const auto& t = list[i];
        if (t.group != group)
        {
            if (t.group.empty()) m.addSeparator();
            else m.addSectionHeader (t.group);
            group = t.group;
        }
        juce::PopupMenu::Item item (t.name);
        item.itemID = menuTemplateBase + static_cast<int> (i);
        const auto n = t.instruments.size();
        if (n > 1) item.shortcutKeyDescription = juce::String (static_cast<int> (n)) + " parts";
        m.addItem (item);
    }
    return m;
}

void MainComponent::showNewMenu()
{
    auto m = templateMenu();
    m.showMenuAsync (juce::PopupMenu::Options(), [this] (int r) { if (r > 0) menuItemSelected (r, 0); });
}

void MainComponent::showFileMenu()
{
    // New, Open, Save and Export under one button, so the toolbar keeps room
    // for what is used while writing (decision 0037); then undo, the score's
    // settings (0038), the sound, the input mode and the look (0039).
    juce::PopupMenu m;
    auto item = [&m] (int id, const juce::String& text, const juce::String& shortcut = {})
    {
        juce::PopupMenu::Item i (text);
        i.itemID = id;
        if (shortcut.isNotEmpty()) i.shortcutKeyDescription = key (shortcut);
        m.addItem (i);
    };
    m.addSubMenu ("New", templateMenu());
    item (menuOpen, "Open...", "Cmd+O");
    item (menuImport, "Import MIDI or MusicXML...");
    m.addSeparator();
    item (menuSave, "Save", "Cmd+S");
    item (menuSaveAs, "Save As...", "Shift+Cmd+S");
    m.addSeparator();
    m.addSubMenu ("Export", exportMenu());
    m.addSeparator();
    {
        juce::PopupMenu::Item i ("Undo");
        i.itemID = menuUndo;
        i.isEnabled = controller.canUndo();
        i.shortcutKeyDescription = key ("Cmd+Z");
        m.addItem (i);
        juce::PopupMenu::Item r ("Redo");
        r.itemID = menuRedo;
        r.isEnabled = controller.canRedo();
        r.shortcutKeyDescription = key ("Shift+Cmd+Z");
        m.addItem (r);
    }
    addScoreItems (m);
    m.addSeparator();
    {
        juce::PopupMenu::Item i ("Note input");
        i.itemID = menuNoteInput;
        i.isTicked = controller.input.noteInput;
        i.shortcutKeyDescription = "N";
        m.addItem (i);
    }
    m.addItem (menuLightPage, "Dark page", true, ! controller.lightPage);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&toolbar.fileAnchor()),
                     [this] (int r) { if (r > 0) menuItemSelected (r, 0); });
}

void MainComponent::addScoreItems (juce::PopupMenu& m)
{
    // What was the Score tab: the score's name, tempo, time and key
    // signatures, bars and sound (decision 0038). Signatures are set at the
    // caret's bar, as the tab's buttons did.
    const auto& s = controller.score;
    const int bar = s.barAt (controller.caret);
    m.addSeparator();
    m.addSectionHeader ("This score");
    m.addItem (menuTitle, "Title and composer...");
    m.addItem (menuTempo, "Tempo (" + juce::String (juce::roundToInt (s.tempos.front().bpm)) + " bpm)...");

    const auto& meter = s.meterAtBar (bar);
    juce::PopupMenu meters;
    bool listed = false;
    for (size_t i = 0; i < meterChoices().size(); ++i)
    {
        const auto [num, den] = meterChoices()[i];
        const bool on = num == meter.num && den == meter.den;
        listed = listed || on;
        meters.addItem (menuMeterBase + static_cast<int> (i), juce::String (num) + "/" + juce::String (den), true, on);
    }
    meters.addSeparator();
    meters.addItem (menuMeterOther, "Other...", true, ! listed);
    m.addSubMenu ("Time signature at bar " + juce::String (bar + 1) + " (" + juce::String (meter.num) + "/" + juce::String (meter.den) + ")", meters);

    const auto& k = s.keyAtBar (bar);
    juce::PopupMenu keys;
    keys.addItem (menuKeyHeard, "Use the key it hears");
    keys.addSeparator();
    for (size_t r = 0; r < scaleview::roots.size(); ++r)
    {
        juce::PopupMenu sc;
        for (size_t i = 0; i < scaleview::scales.size(); ++i)
            sc.addItem (menuKeyBase + static_cast<int> (r) * 32 + static_cast<int> (i), scaleview::scales[i].name, true,
                        static_cast<int> (r) == k.root && static_cast<int> (i) == k.scale);
        keys.addSubMenu (scaleview::roots[r].name, sc, true, nullptr, static_cast<int> (r) == k.root);
    }
    juce::String keyName = "?";
    if (k.root >= 0 && k.root < static_cast<int> (scaleview::roots.size()) && k.scale >= 0 && k.scale < static_cast<int> (scaleview::scales.size()))
        keyName = juce::String (scaleview::roots[static_cast<size_t> (k.root)].name) + " " + scaleview::scales[static_cast<size_t> (k.scale)].name;
    m.addSubMenu ("Key at bar " + juce::String (bar + 1) + " (" + keyName + ")", keys);

    juce::PopupMenu bars;
    const auto [a, b] = controller.selectedBars();
    bars.addItem (menuInsertBar, "Insert a bar at the caret (before bar " + juce::String (bar + 1) + ")");
    bars.addItem (menuDeleteBars, a == b ? "Delete bar " + juce::String (a + 1) : "Delete bars " + juce::String (a + 1) + "-" + juce::String (b + 1));
    bars.addItem (menuAddBars, "Add 4 bars at the end");
    m.addSubMenu ("Bars (" + juce::String (s.bars) + " in all)", bars);

    m.addSeparator();
    juce::PopupMenu sound;
    sound.addItem (menuSoundApple, "Apple General MIDI (macOS)", true, ! audio.usingBuiltInSynth());
    sound.addItem (menuSoundBuiltIn, "Built-in synth", true, audio.usingBuiltInSynth());
    sound.addSeparator();
    sound.addItem (menuAudioSettings, "Audio and MIDI devices...");
    m.addSubMenu ("Sound", sound);
}

namespace
{
// A small box to type into, with OK and Cancel; `done` gets the box while it
// is still there to read.
void askFor (const juce::String& title, const juce::String& message, std::function<void (juce::AlertWindow&)> fill,
             std::function<void (juce::AlertWindow&)> done)
{
    auto* w = new juce::AlertWindow (title, message, juce::MessageBoxIconType::NoIcon);
    fill (*w);
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([w, done] (int r) { if (r == 1) done (*w); }), true);
}
} // namespace

void MainComponent::titleDialog()
{
    askFor ("Title and composer", {}, [this] (juce::AlertWindow& w)
    {
        w.addTextEditor ("title", controller.score.title, "Title");
        w.addTextEditor ("composer", controller.score.composer, "Composer");
    },
    [this] (juce::AlertWindow& w)
    {
        const auto t = w.getTextEditorContents ("title").trim().toStdString();
        const auto who = w.getTextEditorContents ("composer").trim().toStdString();
        if (t != controller.score.title || who != controller.score.composer)
            controller.edit ("Retitled", [t, who] (Score& s) { s.title = t; s.composer = who; });
    });
}

void MainComponent::tempoDialog()
{
    askFor ("Tempo", "Beats a minute, 30 to 240", [this] (juce::AlertWindow& w)
    {
        w.addTextEditor ("bpm", juce::String (juce::roundToInt (controller.score.tempos.front().bpm)), "Tempo");
        if (auto* e = w.getTextEditor ("bpm")) e->setInputRestrictions (3, "0123456789");
    },
    [this] (juce::AlertWindow& w)
    {
        const int bpm = w.getTextEditorContents ("bpm").getIntValue();
        if (bpm < 30 || bpm > 240) { controller.setStatus ("The tempo stays as it was: it can be 30 to 240."); return; }
        controller.setTempo (bpm);
    });
}

void MainComponent::meterDialog()
{
    const int bar = controller.score.barAt (controller.caret);
    askFor ("Time signature at bar " + juce::String (bar + 1), {}, [this, bar] (juce::AlertWindow& w)
    {
        const auto& m = controller.score.meterAtBar (bar);
        juce::StringArray nums, dens { "2", "4", "8", "16" };
        for (int n = 1; n <= 16; ++n) nums.add (juce::String (n));
        w.addComboBox ("num", nums, "Beats in a bar");
        w.addComboBox ("den", dens, "Each beat a");
        if (auto* b = w.getComboBoxComponent ("num")) b->setText (juce::String (m.num), juce::dontSendNotification);
        if (auto* b = w.getComboBoxComponent ("den")) b->setText (juce::String (m.den), juce::dontSendNotification);
    },
    [this, bar] (juce::AlertWindow& w)
    {
        const auto* n = w.getComboBoxComponent ("num");
        const auto* d = w.getComboBoxComponent ("den");
        if (n != nullptr && d != nullptr) controller.setMeterAt (bar, n->getText().getIntValue(), d->getText().getIntValue());
    });
}

void MainComponent::showExportMenu()
{
    exportMenu().showMenuAsync (juce::PopupMenu::Options(), [this] (int r) { if (r > 0) menuItemSelected (r, 0); });
}

juce::PopupMenu MainComponent::exportMenu()
{
    juce::PopupMenu m;
    const auto [a, b] = controller.selectedBars();
    const juce::String bars = a == b ? "bar " + juce::String (a + 1) : "bars " + juce::String (a + 1) + "-" + juce::String (b + 1);
    m.addItem (menuExportMidi, "Score as MIDI...");
    m.addItem (menuExportMidiBars, "Selection (" + bars + ") as MIDI...");
    m.addSeparator();
    m.addItem (menuExportAudio, "Score as audio (WAV)...");
    m.addItem (menuExportAudioBars, "Selection (" + bars + ") as audio (WAV)...");
    m.addSeparator();
    m.addItem (menuExportXml, "Score as MusicXML (for Dorico, MuseScore, Sibelius)...");
    m.addItem (menuExportXmlBars, "Selection (" + bars + ") as MusicXML...");
    return m;
}

//==============================================================================
// Files

void MainComponent::checkSaved (std::function<void()> then)
{
    if (! controller.dirty) { then(); return; }
    auto opts = juce::MessageBoxOptions()
                    .withIconType (juce::MessageBoxIconType::QuestionIcon)
                    .withTitle ("Save the changes to " + juce::String (controller.score.title) + "?")
                    .withMessage ("They will be lost if you don't.")
                    .withButton ("Save")
                    .withButton ("Don't Save")
                    .withButton ("Cancel")
                    .withAssociatedComponent (this);
    juce::AlertWindow::showAsync (opts, [this, then] (int r)
    {
        if (r == 1) saveDialog (false, then);
        else if (r == 2) then();
    });
}

void MainComponent::openFile (const juce::File& f)
{
    juce::String error;
    if (! controller.load (f, error))
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not open " + f.getFileName(), error);
    lastFolder = f.getParentDirectory();
}

void MainComponent::openDialog()
{
    checkSaved ([this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Open a score, a MIDI file or a MusicXML file", lastFolder,
                                                      "*.noterator;*.mid;*.midi;*.musicxml;*.mxl;*.xml");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile()) openFile (f);
        });
    });
}

void MainComponent::importDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Import a MIDI or MusicXML file into this score, at the caret's bar", lastFolder,
                                                  "*.mid;*.midi;*.musicxml;*.mxl;*.xml");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (! f.existsAsFile()) return;
        juce::String error;
        if (! controller.importFile (f, error))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not import " + f.getFileName(), error);
        lastFolder = f.getParentDirectory();
    });
}

void MainComponent::saveDialog (bool saveAs, std::function<void()> then)
{
    if (! saveAs && controller.file.existsAsFile())
    {
        juce::String error;
        if (controller.save (controller.file, error)) { if (then) then(); }
        else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not save", error);
        return;
    }
    const auto suggested = (lastFolder.isDirectory() ? lastFolder : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory))
                               .getChildFile (juce::File::createLegalFileName (controller.score.title) + ".noterator");
    chooser = std::make_unique<juce::FileChooser> ("Save the score", suggested, "*.noterator");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, then] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        f = f.withFileExtension ("noterator");
        juce::String error;
        if (controller.save (f, error)) { lastFolder = f.getParentDirectory(); if (then) then(); }
        else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not save", error);
    });
}

void MainComponent::exportDialog (ExportKind kind, bool selectedBars)
{
    ExportRange range;
    juce::String suffix;
    if (selectedBars)
    {
        const auto [a, b] = controller.selectedBars();
        range.from = controller.score.barStart (a);
        range.to = controller.score.barStart (b + 1);
        suffix = a == b ? " bar " + juce::String (a + 1) : " bars " + juce::String (a + 1) + "-" + juce::String (b + 1);
    }
    const bool audioFile = kind == ExportKind::audio;
    const juce::String ext = audioFile ? "wav" : kind == ExportKind::musicXml ? "musicxml" : "mid";
    const auto suggested = (lastFolder.isDirectory() ? lastFolder : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory))
                               .getChildFile (juce::File::createLegalFileName (juce::String (controller.score.title) + suffix) + "." + ext);
    chooser = std::make_unique<juce::FileChooser> (audioFile ? "Export audio" : kind == ExportKind::musicXml ? "Export MusicXML" : "Export MIDI",
                                                   suggested, "*." + ext);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, kind, audioFile, range, ext] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        f = f.withFileExtension (ext);
        lastFolder = f.getParentDirectory();
        juce::String error;
        if (kind == ExportKind::musicXml)
        {
            if (exportMusicXml (controller.score, range, f, error)) controller.setStatus ("Exported " + f.getFileName());
            else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not export", error);
            return;
        }
        if (! audioFile)
        {
            if (exportMidi (controller.score, range, true, f, error)) controller.setStatus ("Exported " + f.getFileName());
            else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not export", error);
            return;
        }
        // Audio is rendered through a synth of its own, off the audio thread.
        // The window owns itself until it finishes.
        struct Render : public juce::ThreadWithProgressWindow
        {
            Render (const Score& s, ExportRange r, std::unique_ptr<SynthRack> sy, juce::String d, juce::File fl, Controller& c)
                : juce::ThreadWithProgressWindow ("Exporting audio...", true, true),
                  score (s), range (r), synth (std::move (sy)), desc (std::move (d)), file (fl), controller (c) {}
            void run() override
            {
                ok = renderAudio (score, range, *synth, file, [this] (double p) { setProgress (p); return ! threadShouldExit(); }, error);
            }
            void threadComplete (bool cancelled) override
            {
                if (ok) controller.setStatus ("Exported " + file.getFileName() + " through " + desc);
                else if (! cancelled)
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not export audio", error);
                juce::MessageManager::callAsync ([this] { delete this; });
            }
            Score score;
            ExportRange range;
            std::unique_ptr<SynthRack> synth;
            juce::String desc;
            juce::File file;
            Controller& controller;
            bool ok = false;
            juce::String error;
        };
        // Every synth the score needs is made here, on the message thread.
        auto synth = std::make_unique<SynthRack> (audio.usingBuiltInSynth());
        synth->ensureBanks (banksFor (controller.score));
        const auto desc = synth->description();
        audio.stop();
        (new Render (controller.score, range, std::move (synth), desc, f, controller))->launchThread();
    });
}

bool MainComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        for (const char* ext : { ".mid", ".midi", ".noterator", ".musicxml", ".mxl", ".xml" })
            if (f.endsWithIgnoreCase (ext)) return true;
    return false;
}

void MainComponent::filesDropped (const juce::StringArray& files, int, int)
{
    for (const auto& path : files)
    {
        const juce::File f (path);
        if (f.hasFileExtension ("noterator")) { checkSaved ([this, f] { openFile (f); }); return; }
        // A MIDI or MusicXML file dropped on a score joins it, at the caret's bar.
        juce::String error;
        if (! controller.importFile (f, error))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not import " + f.getFileName(), error);
    }
}

//==============================================================================

void MainComponent::audioSettingsDialog()
{
    auto* selector = new juce::AudioDeviceSelectorComponent (audio.devices(), 0, 0, 2, 2, true, false, true, false);
    selector->setSize (520, 420);
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (selector);
    o.dialogTitle = "Audio and MIDI devices";
    o.dialogBackgroundColour = theme::ground;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = true;
    o.resizable = false;
    o.launchAsync();
}

void MainComponent::returnToStart()
{
    controller.returnToStart();
    view.scrollToTickAt (0, 0.0);
}

void MainComponent::skipToEnd()
{
    controller.skipToEnd();
    // The end near the right, so the last bars are in view.
    view.scrollToTickAt (controller.caret, 0.8);
}

void MainComponent::showHelp()
{
    const juce::String text =
        "WRITING\n"
        "N or Return  note input on and off\n"
        "A - G  write that note at the caret (Shift adds it to the chord)\n"
        "0  a rest     1 - 7  note values (5 is a quarter)     .  dot     T  triplet\n"
        "Click a line or space in note input to write the note the key gives it.\n"
        "A MIDI keyboard writes in note input, and always plays.\n\n"
        "EDITING\n"
        "Click a note to select and hear it; double-click for its whole chord\n"
        "Click an empty bar to choose it; drag across bars and parts to choose more\n"
        "(drag along the Chords lane for every part; Shift+click stretches the choice)\n"
        "Shift+drag picks out notes; drag a note up or down to move it\n"
        "Up / Down  a semitone     Cmd+Up / Down  an octave\n"
        "Left / Right  the next note (Shift extends)     Alt+Left / Right  move the selection\n"
        "[ and ]  halve or double     V  swap voice     Delete  remove\n"
        "Cmd+C / X / V  copy, cut, paste at the caret     Cmd+Z  undo\n\n"
        "LISTENING\n"
        "Space  play from bar 1, or stop     Shift+Space  play from the caret\n"
        "Home  back to the start     End  on to the end of the music\n"
        "Follow (toolbar, Play menu)  the page scrolls along with the music as it plays\n"
        "(or turns a page at a time: Play menu)\n"
        "Click a chord in the Chords lane to hear it, or the Scale lane to hear the scale\n\n"
        "GENERATING\n"
        "Choose some bars (or put the caret in a part), choose a generator, press\n"
        "Generate, click a result to hear it, and Insert to put it in.\n"
        "Esc lets go of everything - notes, bars and the part - so an idea goes to every part.\n"
        "Cmd+Z after Generate brings back the ideas it replaced.\n"
        "Cmd-click names to choose several parts, Shift-click for every one between;\n"
        "Cmd+A chooses every part (again: every note). Cmd-click bars to add them.\n"
        "Generate Notes fills the bars chosen: the tune on top, the bass below,\n"
        "chords between. Suggest Notes and Vary Notes work on the music you select.\n\n"
        "BLOCKS\n"
        "Pick a chord, arpeggio, run or interval, click a degree\n"
        "to see and hear it, and Insert to put it at the caret - the caret moves\n"
        "on, so blocks can be laid one after another.";
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Noterator - keys", text);
}

} // namespace nt
