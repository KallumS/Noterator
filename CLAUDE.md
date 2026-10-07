# Noterator

A JUCE notation app for macOS (Apple silicon only; Windows later) whose music
mostly comes from the family's generators. The score is MIDI; the notation is
engraved from it every time it is drawn.

## Where the reasons are

- **[`docs/decisions/`](docs/decisions/README.md)** - one record per choice
  someone could reasonably make the other way. Referred to by number:
  **(0006)** is `docs/decisions/0006-instruments-carry-their-context.md`. Not
  edited to stay true: a reversed decision keeps its text and gains a pointer.
- **[`docs/sessions/`](docs/sessions/README.md)** - one log per working
  session: the route, the mistakes, what looked broken and was not.

Write to both. A rule here without its reason gets undone.

## Shape of it

| | |
| --- | --- |
| `Source/Core/` | The music. **No JUCE in here, ever** - it is what makes it testable in seconds. |
| `Source/Core/Score.*` | Parts of notes in ticks (960 a quarter), meters and keys by bar, tempos by tick. |
| `Source/Core/Engrave.*` | Notes in, a laid-out page out, in staff spaces. Decides; draws nothing. |
| `Source/Core/Edit.*` | Every change the editor can make, as a function. |
| `Source/Core/Instruments.*` | The one table of what each instrument is (0006). |
| `Source/Core/Spelling.*`, `ScaleModel.h` | Keys, spelling, signatures. `ScaleModel.h` is ScaleView's, **unchanged**. |
| `Source/Core/Detect.*` | Chords and keys along the score. |
| `Source/Core/AutoCC.*` | AutoCC's curves, computed for a whole part. |
| `Source/Core/Perform.*`, `MidiFile.*`, `ScoreFile.*` | The score as MIDI events, as a .mid, as a .noterator. |
| `Source/Engines/` | The Lua host and what happens around a generator. Also no JUCE. |
| `Engines/<app>/` | The family's engines, **copied unchanged** (0003). |
| `Engines/adapters/` | The only Lua written here: one adapter per engine. |
| `Source/App/` | The JUCE app. `Controller` owns the score; everything else asks it. |
| `Source/App/ScoreRenderer.*` | The ink: a layout into Bravura glyphs. Shared with `tools/RenderScore.cpp`. |
| `Tests/` | Core tests, no JUCE. |

## Working in it

```sh
cmake -B build-core -G Ninja -DNOTERATOR_BUILD_APP=OFF && cmake --build build-core && ./build-core/NoteratorTests
lua5.4 tools/try_generators.lua
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
./build/NoteratorRender_artefacts/Release/NoteratorRender demo out.png 12 light
```

- **Look at a render after any engraving change** (0010). `demo` is a page that
  exercises most of the engraver; `generated` drops Catalogue lines into a
  string trio. Crop and enlarge (`convert big.png -crop ...`) to see detail.
- The app runs headless on Linux under `Xvfb :99` and can be driven with
  `xdotool` and screenshotted with `import -window root`. There is no sound
  device in the container; the built-in synth runs silently.
- **The Mac app cannot be built here.** GitHub Actions builds it on every push
  (`.github/workflows/build.yml`); check that run after touching anything
  Apple-only (`AudioEngine.cpp`'s Audio Unit code, CMake's Apple settings).
- When fixing a bug, have the test fail on the old code first.

## Rules that are easy to break

- **The notation is never stored** (0002). If something needs remembering
  about a note, it is a field on `Note`, not a property of the page.
- **Never edit `Engines/<app>/*.lua`.** Fix it in that app's repository and copy
  it across (`tools/sync_engines.sh`), so the copies never drift. Record the
  commit in `Engines/VENDORED.md`.
- **`ScaleModel.h` is ScaleView's file.** Chord naming changes start there.
- **The audio thread does not allocate or lock** beyond try-locks: the
  sequence is swapped in whole (`std::atomic_store`), messages from the window
  queue under a lock the audio thread only tries.
- **An Audio Unit is created on the message thread** - including for audio
  export, which makes its synth before starting its thread.
- **Pointers into `score.parts` die when a part is added.** Hold ids
  (`Generators.cpp::insertResult` crashed on this before it was written that way).
- **The engraver must stay near-linear.** Anything per element per measure
  goes through the by-measure buckets; the 200-bar test holds it under a
  second.
- **General MIDI gets CC7 and CC11 only** from AutoCC; a .mid gets all four (0008).
- Letters in shortcuts arrive in either case: compare them upper-cased.

## Colour

The house scheme (Good Idea's `docs/COLOUR.md`), in `Theme.*`: a dark cool-grey
ground, light grey controls with **dark ink on every button**, one yellow
(`#FFF200`) for what is on - a chosen button, the selection, the sounding note,
the caret in note input. Every grey has R < G < B. Red `#D2483F` is warnings
only. The page is set in ink on its own paper and can be turned to black on
white (Light page); on white the accent is the same yellow shaded down.

## Engraving conventions

Starting Blocks Notation's, ported and grown (see `Engrave.h`): a note is
written as its share of the bar where it was gated short; a duration with a
symbol of its own is written as that symbol; rests show the beat; the key
signature is scored; stems measure from the far head; beams lean a quarter of
the interval, at most a space and a bit; accidentals last to the bar, carry
across a tie and pack into columns; a crossed head's side is decided once.
New here: every staff shares one set of columns per bar (0004); a sustained
low note under moving music goes to the lower voice; triplets are found per
beat by picking the coarsest grid that fits.
