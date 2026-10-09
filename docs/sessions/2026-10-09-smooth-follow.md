# 2026-10-09 (later) - Follow scrolls smoothly

## Asked

After Follow (0033): could the page move with the playhead, scrolling along
smoothly, rather than flipping to the next page?

## Done (0034)

- `Follow.h`: `FollowStyle::smooth` (the default) - the playhead walks in to
  a third of the way across, then the music moves under it - and
  `FollowStyle::page`, 0033's. The Play menu chooses; the choice is
  remembered.
- `SmoothClock` in `Follow.h`, used by `Controller::playheadTick`.
- `Layout::playheadX` in the engraver: continuous across bar lines.
- ScoreView redraws at 60 Hz, scrolls in whole pixels.
- Shared to Miderator with `tools/sync_from_noterator.sh`.

## The route

- **Playback in the container, for the first time.** No sound card, so the
  playhead had never moved here. A PulseAudio null sink behind ALSA's
  default device runs the audio callback in real time; the app plays
  silently and the page can be watched, screenshotted and recorded
  (CLAUDE.md has the commands).
- **The first smooth scroll juddered.** Recorded at 60 fps and measured by
  matching each frame to the last, the page moved 8 pixels every fourth
  frame - but that was the Debug build drawing too slowly. In RelWithDebInfo
  it moved most frames, unevenly: 0 to 6 pixels.
- **The uneven part was the clock.** A trace of the timer showed the
  playhead advancing by exactly 22 or 45 ticks per frame - whole audio
  blocks. The first `SmoothClock` only glided between blocks when frames
  came faster than blocks, and here they come slower (10-27 ms against
  11.6 ms). The second runs on its own clock and is pulled 15% of the way
  to the sound each frame. A test with blocks of 512 samples and uneven
  frames fails the first by a whole block (10 ms) and passes the second
  within 3 ms; in the window the music's speed now holds within about 3%.
- **A hitch at every bar line was found by reading `xForTick`**, before it
  showed: the page's x jumps from the bar's end to the next bar's first
  column. `playheadX` interpolates across; its test fails on `xForTick`.

## What looked wrong and was not

- **Recorded at 60 fps, a third of the frames had not moved.** Xvfb draws in
  software and the app repaints about 50 times a second there; the trace
  shows the timer and the clock even. A Mac draws on its GPU.

## Not done yet

- Not yet seen on a Mac. Large scores redraw the whole page every frame;
  if that stutters, Turn a Page at a Time is the lighter choice, and drawing
  only what moved would be the fix.
