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

## Then: more into File, and orchestration (0039, 0040)

Undo, Redo, Sound, the input mode and the look left the toolbar for the File
menu, in both apps, by one script; the status line still shows the input
mode. Tried in both windows (Light and Note input from the menu, ticked).

The user asked for generated music to fill every chosen part by range, with
a string quintet as the example, and sent orchestration books. A helper read
them (Rimsky-Korsakov's chapter III above all) into a rulebook; the result is
`Orchestrate.*`, shared, made in Noterator and synced (0040). Measured
first: Generate Notes' chords are block chords at only ~40% of onsets - the
rest is a bass alone, broken chords, arpeggios - so the harmony is read with
`detectChords`, and inner parts strike where the chords strike or hold.
Mistakes on the way: the first voicing bounded inner parts by the tune's
octave doubling, squeezing the viola into the bass's register - inner parts
now go under the tune, starting where the generated chord has their note;
and a Good Idea tune overlapping by a tick read as chords, so the tune and
the chords swapped and the piccolo and glockenspiel were thinned - seen in
Miderator's window, fixed by naming first (the test failed before). The
full orchestra in the window: all 28 parts filled, nothing thinned. 85 core tests, 11 app tests.

## Then: where generated music goes, simplified (0041)

Trying 0040 the user found the placement confusing and set four cases:
nothing chosen shares an idea across every part; bars of one part take all
of it (first read as "one bar = where it starts", corrected by the user to
"one bar = one bar of it"); bars of several parts share it. `insertWhole`
puts all of a result into one part - thinned, then put in its register - and
no part is added any more: a line with nowhere to go is named. An older app
test that put chords into the caret's violin with nothing chosen now chooses
the violin's bars. Mistake: an interrupted tool call had in fact written its
tests, so they went in twice - caught by counting `TEST` lines. Miderator's
branch had been merged and deleted on GitHub; its new commit was moved onto
`main`. In the window, nothing chosen in a full orchestra: all 28 parts
filled from bar 1. 87 core tests, 12 app tests.

## Then: one line to one part; bars filled once (0042)

The user asked that a single line (a melody, a motif) never be shared out,
and that chosen bars be filled once - cut where they end, the rest left
empty, never repeated. `isSingleLine` and `Controller::lineTarget` (the
caret's part, among the chosen ones if bars are chosen); `fitToSpan` no
longer repeats - its test was changed first and failed, and an older test
that expected a two-bar idea twice in four bars now expects it once. A tune
with a second voice splits, the upper parts the tune. The Generate tab's
"Into" line was too long for "Baritone Saxophone" and was shortened. In the
window: a motif with nothing chosen went into Violin I alone. 88 core tests, 13 app tests.

## Then: auditions play every note, on a piano (0043)

After merging 0042 the user found that auditioning Generate Notes and the
Blocks preview played one note at a time, and asked for every note, always
on a piano. The cause was 0036: the audition played the result as placed,
fitted to each part (chords into a violin, one note), and the Blocks preview
was fitted to the caret's violin though the block itself goes in unthinned.
`addAudition` (shared) adds the result as it was made - every note on a
piano, drums on a kit, soloed if anything is - and `Controller::auditionScore`
plays it where it would go, the parts it replaces silent there;
`auditionAlone` plays a block on its own. The app test failed with the old
audition put back. In Miderator's window, the Blocks preview with the caret
on Violin I shows the whole C major chord again. The branch was restarted
from `main` after the user merged 0042. 89 core tests, 14 app tests.
