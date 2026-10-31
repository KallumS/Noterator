#include "MainComponent.h"

#include "Exporter.h"
#include "ScaleModel.h"
#include "SystemSettings.h"
#include "Templates.h"
#include "Theme.h"

namespace nt
{

namespace
{
enum MenuIds
{
    menuNew = 1, menuOpen, menuImport, menuSave, menuSaveAs, menuExportMidi, menuExportMidiBars, menuExportAudio, menuExportAudioBars,
    menuExportXml, menuExportXmlBars,
    menuUndo = 100, menuRedo, menuCut, menuCopy, menuPaste, menuDelete, menuSelectAll, menuVoice, menuClearAll,
    menuPlay = 200, menuPlayCaret, menuToStart, menuToEnd, menuFollow, menuFollowSmooth, menuFollowPage, menuNoteInput, menuLightPage, menuTransposed, menuZoomIn, menuZoomOut, menuAudioSettings, menuHelp,
    menuScaleBase = 340,         // + index into interfaceScales()
    menuTabBase = 360,           // + the side panel's tab
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

// A shortcut as the Mac writes it: Shift+Cmd+S is ⇧⌘S (decision 0054).
juce::String key (const juce::String& k)
{
   #if JUCE_MAC
    if (k == "Delete") return ui ("⌫");
    return k.replace ("Ctrl+", ui ("⌃")).replace ("Alt+", ui ("⌥")).replace ("Shift+", ui ("⇧")).replace ("Cmd+", ui ("⌘"));
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
    view.setTitle ("Score");
    tabs.onTabChanged = [this] { if (isShowing()) grabKeyboardFocus(); };

    toolbar.onFile = [this] { showFileMenu(); };
    toolbar.onStart = [this] { returnToStart(); };
    toolbar.onEnd = [this] { skipToEnd(); };
    toolbar.onClear = [this] { clearAllDialog(); };

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
        controller.varyReplaces = p->getBoolValue ("varyReplaces", false);
        controller.zoom = static_cast<float> (juce::jlimit (5.0, 24.0, p->getDoubleValue ("zoom", controller.zoom)));
        const auto scale = static_cast<float> (p->getDoubleValue ("interfaceScale", 1.0));
        for (auto s : interfaceScales())
            if (std::abs (s - scale) < 0.01f) juce::Desktop::getInstance().setGlobalScaleFactor (s);
        controller.viewChanged();
    }
    readSystemSettings();
    startTimer (1500);
    juce::Desktop::getInstance().addFocusChangeListener (this);

    controller.addChangeListener (this);
    setWantsKeyboardFocus (true);
    // Clicking a control leaves the keyboard with the score (decision 0048).
    theme::clicksKeepFocus (*this);
    const float scale = juce::Desktop::getInstance().getGlobalScaleFactor();
    setSize (juce::roundToInt (1440.0f / scale), juce::roundToInt (880.0f / scale));
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
    juce::Desktop::getInstance().removeFocusChangeListener (this);
    controller.removeChangeListener (this);
    audio.stop();
}

void MainComponent::paint (juce::Graphics& g) { g.fillAll (theme::ground); }

void MainComponent::resized()
{
    // Nothing that matters along the bottom edge, which is often below the
    // screen: the status line is under the toolbar (decision 0048).
    auto r = getLocalBounds();
    toolbar.setBounds (r.removeFromTop (toolbar.heightFor (getWidth())));
    statusBar.setBounds (r.removeFromTop (26));
    tabs.setBounds (r.removeFromRight (juce::jlimit (280, 380, getWidth() / 3)));
    view.setBounds (r);
}

const std::vector<float>& MainComponent::interfaceScales()
{
    static const std::vector<float> s { 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };
    return s;
}

void MainComponent::windowReady()
{
    auto* w = findParentComponentOfClass<juce::ResizableWindow>();
    if (w == nullptr) return;
    // The smallest window is about the same on the screen at every size, so
    // 200% still fits a laptop: the layout reflows rather than the window
    // outgrowing the screen.
    const float scale = juce::Desktop::getInstance().getGlobalScaleFactor();
    const int minW = std::min (980, juce::roundToInt (1100.0f / scale)), minH = std::min (600, juce::roundToInt (700.0f / scale));
    w->setResizeLimits (minW, minH, 10000, 10000);
    if (const auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForRect (w->getScreenBounds()))
    {
        const auto area = d->userBounds.toNearestInt();
        auto b = w->getBounds();
        b.setSize (std::min (b.getWidth(), area.getWidth()), std::min (b.getHeight(), area.getHeight()));
        w->setBounds (b.constrainedWithin (area));
    }
}

void MainComponent::setInterfaceScale (float scale)
{
    auto& desktop = juce::Desktop::getInstance();
    const float old = desktop.getGlobalScaleFactor();
    if (std::abs (old - scale) < 0.01f) return;
    auto* w = findParentComponentOfClass<juce::ResizableWindow>();
    const auto before = w != nullptr ? w->getBounds().toFloat() * old : juce::Rectangle<float>();
    desktop.setGlobalScaleFactor (scale);
    if (auto* p = preferences.getUserSettings()) p->setValue ("interfaceScale", static_cast<double> (scale));
    if (w != nullptr)
    {
        // The same place on the screen, drawn larger or smaller inside.
        w->setResizeLimits (100, 100, 10000, 10000);
        w->setBounds ((before / scale).toNearestInt());
        windowReady();
    }
    controller.setStatus ("Interface size " + juce::String (juce::roundToInt (scale * 100.0f)) + "%");
}

void MainComponent::readSystemSettings()
{
    const bool contrast = system::increaseContrast();
    const bool motion = system::reduceMotion();
    if (contrast == theme::increasedContrast && motion == controller.reduceMotion) return;
    theme::useIncreasedContrast (contrast);
    controller.reduceMotion = motion;
    controller.viewChanged();
    repaint();
}

void MainComponent::timerCallback() { readSystemSettings(); }

void MainComponent::globalFocusChanged (juce::Component* focused)
{
    if (lastFocused != nullptr) lastFocused->repaint();
    if (focused != nullptr) focused->repaint();
    lastFocused = focused;
    // A setting reached with Tab scrolls into view.
    if (focused == nullptr) return;
    if (auto* port = focused->findParentComponentOfClass<juce::Viewport>())
        if (auto* inside = port->getViewedComponent(); inside != nullptr && inside->isParentOf (focused))
        {
            const auto b = inside->getLocalArea (focused, focused->getLocalBounds());
            const auto shown = port->getViewArea();
            if (b.getY() < shown.getY()) port->setViewPosition (shown.getX(), b.getY());
            else if (b.getBottom() > shown.getBottom()) port->setViewPosition (shown.getX(), b.getBottom() - shown.getHeight());
        }
}

void MainComponent::showError (const juce::String& title, const juce::String& message)
{
    // The Mac's own alert (decision 0048).
    juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
                                           .withIconType (juce::MessageBoxIconType::WarningIcon)
                                           .withTitle (title)
                                           .withMessage (message)
                                           .withButton ("OK")
                                           .withAssociatedComponent (this),
                                       nullptr);
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (auto* p = preferences.getUserSettings())
    {
        if (p->getBoolValue ("darkPage", false) == controller.lightPage) p->setValue ("darkPage", ! controller.lightPage);
        if (p->getBoolValue ("followPlayback", true) != controller.followPlayback) p->setValue ("followPlayback", controller.followPlayback);
        if (p->getBoolValue ("varyReplaces", false) != controller.varyReplaces) p->setValue ("varyReplaces", controller.varyReplaces);
        const juce::String style = controller.followStyle == FollowStyle::page ? "page" : "smooth";
        if (p->getValue ("followStyle", "smooth") != style) p->setValue ("followStyle", style);
        if (std::abs (p->getDoubleValue ("zoom", 0.0) - controller.zoom) > 0.01) p->setValue ("zoom", controller.zoom);
    }
    updateTitle();
    menuItemsChanged();
}

void MainComponent::updateTitle()
{
    auto* w = findParentComponentOfClass<juce::DocumentWindow>();
    if (w == nullptr) return;
   #if JUCE_MAC
    // As a Mac document window: the score's name, a dot in the close button
    // while there are unsaved changes, and the file's icon beside the name.
    w->setName (juce::String (controller.score.title));
    if (auto* peer = w->getPeer())
    {
        peer->setDocumentEditedStatus (controller.dirty);
        peer->setRepresentedFile (controller.file);
    }
   #else
    w->setName ((controller.dirty ? "* " : "") + juce::String (controller.score.title) + " - Noterator");
   #endif
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
        if (code >= '1' && code <= '3') { tabs.setCurrentTabIndex (code - '1'); return true; }
        if (code == 'C') { c.copySelection(); return true; }
        if (code == 'X') { c.cutSelection(); return true; }
        if (code == 'V') { c.paste(); return true; }
        if (code == '=' || code == '+') { view.zoomBy (1.15f); return true; }
        if (code == '-') { view.zoomBy (1.0f / 1.15f); return true; }
        if (code == juce::KeyPress::upKey) { c.transposeSelection (12); return true; }
        if (code == juce::KeyPress::downKey) { c.transposeSelection (-12); return true; }
        return false;
    }

    // Tab and Shift+Tab: the score, then every control in reading order, and
    // round to the score again (decision 0048). JUCE's own order would step out
    // to the window itself.
    if (code == juce::KeyPress::tabKey && ! cmd && ! mods.isAltDown())
    {
        auto list = juce::KeyboardFocusTraverser().getAllComponents (this);
        list.erase (std::remove_if (list.begin(), list.end(), [] (juce::Component* x) { return ! x->isShowing() || ! x->isEnabled(); }),
                    list.end());
        list.insert (list.begin(), this);
        const auto at = std::find (list.begin(), list.end(), juce::Component::getCurrentlyFocusedComponent());
        const int n = static_cast<int> (list.size());
        const int from = at == list.end() ? 0 : static_cast<int> (at - list.begin());
        list[static_cast<size_t> ((from + (mods.isShiftDown() ? n - 1 : 1)) % n)]->grabKeyboardFocus();
        return true;
    }

    // A control Tab has reached: Space presses it, as Return does, and Esc
    // hands the keyboard back to the score (decision 0048).
    if (auto* focused = juce::Component::getCurrentlyFocusedComponent(); focused != nullptr && focused != this && isParentOf (focused))
    {
        if (code == juce::KeyPress::escapeKey) { grabKeyboardFocus(); return true; }
        if (code == juce::KeyPress::spaceKey && mods.isShiftDown() == false)
        {
            if (auto* b = dynamic_cast<juce::Button*> (focused)) { b->triggerClick(); return true; }
            if (auto* box = dynamic_cast<juce::ComboBox*> (focused)) { box->showPopup(); return true; }
        }
    }

    // Space from bar 1, Shift+Space from the caret (decision 0035).
    if (code == juce::KeyPress::spaceKey) { c.togglePlay (! mods.isShiftDown()); return true; }
    if (code == juce::KeyPress::homeKey) { returnToStart(); return true; }
    if (code == juce::KeyPress::endKey) { skipToEnd(); return true; }
    // Escape lets go of everything, the caret's part too (decision 0049).
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
        item (menuOpen, "Open" + dots, "Cmd+O");
        item (menuImport, "Import MIDI or MusicXML" + dots);
        m.addSeparator();
        item (menuSave, "Save", "Cmd+S");
        item (menuSaveAs, "Save As" + dots, "Shift+Cmd+S");
        m.addSeparator();
        item (menuExportMidi, "Export Score as MIDI" + dots);
        item (menuExportMidiBars, "Export Selected Bars as MIDI" + dots);
        item (menuExportAudio, "Export Score as Audio (WAV)" + dots);
        item (menuExportAudioBars, "Export Selected Bars as Audio (WAV)" + dots);
        item (menuExportXml, "Export Score as MusicXML" + dots);
        item (menuExportXmlBars, "Export Selected Bars as MusicXML" + dots);
        addScoreItems (m);
    }
    else if (index == 1)
    {
        item (menuUndo, "Undo", "Cmd+Z", controller.canUndo());
        item (menuRedo, "Redo", "Shift+Cmd+Z", controller.canRedo());
        m.addSeparator();
        item (menuCut, "Cut", "Cmd+X", ! controller.selection.empty());
        item (menuCopy, "Copy", "Cmd+C", ! controller.selection.empty());
        item (menuPaste, "Paste", "Cmd+V");
        item (menuDelete, "Delete", "Delete", ! controller.selection.empty());
        item (menuSelectAll, "Select All", "Cmd+A");
        m.addSeparator();
        item (menuVoice, "Swap Voice", "V", ! controller.selection.empty());
        m.addSeparator();
        item (menuClearAll, ui ("Clear All Notes…"), {}, controller.score.lastNoteEnd() > 0);
    }
    else if (index == 2)
    {
        m.addItem (menuNoteInput, "Note Input", true, controller.input.noteInput);
        m.addItem (menuLightPage, "Dark Page", true, ! controller.lightPage);
        m.addItem (menuTransposed, "Transposed Score", true, controller.transposedScore);
        m.addSeparator();
        item (menuZoomIn, "Zoom In", "Cmd+=");
        item (menuZoomOut, "Zoom Out", "Cmd+-");
        m.addSeparator();
        for (int t = 0; t < tabs.getNumTabs(); ++t)
            item (menuTabBase + t, "Show " + tabs.getTabNames()[t], "Cmd+" + juce::String (t + 1));
        m.addSeparator();
        juce::PopupMenu sizes;
        const float now = juce::Desktop::getInstance().getGlobalScaleFactor();
        for (size_t i = 0; i < interfaceScales().size(); ++i)
            sizes.addItem (menuScaleBase + static_cast<int> (i), juce::String (juce::roundToInt (interfaceScales()[i] * 100.0f)) + "%", true,
                           std::abs (interfaceScales()[i] - now) < 0.01f);
        m.addSubMenu ("Interface Size", sizes);
    }
    else if (index == 3)
    {
        if (controller.audio.isPlaying()) item (menuPlay, "Stop", "Space");
        else
        {
            item (menuPlay, "Play from Start", "Space");
            item (menuPlayCaret, "Play from Cursor", "Shift+Space");
        }
        item (menuToStart, "Return to Start", "Home");
        item (menuToEnd, "Skip to End", "End");
        m.addSeparator();
        m.addItem (menuFollow, "Follow Playhead", true, controller.followPlayback);
        // With Reduce Motion on, the page turns rather than scrolls (0048).
        m.addItem (menuFollowSmooth, "    Scroll with the Music", ! controller.reduceMotion,
                   controller.followPlayback && controller.followStyleNow() == FollowStyle::smooth);
        m.addItem (menuFollowPage, "    Turn a Page at a Time", true, controller.followPlayback && controller.followStyleNow() == FollowStyle::page);
        if (controller.reduceMotion) m.addItem (-1, ui ("    (Reduce Motion is on in the Mac’s settings)"), false);
        m.addSeparator();
        item (menuAudioSettings, "Audio and MIDI Devices" + dots);
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
    if (id >= menuTabBase && id < menuTabBase + tabs.getNumTabs())
    {
        tabs.setCurrentTabIndex (id - menuTabBase);
        return;
    }
    if (id >= menuScaleBase && id < menuScaleBase + static_cast<int> (interfaceScales().size()))
    {
        setInterfaceScale (interfaceScales()[static_cast<size_t> (id - menuScaleBase)]);
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
        case menuClearAll: clearAllDialog(); break;
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
    item (menuOpen, "Open" + dots, "Cmd+O");
    item (menuImport, "Import MIDI or MusicXML" + dots);
    m.addSeparator();
    item (menuSave, "Save", "Cmd+S");
    item (menuSaveAs, "Save As" + dots, "Shift+Cmd+S");
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
        m.addItem (menuClearAll, ui ("Clear All Notes…"), controller.score.lastNoteEnd() > 0);
    }
    addScoreItems (m);
    m.addSeparator();
    {
        juce::PopupMenu::Item i ("Note Input");
        i.itemID = menuNoteInput;
        i.isTicked = controller.input.noteInput;
        i.shortcutKeyDescription = "N";
        m.addItem (i);
    }
    m.addItem (menuLightPage, "Dark Page", true, ! controller.lightPage);
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
    m.addSectionHeader ("This Score");
    m.addItem (menuTitle, "Title and Composer" + dots);
    m.addItem (menuTempo, "Tempo (" + juce::String (juce::roundToInt (s.tempos.front().bpm)) + " BPM)" + dots);

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
    meters.addItem (menuMeterOther, "Other" + dots, true, ! listed);
    m.addSubMenu ("Time Signature at Bar " + juce::String (bar + 1) + " (" + juce::String (meter.num) + "/" + juce::String (meter.den) + ")", meters);

    const auto& k = s.keyAtBar (bar);
    juce::PopupMenu keys;
    keys.addItem (menuKeyHeard, "Use the Key It Hears");
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
    m.addSubMenu ("Key at Bar " + juce::String (bar + 1) + " (" + keyName + ")", keys);

    juce::PopupMenu bars;
    const auto [a, b] = controller.selectedBars();
    bars.addItem (menuInsertBar, "Insert a Bar at the Caret (Before Bar " + juce::String (bar + 1) + ")");
    bars.addItem (menuDeleteBars, a == b ? "Delete Bar " + juce::String (a + 1) : "Delete Bars " + juce::String (a + 1) + "-" + juce::String (b + 1));
    bars.addItem (menuAddBars, "Add 4 Bars at the End");
    m.addSubMenu ("Bars (" + juce::String (s.bars) + " in All)", bars);

    m.addSeparator();
    juce::PopupMenu sound;
    sound.addItem (menuSoundApple, "Apple General MIDI (macOS)", true, ! audio.usingBuiltInSynth());
    sound.addItem (menuSoundBuiltIn, "Built-in Synth", true, audio.usingBuiltInSynth());
    sound.addSeparator();
    sound.addItem (menuAudioSettings, "Audio and MIDI Devices" + dots);
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
    // Cancel on the left and OK, the default, on the right, as on the Mac.
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([w, done] (int r) { if (r == 1) done (*w); }), true);
}
} // namespace

void MainComponent::titleDialog()
{
    askFor ("Title and Composer", {}, [this] (juce::AlertWindow& w)
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
    askFor ("Tempo", ui ("Beats per minute, from 30 to 240."), [this] (juce::AlertWindow& w)
    {
        w.addTextEditor ("bpm", juce::String (juce::roundToInt (controller.score.tempos.front().bpm)), "Tempo");
        if (auto* e = w.getTextEditor ("bpm")) e->setInputRestrictions (3, "0123456789");
    },
    [this] (juce::AlertWindow& w)
    {
        const int bpm = w.getTextEditorContents ("bpm").getIntValue();
        if (bpm < 30 || bpm > 240) { controller.setStatus ("The tempo is unchanged. Choose a tempo from 30 to 240 beats a minute."); return; }
        controller.setTempo (bpm);
    });
}

void MainComponent::meterDialog()
{
    const int bar = controller.score.barAt (controller.caret);
    askFor ("Time Signature at Bar " + juce::String (bar + 1), {}, [this, bar] (juce::AlertWindow& w)
    {
        const auto& m = controller.score.meterAtBar (bar);
        juce::StringArray nums, dens { "2", "4", "8", "16" };
        for (int n = 1; n <= 16; ++n) nums.add (juce::String (n));
        w.addComboBox ("num", nums, "Beats per bar");
        w.addComboBox ("den", dens, "Beat value");
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
    m.addItem (menuExportMidi, "Score as MIDI" + dots);
    m.addItem (menuExportMidiBars, "Selection (" + bars + ") as MIDI" + dots);
    m.addSeparator();
    m.addItem (menuExportAudio, "Score as Audio (WAV)" + dots);
    m.addItem (menuExportAudioBars, "Selection (" + bars + ") as Audio (WAV)" + dots);
    m.addSeparator();
    m.addItem (menuExportXml, "Score as MusicXML (for Dorico, MuseScore, Sibelius)" + dots);
    m.addItem (menuExportXmlBars, "Selection (" + bars + ") as MusicXML" + dots);
    return m;
}

//==============================================================================
// Files

void MainComponent::checkSaved (std::function<void()> then)
{
    if (! controller.dirty) { then(); return; }
    // The Mac's own question, in its own words (decision 0048).
    auto opts = juce::MessageBoxOptions()
                    .withIconType (juce::MessageBoxIconType::QuestionIcon)
                    .withTitle ("Do you want to save the changes you made to " + juce::String (controller.score.title) + "?")
                    .withMessage ("Your changes will be lost if you don't save them.")
                    .withButton ("Save")
                    .withButton (ui ("Don’t Save"))
                    .withButton ("Cancel")
                    .withAssociatedComponent (this);
    juce::NativeMessageBox::showAsync (opts, [this, then] (int r)
    {
        if (r == 0) saveDialog (false, then);
        else if (r == 1) then();
    });
}

void MainComponent::openFile (const juce::File& f)
{
    juce::String error;
    if (! controller.load (f, error))
        showError (ui ("“") + f.getFileName() + ui ("” couldn’t be opened."), error);
    lastFolder = f.getParentDirectory();
}

void MainComponent::openDialog()
{
    checkSaved ([this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Open a Score, MIDI File or MusicXML File", lastFolder,
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
    chooser = std::make_unique<juce::FileChooser> ("Import a MIDI or MusicXML File at the Cursor", lastFolder,
                                                  "*.mid;*.midi;*.musicxml;*.mxl;*.xml");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (! f.existsAsFile()) return;
        juce::String error;
        if (! controller.importFile (f, error))
            showError (ui ("“") + f.getFileName() + ui ("” couldn’t be imported."), error);
        lastFolder = f.getParentDirectory();
    });
}

void MainComponent::saveDialog (bool saveAs, std::function<void()> then)
{
    if (! saveAs && controller.file.existsAsFile())
    {
        juce::String error;
        if (controller.save (controller.file, error)) { if (then) then(); }
        else showError (ui ("The score couldn’t be saved."), error + "\n\nTry Save As to choose another folder, or check that the disk has room.");
        return;
    }
    const auto suggested = (lastFolder.isDirectory() ? lastFolder : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory))
                               .getChildFile (juce::File::createLegalFileName (controller.score.title) + ".noterator");
    chooser = std::make_unique<juce::FileChooser> ("Save Score", suggested, "*.noterator");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, then] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        f = f.withFileExtension ("noterator");
        juce::String error;
        if (controller.save (f, error)) { lastFolder = f.getParentDirectory(); if (then) then(); }
        else showError (ui ("The score couldn’t be saved."), error + "\n\nTry Save As to choose another folder, or check that the disk has room.");
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
        suffix = a == b ? " bar " + juce::String (a + 1) : " bars " + juce::String (a + 1) + "-" + juce::String (b + 1);   // a file name: plain hyphen
    }
    const bool audioFile = kind == ExportKind::audio;
    const juce::String ext = audioFile ? "wav" : kind == ExportKind::musicXml ? "musicxml" : "mid";
    const auto suggested = (lastFolder.isDirectory() ? lastFolder : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory))
                               .getChildFile (juce::File::createLegalFileName (juce::String (controller.score.title) + suffix) + "." + ext);
    chooser = std::make_unique<juce::FileChooser> (audioFile ? "Export Audio" : kind == ExportKind::musicXml ? "Export MusicXML" : "Export MIDI",
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
            else showError (ui ("The file couldn’t be exported."), error + "\n\nTry another folder, or check that the disk has room.");
            return;
        }
        if (! audioFile)
        {
            if (exportMidi (controller.score, range, true, f, error)) controller.setStatus ("Exported " + f.getFileName());
            else showError (ui ("The file couldn’t be exported."), error + "\n\nTry another folder, or check that the disk has room.");
            return;
        }
        // Audio is rendered through a synth of its own, off the audio thread.
        // The window owns itself until it finishes.
        struct Render : public juce::ThreadWithProgressWindow
        {
            Render (const Score& s, ExportRange r, std::unique_ptr<SynthRack> sy, juce::String d, juce::File fl, Controller& c)
                : juce::ThreadWithProgressWindow ("Exporting audio" + dots, true, true),
                  score (s), range (r), synth (std::move (sy)), desc (std::move (d)), file (fl), controller (c) {}
            void run() override
            {
                ok = renderAudio (score, range, *synth, file, [this] (double p) { setProgress (p); return ! threadShouldExit(); }, error);
            }
            void threadComplete (bool cancelled) override
            {
                if (ok) controller.setStatus ("Exported " + file.getFileName() + " using " + desc);
                else if (! cancelled)
                    juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
                                                           .withIconType (juce::MessageBoxIconType::WarningIcon)
                                                           .withTitle (ui ("The audio couldn’t be exported."))
                                                           .withMessage (error)
                                                           .withButton ("OK"),
                                                       nullptr);
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
            showError (ui ("“") + f.getFileName() + ui ("” couldn’t be imported."), error);
    }
}

//==============================================================================

void MainComponent::audioSettingsDialog()
{
    auto* selector = new juce::AudioDeviceSelectorComponent (audio.devices(), 0, 0, 2, 2, true, false, true, false);
    selector->setSize (520, 420);
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (selector);
    o.dialogTitle = "Audio and MIDI Devices";
    o.dialogBackgroundColour = theme::ground;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = true;
    o.resizable = false;
    o.launchAsync();
}

void MainComponent::clearAllDialog()
{
    if (controller.score.lastNoteEnd() <= 0) { controller.setStatus ("There are no notes to clear."); return; }
    // It asks, as removing a part with notes in it does; Undo is there too.
    const auto options = juce::MessageBoxOptions()
                             .withIconType (juce::MessageBoxIconType::QuestionIcon)
                             .withTitle ("Clear every note?")
                             .withMessage ("Every part keeps its instrument, and the tempo, key and bars stay. Undo brings the notes back.")
                             .withButton ("Clear")
                             .withButton ("Cancel")
                             .withAssociatedComponent (this);
    juce::NativeMessageBox::showAsync (options, [safe = juce::Component::SafePointer<MainComponent> (this)] (int r)
    {
        if (safe != nullptr && r == 0) safe->controller.clearAllNotes();
    });
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
    // The keys in the Mac's own symbols, and plain sentences (decision 0054).
    const juce::String text = ui (
        "WRITING\n"
        "N or Return    Turn note input on or off\n"
        "A–G    Write that note at the cursor (⇧ adds it to the chord)\n"
        "0    Write a rest\n"
        "1–7    Choose a note value (5 is a quarter note)\n"
        ".  and  T    Dot and triplet\n"
        "In note input, click a line or space to write a note there. A MIDI keyboard writes in note input and always plays.\n"
        "\n"
        "EDITING\n"
        "Click a note to select and hear it. Double-click to select its whole chord.\n"
        "Click an empty bar to choose it. Drag across bars and parts to choose more, or along the Chords lane to choose every part. ⇧-click extends the choice.\n"
        "⇧-drag selects notes. Drag a note up or down to move it.\n"
        "↑ ↓    Move up or down a semitone (⌘ for an octave)\n"
        "← →    Select the next note (⇧ extends the selection)\n"
        "⌥← ⌥→    Move the selection\n"
        "[  ]    Halve or double the note values\n"
        "V    Swap voice\n"
        "⌫    Delete\n"
        "⌘C  ⌘X  ⌘V    Copy, cut and paste at the cursor\n"
        "⌘Z  ⇧⌘Z    Undo and redo\n"
        "Clear All (toolbar)    Clear every note from every part, after asking\n"
        "\n"
        "LISTENING\n"
        "Space    Play from bar 1, or stop\n"
        "⇧Space    Play from the cursor\n"
        "Home  End    Go to the start or the end of the music\n"
        "Follow (toolbar or Play menu) scrolls the page with the music as it plays.\n"
        "Click a chord in the Chords lane to hear it, or the Scale lane to hear the scale.\n"
        "\n"
        "GENERATING\n"
        "Choose some bars or parts, choose a generator, click Generate, click an idea to hear it, then click Insert.\n"
        "Generate Notes fills the chosen bars: the tune on top, the bass below and the chords between. Suggest Notes and Vary Notes work on the music you select.\n"
        "Esc    Let go of everything, so the next idea goes to every part\n"
        "⌘Z    After Generate, bring back the ideas it replaced\n"
        "⌘-click    Add a part (by its name) or bars to the choice\n"
        "⇧-click    Choose every part between two names\n"
        "⌘A    Choose every part; press again to select every note too\n"
        "\n"
        "BLOCKS\n"
        "Choose a chord, arpeggio, run or interval, click a degree to see and hear it, then click Insert to add it at the cursor. The cursor moves on, so you can add blocks one after another.\n"
        "\n"
        "THE WINDOW\n"
        "Tab  ⇧Tab    Move between the buttons and menus; Space or Return presses one\n"
        "Esc    Back to the score from a button or menu\n"
        "⌘1  ⌘2  ⌘3    The Generate, Blocks and Parts tabs\n"
        "View > Interface Size makes everything larger, up to 200%.");
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Keyboard Shortcuts", text);
}

} // namespace nt
