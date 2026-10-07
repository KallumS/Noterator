# Prompt for the next session

Copy everything in the box below into a new Claude Code session on the
Noterator repository. Replace the line under "This session" with what you
want done; if you leave it, the session starts on the first roadmap item.

```text
We're continuing work on Noterator (KallumS/Noterator): a macOS notation app
(JUCE 8, C++17, Apple silicon only) whose music mostly comes from my
generators, which run unchanged inside it through embedded Lua: Good Idea
(the main one), Midi Suggester and Midi Variator in the Generate tab, and
Starting Blocks as its own Blocks toolbox. Bars can be chosen by dragging and
generated into; scores open and save as MIDI and MusicXML. It works; I have
tested it on my Mac.

I'm not technical: explain things in plain words, show me screenshots of what
changed, and make sure each change reaches me as the downloadable Mac app from
GitHub Actions.

Before doing anything:
1. If the work so far (branch claude/epic-hypatia-z0ya3r) is not yet in your
   branch, start your branch from it.
2. Read CLAUDE.md, then docs/ARCHITECTURE.md, then the latest log in
   docs/sessions/. Follow their rules - especially: never edit the vendored
   engines in Engines/<app>/, no JUCE in Source/Core, look at a render after
   any engraving change, and commit and push early.
3. Build and run the tests (core, app tests, tools/try_generators.lua), and
   render the demo page, to confirm everything is green before changing
   anything.

This session:
<what I want next - for example: "a page view", "dynamics and articulations",
"drawable CC lanes", "VST3 instruments per part", or a list
of things I noticed while testing>

The roadmap, in the order I'm most likely to want it: a page view (systems on
pages, title); dynamics, articulations and slurs, with articulation switching
for sample libraries (and carried in MusicXML); drawable CC lanes seeded by
AutoCC; VST3 and CLAP instruments per part and SoundFonts through the Mac's
General MIDI synth; real-time MIDI recording; Windows.

When you finish: write the session log in docs/sessions/, add a decision
record in docs/decisions/ (and a line in docs/ARCHITECTURE.md) for any choice
someone could reasonably make the other way, update CLAUDE.md if a rule
changed, push, check that the GitHub Actions run is green on both Linux and
macOS, and tell me where to download the new app.
```

## Things worth knowing before you paste it

- The other repositories (Good-Idea, Midi-Catalogue, ScaleView and so on) are
  only needed if the session has to copy an engine update across; it will ask
  for them if so.
- If you want the app to open with a normal double-click, the session can set
  up Apple Developer ID signing - it needs your Apple Developer account, added
  to the repository as secrets.
