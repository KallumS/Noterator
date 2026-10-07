#include "GeneratorPanel.h"

#include "Theme.h"

namespace nt
{

GeneratorPanel::GeneratorPanel (Controller& c) : controller (c)
{
    for (auto* comp : std::initializer_list<juce::Component*> { &generator, &description, &followKey, &settingsView, &generateButton,
                                                                 &moreButton, &insertButton, &stopButton, &results, &message, &target })
        addAndMakeVisible (comp);
    settingsView.setViewedComponent (&settingsHolder, false);
    settingsView.setScrollBarsShown (true, false);

    int id = 1;
    for (const auto& g : controller.lua.generators()) generator.addItem (g.name, id++);
    generator.onChange = [this]
    {
        const int i = generator.getSelectedId() - 1;
        if (i < 0) return;
        current = controller.lua.generators()[static_cast<size_t> (i)].id;
        found.clear();
        results.updateContent();
        message.setText ({}, juce::dontSendNotification);
        rebuildSettings();
    };

    description.setFont (juce::FontOptions (13.0f));
    description.setColour (juce::Label::textColourId, theme::textDim);
    description.setJustificationType (juce::Justification::topLeft);
    message.setFont (juce::FontOptions (12.5f));
    message.setColour (juce::Label::textColourId, theme::stepNumber);
    message.setJustificationType (juce::Justification::topLeft);
    target.setFont (juce::FontOptions (13.0f));
    target.setColour (juce::Label::textColourId, theme::text);

    followKey.setToggleState (true, juce::dontSendNotification);
    followKey.setTooltip ("Write in the key the score is in at the caret, rather than the generator's own setting");
    followKey.onClick = [this] { rebuildSettings(); };

    generateButton.setTooltip ("Make new results (G)");
    moreButton.setTooltip ("More results with the same settings");
    insertButton.setTooltip ("Put the chosen result into the caret's part, at the caret's bar (Return)");
    stopButton.setTooltip ("Stop the audition");
    generateButton.onClick = [this] { run (false); };
    moreButton.onClick = [this] { run (true); };
    insertButton.onClick = [this] { insertSelected(); };
    stopButton.onClick = [this] { controller.stop(); };

    results.setRowHeight (44);
    results.setColour (juce::ListBox::backgroundColourId, theme::sunken);
    results.setColour (juce::ListBox::outlineColourId, theme::rule);
    results.setOutlineThickness (1);

    if (! controller.lua.ok())
        message.setText ("The generators did not load:\n" + juce::String (controller.lua.error()), juce::dontSendNotification);
    controller.addChangeListener (this);
    generator.setSelectedId (1);
}

GeneratorPanel::~GeneratorPanel() { controller.removeChangeListener (this); }

bool GeneratorPanel::needsSelection() const
{
    for (const auto& g : controller.lua.generators())
        if (g.id == current) return g.needsSelection;
    return false;
}

void GeneratorPanel::paint (juce::Graphics& g) { g.fillAll (theme::ground); }

void GeneratorPanel::resized()
{
    auto r = getLocalBounds().reduced (10);
    generator.setBounds (r.removeFromTop (28));
    r.removeFromTop (4);
    description.setBounds (r.removeFromTop (52));
    followKey.setBounds (r.removeFromTop (24));
    r.removeFromTop (4);
    const int settingsH = std::min (static_cast<int> (boxes.size()) * 30 + 4, std::max (120, r.getHeight() / 2 - 40));
    settingsView.setBounds (r.removeFromTop (settingsH));
    r.removeFromTop (8);
    target.setBounds (r.removeFromTop (20));
    r.removeFromTop (4);
    auto buttons = r.removeFromTop (30);
    const int w = (buttons.getWidth() - 12) / 4;
    generateButton.setBounds (buttons.removeFromLeft (w)); buttons.removeFromLeft (4);
    moreButton.setBounds (buttons.removeFromLeft (w)); buttons.removeFromLeft (4);
    insertButton.setBounds (buttons.removeFromLeft (w)); buttons.removeFromLeft (4);
    stopButton.setBounds (buttons);
    r.removeFromTop (8);
    message.setBounds (r.removeFromBottom (54));
    r.removeFromBottom (4);
    results.setBounds (r);
    layoutSettings();
}

void GeneratorPanel::rebuildSettings()
{
    const auto ctx = controller.generatorContext (needsSelection());
    if (followKey.getToggleState()) controller.lua.useKey (current, ctx.root, ctx.scale);
    const auto settings = controller.lua.settings (current, ctx);
    for (const auto& g : controller.lua.generators())
        if (g.id == current) description.setText (g.description, juce::dontSendNotification);

    labels.clear();
    boxes.clear();
    for (const auto& s : settings)
    {
        // The key and scale settings are the score's while the toggle is on.
        if (followKey.getToggleState() && (s.label == "Key" || s.label == "Scale")) continue;
        auto label = std::make_unique<juce::Label> (s.id, s.label);
        label->setFont (juce::FontOptions (13.0f));
        label->setColour (juce::Label::textColourId, theme::stepNumber);
        auto box = std::make_unique<juce::ComboBox> (s.id);
        for (size_t i = 0; i < s.names.size(); ++i)
        {
            box->addItem (s.names[i], static_cast<int> (i) + 1);
        }
        box->setSelectedId (s.index + 1, juce::dontSendNotification);
        juce::String tip = s.hint;
        if (s.index >= 0 && s.index < static_cast<int> (s.hints.size()) && ! s.hints[static_cast<size_t> (s.index)].empty())
            tip = (tip.isEmpty() ? juce::String() : tip + "\n") + s.hints[static_cast<size_t> (s.index)];
        box->setTooltip (tip);
        label->setTooltip (tip);
        const std::string settingId = s.id;
        auto* raw = box.get();
        box->onChange = [this, settingId, raw]
        {
            controller.lua.set (current, settingId, raw->getSelectedId() - 1, controller.generatorContext (needsSelection()));
            // Showing or hiding one setting can depend on another.
            juce::MessageManager::callAsync ([this] { rebuildSettings(); });
        };
        settingsHolder.addAndMakeVisible (*label);
        settingsHolder.addAndMakeVisible (*box);
        labels.push_back (std::move (label));
        boxes.push_back (std::move (box));
    }
    resized();
}

void GeneratorPanel::layoutSettings()
{
    const int w = settingsView.getWidth() - settingsView.getScrollBarThickness() - 2;
    int y = 2;
    for (size_t i = 0; i < boxes.size(); ++i)
    {
        labels[i]->setBounds (0, y, w * 2 / 5, 26);
        boxes[i]->setBounds (w * 2 / 5, y, w - w * 2 / 5, 26);
        y += 30;
    }
    settingsHolder.setSize (w, y);
}

void GeneratorPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    juce::String into = "Into: ";
    if (const auto* p = controller.caretPartPtr())
        into += juce::String (p->name) + ", from bar " + juce::String (controller.score.barAt (controller.caret) + 1);
    if (needsSelection())
    {
        const auto [a, b] = controller.selectedBars();
        const juce::String bars = "bar" + juce::String (a == b ? " " : "s ") + juce::String (a + 1) + (a == b ? juce::String() : "-" + juce::String (b + 1));
        into = controller.selection.empty() ? juce::String ("Select some music in the score for this one")
             : juce::String (static_cast<int> (controller.selection.size())) + " notes in " + bars
               + (current == "midi-variator" ? "  |  goes after them" : "  |  goes beside them");
    }
    target.setText (into, juce::dontSendNotification);
    insertButton.setEnabled (results.getSelectedRow() >= 0);
}

void GeneratorPanel::run (bool more)
{
    if (current.empty()) return;
    seed = more ? seed + 6 : static_cast<int> (juce::Random::getSystemRandom().nextInt (90000)) + 1;
    const auto ctx = controller.generatorContext (needsSelection());
    if (followKey.getToggleState()) controller.lua.useKey (current, ctx.root, ctx.scale);
    const auto out = controller.lua.generate (current, ctx, seed, 6);
    if (! out.error.empty())
    {
        message.setText ("The generator failed - this is a bug, please report it:\n" + juce::String (out.error).upToFirstOccurrenceOf ("\n", false, false),
                         juce::dontSendNotification);
        message.setColour (juce::Label::textColourId, theme::warn);
        return;
    }
    message.setColour (juce::Label::textColourId, theme::stepNumber);
    if (more) found.insert (found.end(), out.results.begin(), out.results.end());
    else found = out.results;
    message.setText (out.message.empty() ? juce::String (static_cast<int> (found.size())) + " results. Click one to hear it; Insert puts it in."
                                         : juce::String (out.message), juce::dontSendNotification);
    results.updateContent();
    results.repaint();
    if (! more && ! found.empty()) results.selectRow (0, false, true);
}

void GeneratorPanel::insertSelected()
{
    const int row = results.getSelectedRow();
    if (row < 0 || row >= static_cast<int> (found.size())) return;
    controller.stop();
    controller.insertGenerated (found[static_cast<size_t> (row)], needsSelection(), current);
}

void GeneratorPanel::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= static_cast<int> (found.size())) return;
    const auto& r = found[static_cast<size_t> (row)];
    if (selected)
    {
        g.setColour (theme::frameActive);
        g.fillRect (0, 0, width, height);
        g.setColour (theme::accent);
        g.fillRect (0, 2, 3, height - 4);
    }
    g.setColour (theme::rule);
    g.fillRect (0, height - 1, width, 1);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    g.drawText (r.title, 10, 3, width - 16, 20, juce::Justification::centredLeft, true);
    int notes = 0;
    for (const auto& p : r.parts) notes += static_cast<int> (p.notes.size());
    juce::String sub;
    for (const auto& p : r.parts) sub += (sub.isEmpty() ? "" : ", ") + juce::String (p.name);
    sub += "  |  " + juce::String (static_cast<double> (r.length) / PPQ, 1) + " beats, " + juce::String (notes) + " notes";
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (sub, 10, 22, width - 16, 18, juce::Justification::centredLeft, true);
}

void GeneratorPanel::selectedRowsChanged (int row)
{
    insertButton.setEnabled (row >= 0);
    if (row >= 0 && row < static_cast<int> (found.size())) controller.auditionGenerated (found[static_cast<size_t> (row)], needsSelection(), current);
}

void GeneratorPanel::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    results.selectRow (row);
    insertSelected();
}

juce::String GeneratorPanel::getTooltipForRow (int row)
{
    if (row < 0 || row >= static_cast<int> (found.size())) return {};
    return found[static_cast<size_t> (row)].detail;
}

} // namespace nt
