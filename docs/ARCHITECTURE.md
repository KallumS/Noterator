# Noterator's architecture, and the decisions behind it

One page for the whole shape of the app and every big decision in it. Each
decision has its own record in [`decisions/`](decisions/README.md), with the
reasoning and what it costs; this page is the map.

## The shape

```
                 Engines/<app>/*.lua  (the family's engines, unchanged)
                 Engines/adapters/*.lua  (one adapter each)
                          |
                          v
  Source/Engines/   LuaEngine  ---  Generators (context in, results placed)
                          |
                          v
  Source/Core/      Score  <-- Edit (every change)        Instruments (one table), Templates
   (no JUCE)          |                                     |
                      +--> Engrave --> Layout (staff spaces) |
                      +--> Detect  --> chord and key lanes  <-+
                      +--> Perform --> MIDI events <-- AutoCC
                      +--> MidiFile, ScoreFile, MusicXml (own Xml reader/writer)
                          |
                          v
  Source/App/       Controller (owns the score, undo, selection, caret)
   (JUCE)             |-- ScoreView + ScoreRenderer (Bravura)  -- the page
                      |-- Toolbar; Generate, Blocks, Parts panels; status line
                      |-- AudioEngine (a rack of Apple GM Audio Units / built-in synths)
                      `-- Exporter (MIDI, WAV, MusicXML)
```

- **The score is MIDI.** Parts of notes in ticks (960 a quarter), sounding
  pitch. Nothing about the notation is stored.
- **The page is derived.** `Engrave` turns the score into a layout in staff
  spaces; `ScoreRenderer` inks it. The same renderer draws PNGs with no window.
- **The music core has no JUCE** (`Source/Core`, `Source/Engines`), so 78 tests
  build and run in seconds. `NoteratorAppTests` covers the JUCE side.
- **One controller.** Every window piece reads the `Controller` and asks it for
  changes; it keeps undo, re-engraves, re-detects and re-sends to playback.
- **The page follows the music** (`Follow.h`): smoothly by default, a page
  at a time on request, the playhead on a steady clock of its own.
- **Shared with Miderator.** Miderator (KallumS/Miderator) is this app with a
  piano roll for a page; `Source/Core` (but its `Roll.*`), `Source/Engines`,
  the engines and the core tests are the same files in both. Changes are made
  here and copied there. Decisions 0026-0032 are Miderator's; the two apps
  number decisions in one sequence.

## The decisions

### What the app is
| | |
| --- | --- |
| [0001](decisions/0001-a-juce-app-not-a-reascript.md) | A standalone JUCE 8 app, not a ReaScript: it owns its window, files and audio, and can host instruments. |
| [0012](decisions/0012-apple-silicon-only-built-by-ci.md) | Apple silicon only; GitHub Actions builds, tests and packages the Mac app, because nothing here can. |

### The music
| | |
| --- | --- |
| [0002](decisions/0002-the-score-is-midi-shaped.md) | The score is MIDI; notation is worked out from it every time. Generators' output goes in unconverted. |
| [0005](decisions/0005-edits-are-functions-undo-is-a-copy.md) | Every edit is a pure function; undo keeps a copy of the score. Note input overwrites. |
| [0006](decisions/0006-instruments-carry-their-context.md) | One instrument table (Midi Catalogue's numbers plus clefs, transposition, sound, AutoCC shape) read by everything. |
| [0022](decisions/0022-templates-in-score-order.md) | Templates (sections, full orchestra, big band) are data in the core, in score order, numbered where repeated. |

### The generators
| | |
| --- | --- |
| [0003](decisions/0003-run-the-lua-engines-unchanged.md) | The five Lua engines run unchanged through embedded Lua 5.4, each behind a small adapter. |
| [0036](decisions/0036-one-note-at-a-time-for-one-note-instruments.md) | Every generated line is fitted to the part it lands in: no more notes at once than the instrument plays - the top line kept, or the bottom for a bass. Blocks go in as they are. |
| [0011](decisions/0011-selection-results-go-beside-or-after.md) | Results made from a selection go beside it (Suggester) or after it (Variator), never over it. |
| [0017](decisions/0017-catalogue-leaves-generate.md) | Generate lists Good Idea, Suggester and Variator; the Catalogue stays loaded but unlisted. |
| [0018](decisions/0018-starting-blocks-is-a-toolbox.md) | Starting Blocks is a Blocks tab: kind, degree buttons, preview; a block goes at the caret and the caret moves on. |
| [0024](decisions/0024-blocks-without-drums-and-bass.md) | Blocks offers chords, arpeggios, runs and intervals; drums and bass are Good Idea's. |
| [0025](decisions/0025-generators-named-for-what-they-do.md) | In the app: Generate Notes (Good Idea), Suggest Notes (Suggester), Vary Notes (Variator); code keeps the engines' names. |
| [0019](decisions/0019-bars-can-be-chosen-and-filled.md) | Bars chosen by dragging; Good Idea fills them exactly, tune on top, bass below, chords between; Variator replaces them. |

### The page
| | |
| --- | --- |
| [0004](decisions/0004-one-set-of-columns-in-a-galley.md) | Every staff shares one set of columns per bar, in one long scrolling system (a galley). |
| [0009](decisions/0009-bravura-not-drawn-glyphs.md) | Bravura draws every symbol, calibrated so a notehead is one staff space. |
| [0010](decisions/0010-verify-by-rendering.md) | Engraving changes are checked by rendering them to PNG. |
| [0014](decisions/0014-chords-and-keys-read-from-the-score.md) | Chord lane: ScaleView's names on beat-by-beat segments, only real harmony named. Key lane: Suggester's finder, the signature breaking ties. |
| [0015](decisions/0015-the-house-scheme-and-a-dark-page.md) | The family's colour scheme (its dark-page default replaced by 0020). |
| [0020](decisions/0020-light-page-by-default.md) | Black on white by default; Dark page on request, remembered. |

### Input and sound
| | |
| --- | --- |
| [0033](decisions/0033-follow-the-playhead-switchable.md) | While it plays, the page follows the playhead; Follow switches it, remembered. |
| [0037](decisions/0037-a-file-menu-button.md) | New, Open, Save and Export under one File button. |
| [0038](decisions/0038-the-score-tab-moves-into-file.md) | The Score tab's settings move into the File menu; the tab is gone. |
| [0039](decisions/0039-undo-sound-input-and-look-into-file.md) | Undo, Redo, Sound, the input mode and the look move into the File menu. |
| [0040](decisions/0040-generated-music-orchestrated-across-chosen-parts.md) | Generated music shared across the chosen parts by instrument: each section the whole harmony, tune on top, bass below doubled an octave down, chord between, voice-led (`Orchestrate.*`). |
| [0041](decisions/0041-where-generated-music-goes.md) | Nothing chosen: shared across every part; bars of one part: all of it there; bars of several: shared across them. No part is ever added. |
| [0042](decisions/0042-a-single-line-to-one-part-and-a-span-filled-once.md) | A single line (melody, motif) goes to one part, never shared; chosen bars are filled once, cut or left empty, never repeated. |
| [0043](decisions/0043-an-audition-plays-every-note-on-a-piano.md) | An audition is the result itself: every note on a piano, drums on a kit, heard where it would go - nothing fitted. |
| [0044](decisions/0044-a-name-click-lets-go-of-bars-chosen-elsewhere.md) | A click on a part's name lets go of bars chosen in other parts, so the next idea goes to it. |
| [0045](decisions/0045-built-on-the-users-own-mac.md) | `build-mac.command` builds the app on the user's Mac and puts it in Applications; CI still builds and tests every push. |
| [0046](decisions/0046-blocks-chords-keep-their-root.md) | A Blocks chord carries its root into the score; the Chords lane names it from there while its notes are unchanged. |
| [0047](decisions/0047-blocks-offers-a-chords-real-inversions.md) | Blocks offers each chord its own inversions: two for a triad, three for a seventh, up to six for a thirteenth. |
| [0048](decisions/0048-escape-lets-go-of-everything.md) | Escape lets go of everything - selection, chosen bars and the caret's part - so the next idea goes to every part, a single line to the top one. |
| [0049](decisions/0049-undo-brings-back-generated-ideas.md) | A Generate is a step in Undo: Cmd+Z brings back the list of ideas from before it. |
| [0050](decisions/0050-choosing-several-parts-and-blocks-of-bars.md) | Cmd and Shift on names choose several parts, Cmd+A every part (again: every note); Cmd on bars adds blocks, each filled on its own. |
| [0051](decisions/0051-no-more-button.md) | No More button: Generate again makes a fresh list, and Undo brings back the last. |
| [0052](decisions/0052-vary-notes-across-parts-goes-after.md) | Vary Notes on music in several parts puts the variation after it, shared across those parts. |
| [0035](decisions/0035-space-plays-from-the-start.md) | Space plays from bar 1, Shift+Space and Play from the caret; |◀ and ▶| (Home, End) go to the start and the end of the music. |
| [0034](decisions/0034-follow-scrolls-smoothly.md) | Following scrolls smoothly by default, the playhead a third of the way across, on a steady clock; turning pages is a choice in the Play menu. |
| [0016](decisions/0016-step-input-with-the-keys-people-know.md) | Step-time input with MuseScore's keys; letters start in the instrument's register; MIDI keys write chords. |
| [0007](decisions/0007-one-performance-and-the-macs-own-orchestra.md) | One performance feeds playback, WAV and .mid; sound from Apple's General MIDI Audio Unit, a built-in synth elsewhere. |
| [0008](decisions/0008-general-midi-gets-two-cc-lanes.md) | General MIDI gets AutoCC's CC7 and CC11; a .mid gets all four lanes. |
| [0023](decisions/0023-a-synth-per-sixteen-channels.md) | A synth per sixteen channels (up to four), so every part of a big score has a channel and a sound of its own. |
| [0013](decisions/0013-autocc-computed-for-a-whole-part.md) | AutoCC's presets and envelope, ported and computed once per part. |

### Files
| | |
| --- | --- |
| [0021](decisions/0021-musicxml-in-and-out.md) | MusicXML import (.musicxml, .xml, .mxl) and export, in the core with its own XML parser; export written from the layout. |

## Borrowed from the family, unchanged

| | |
| --- | --- |
| `Engines/good-idea`, `midi-catalogue`, `midi-suggester`, `midi-variator`, `starting-blocks` | The engines, at the commits in [`Engines/VENDORED.md`](../Engines/VENDORED.md) |
| `Source/Core/ScaleModel.h` | ScaleView's scales, spelling and chord naming |
| `Detect.cpp` key finder | Midi Suggester's `detectKey`, weights as they stand |
| `AutoCC.cpp` | AutoCC.jsfx's presets and envelope |
| `Engrave.cpp` rules | Starting Blocks Notation's engraver, ported and grown |
| `Theme.*` | The house colour scheme |

## Where it goes next

Roughly in order: a page view; dynamics, articulations and slurs (with
articulation switching for sample libraries); drawable CC lanes seeded by
AutoCC; VST3 and CLAP instruments per part and SoundFonts through the Mac's
synth; real-time MIDI recording; Windows.
