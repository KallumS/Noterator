# 2026-10-21: Vary Notes in place, Voicing on Any, Clear All

Asked, three things: Vary Notes to offer replacing the selected music or
adding the variation after it; Generate Notes' Voicing to start on Any; a
button to clear the timeline. Done in both apps on `claude/blissful-bohr-lqwr1j`
(restarted from `main`, where 0048 had been merged), as 0055, 0056 and 0057.

## The route

1. **Voicing (0056).** "Voicing" is not in the adapter: it is one of Good
   Idea's own settings (`gi_idea.lua`, default Close), which is never edited
   here. The adapter's `newState` sets it to Any after the engine's defaults.
   The test was written first and read "got Close, wanted Any"; it looks for
   Voicing on a Measure, since a Motif has no chords and hides it.
2. **Vary Notes (0055).** Placement is `Controller::place`'s, so the choice is
   a controller flag, `varyReplaces`, shown by the Generate tab as a "Put it"
   menu in the row where "Use the score's key" sits for the others (Vary Notes
   reads the key from the music, so that toggle never did anything for it).
   Four cases, tested before the menu was built: notes or bars chosen, after
   or in place. In place for notes deletes them and inserts without clearing,
   so other notes in those bars stay; in place for bars is the old behaviour.
3. **Clear All (0057).** `Controller::clearAllNotes`, one edit, the parts
   kept; Blocks' chord roots go with the notes. A toolbar button at the right,
   an Edit and a File menu item, the Mac's alert first.
4. Driven headless: Vary Notes chosen, Put it set to In place, a variation
   inserted over a Good Idea motif (the original gone, the variation in bar
   1); Clear All, the alert, every note gone, the status line naming Undo.

## What looked broken and was not, and what was

- **The audition after chosen bars would have been empty**: its length was
  measured from where the variation went to the end of the bars, which after
  the bars is nothing or less. Measured from the bars' own span now; a test
  checks it.
- **The Into line kept the last generator's words** after switching to
  another, until something else changed (seen in a screenshot: Vary Notes
  showing Generate Notes' "chords to every part"). It is recomputed when the
  generator changes.
- **A force-push was refused** (the branch on GitHub held only merged
  history). Not needed: the old tip was an ancestor of `main`, so a plain push
  was a fast-forward.

## Not done yet

- None of 0055-0057 tried by the user on a Mac.

## Then: the two lines of work brought together

The user could not merge PR #9 (`claude/adoring-feynman-bihxp2`, work from
2026-10-10 to 10-15: Escape, Undo for Generate, several parts, no More
button, Vary Notes across parts, Apple's writing guidelines). It had started
from `main` before 0048 was merged, and numbered its decisions from 0048 too.

- Its decisions renumbered 0049-0054 on a copy of it (one perl pass, so
  0048 to 0049 could not cascade), today's 0055-0057; then that copy merged
  into this branch, so PR #9's commits are all here and its branch untouched.
- Conflicts resolved by meaning: their wording everywhere (0054) with 0048's
  structure kept - the Mac's own alerts with their titles, Reduce Motion in
  their Play menu, Clear All in their Edit menu, the status line's layout
  with their "Every part" and "N parts". Escape first hands the keyboard
  back from a control (0048), then lets go of everything (0049).
- Vary Notes: their across-parts rule (0053) and Put it (0055) combined - in
  place across several parts takes those bars of those parts; a test for it.
- The Generate tooltip's "(G)" dropped again: G writes a note in Noterator.
