# 2026-10-14 - The apps' text to Apple's writing guidelines (both apps)

## Asked

"Review the GUI text on both Noterator and Miderator to make them in line
with Apple's suggested guidelines" - the HIG's Writing page.

## Done (0053)

- Read the Writing page and, for the Mac's specifics, the Menus, Buttons and
  Alerts pages. The developer site renders with JavaScript; its data is at
  `developer.apple.com/tutorials/data/design/human-interface-guidelines/<page>.json`.
- `ui()` in `Theme.h` for text beyond ASCII; `tools/check_ui_text.py` to
  find any that is not read through it (tested on a planted mistake).
- Every menu, button, dialog, alert, tooltip, status message and the
  Keyboard Shortcuts window rewritten in both apps; the shared Generate tab
  and export messages in Noterator first, copied across. Shortcuts show as
  the Mac writes them (`key()`: ⇧⌘S, ⌫). The part-removal confirmation
  became an alert with a Remove button.
- A test expected "Bars 2-5": now the en dash.

## What looked broken and was not

- On Linux the menus show "Shift+Ctrl+S": `key()` writes symbols only on the
  Mac.

## Not done yet

- ~~Good Idea's "Make a" setting label~~ - done in the engines, below.
- The shared core's file errors ("This is not a MIDI file.") were already
  plain and are left as they are.

# Later - the engines' own words (2026-10-15)

The user asked for every one of their repositories to follow the same rules.
Each generator changed its words in its own repository (Good Idea 0030, Midi
Catalogue 0011, Midi Suggester 0008, Midi Variator 0014), and
`tools/sync_engines.sh` copied them here (commits in `Engines/VENDORED.md`):

- Good Idea's "Make a" is "Kind"; multi-word values read title-style
  ("1 a Bar", "Every 4 Bars", "Four on the Floor") through `name(v)` -
  the stored values did not change, so saved settings still load; hints
  have em dashes and curly quotes.
- Midi Suggester's chord paces are "2 a Bar", "1 a Bar", "1 per 2 Bars".
- Midi Variator's kinds, focuses and forms are title-style ("Add Notes",
  "Towards the End", "A Refrain"); its change sentences use → and en dashes.
- Midi Catalogue's reasons for leaving an entry out read after their count
  ("3 outside the Tuba’s range").
- Starting Blocks' engine did not change.

`TestGenerators` looked for Good Idea's setting by its label and now looks
for "Kind". Everything beyond ASCII reaches JUCE through `std::string`,
which juce::String reads as UTF-8, so `ui()` is not needed for it.
