# Decisions

One file per decision that would otherwise have to be re-argued from scratch:
what the situation was, what was chosen, and what it costs. They are dated and
numbered and **they are not edited to stay true** - a reversed decision keeps
its text and gains a line at the top pointing at the one that replaced it.

`CLAUDE.md` is the working guide; these are the reasons underneath it. The code
refers to them by number: "(decision 0006)".

| | | |
| --- | --- | --- |
| [0001](0001-a-juce-app-not-a-reascript.md) | A desktop app in JUCE, not a ReaScript | Accepted |
| [0002](0002-the-score-is-midi-shaped.md) | The score is MIDI-shaped; notation is worked out from it | Accepted |
| [0003](0003-run-the-lua-engines-unchanged.md) | Run the family's Lua engines unchanged, through embedded Lua | Accepted |
| [0004](0004-one-set-of-columns-in-a-galley.md) | One set of columns for every staff, in one long system | Accepted |
| [0005](0005-edits-are-functions-undo-is-a-copy.md) | Every edit is a function in Edit.h; undo keeps the score from before | Accepted |
| [0006](0006-instruments-carry-their-context.md) | Instruments carry their context, and everything reads it | Accepted |
| [0007](0007-one-performance-and-the-macs-own-orchestra.md) | One performance for playback and export, through the Mac's own General MIDI set | Accepted |
| [0008](0008-general-midi-gets-two-cc-lanes.md) | General MIDI playback gets AutoCC's CC7 and CC11 only; files get all four | Accepted |
| [0009](0009-bravura-not-drawn-glyphs.md) | Bravura for the symbols, where Starting Blocks Notation drew its own | Accepted |
| [0010](0010-verify-by-rendering.md) | Verify the engraving by rendering it | Accepted |
