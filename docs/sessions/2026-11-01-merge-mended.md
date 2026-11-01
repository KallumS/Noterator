# 2026-11-01: what the last merge of main undid, put back

Found while mending Miderator's Mac build, which the same merges broke (its
log of today has the detail). Here nothing failed to compile, so nothing
showed it: the CI run on `main` after PR #10 was cancelled.

## What happened

`claude/blissful-bohr-lqwr1j` held both sessions' work (0049-0057) at
`dffa6c0`. Merging `main` with AutoCC 1.3 (`be02e74`) dropped 0049-0057's
rows from `docs/ARCHITECTURE.md` and `docs/decisions/README.md`, and AutoCC
1.3's decision was numbered 0049, as Escape's is. Merging `main` again after
PR #9 (`92fe935`) kept main's older `MainComponent.cpp` and `Panels.h`: the
File, Export and Bars menus and Add Instrument went back to before 0054's
wording.

## The mend

- `MainComponent.cpp` and `Panels.h` as they were at `be02e74`.
- The 0049-0057 rows back; AutoCC 1.3's decision is 0058, in its file name,
  0013's pointer, `CLAUDE.md` and `AutoCC.h`'s comment (shared: copied to
  Miderator).
- `CLAUDE.md`'s test counts: 96 core, 29 JUCE-side.

## Lesson

After merging `main` into a branch, diff the result against the branch
before the merge: anything there that `main` did not mean to bring is a
revert.
