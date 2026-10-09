# 2026-10-09 (evening) - Start, end, and Space from bar 1

Done alongside Miderator, the same change in both apps.

## Asked

A "return to start" and a "skip to end" button beside Play, Space to play
from bar 1 and Shift+Space from the current location - in both apps.

## Done (0035)

- `Controller::togglePlay (fromStart)`, `returnToStart`, `skipToEnd`,
  `musicEnd` (the bar line after the last note). Space passes `fromStart`,
  Shift+Space and the Play button do not.
- `TransportButton` in `Panels`: |◀ and ▶|, drawn, either side of Play.
- Home and End do the same; the Play menu lists all four.
- `ScoreView::scrollToTickAt` puts the end of the music near the right.
- An app test (9 now): Space from 0, Shift+Space from the caret, start
  while playing plays on from 0, end stops and puts the caret at the bar
  line after the last note, and an empty score's end is its last bar line.

## The route

- Both repositories' branches had been merged into `main` and deleted; the
  branch was started again from `main`, as the work is new.
- Tried in the window with the silent PulseAudio sink: End showed bars 22-24
  with the final bar line, Home went back, a click in bar 3 then
  Shift+Space played from there, Space played from bar 1.

## What looked wrong and was not

- **Shift+Space from a chosen bar started mid-bar**: a click on an empty bar
  chooses the bar and puts the caret where the click was. "The current
  location" is the caret (0035).

## Then: one note at a time, and a File button (0036, 0037)

The user found chords generated for one-note instruments, and the toolbar
cluttered.

- **Measured first.** A probe put Good Idea's results (7 kinds, 12 seeds,
  2 each) into a string quartet, with the caret in the first violin and with
  bars chosen across all four. Chord phrases gave the violin chords 24 times
  in 24 (the first line always went to the caret's part, unchecked), and a
  string part two notes at once in 30 of 48 across bars (the chords were
  dealt out, but held notes ran into the next chord). Everything else was
  already clean.
- **`fitToPolyphony`** in `Generators.*`, used by `insertResult` and
  `insertIntoRange` for every line: the top notes kept (the bottom for a
  bass), held notes cut to the next, drums untouched. Blocks pass
  `fitPolyphony = false` in `Controller::place`. The status line names the
  parts that were thinned. Four core tests failed on the old code and pass
  now (78), one of them over real Good Idea chord phrases; an app test
  covers the controller (10). The probe afterwards: 0 everywhere.
- **File**: New (the templates), Open, Import, Save, Save As, Export (the
  same sub-menu as before) under one button; tried in the window - New >
  Piano made a piano score, Export opened its list.

## Then: the Score tab into File (0038)

The user asked for the Score tab's contents to go under File too. They are
now a "This score" section in the File menu (and the Mac's File menu): title
and composer and tempo in small boxes, time signature and key as sub-menus
ticked at the caret's bar, bars, and sound; the tab and `ScorePanel` are
gone, and "use the key it hears" is `Controller::useHeardKey`, with an app
test (11 now). The same script made the change in Miderator, where it found
a bug of Miderator's own (its grid range swallowed the new menu ids; see its
log). Tried in the window: 3/4 at bar 1 reset the staves' signatures, tempo
88 showed back in the menu.
