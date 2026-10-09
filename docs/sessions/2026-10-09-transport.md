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
