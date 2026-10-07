/*
    Generators - what happens around a generator: the context it is asked in,
    and putting what it made into the score.

    A generator writes for an instrument when it can (Midi Catalogue does);
    when it cannot, what it made is moved by octaves into the part's own
    range, preferring where the instrument sounds like itself, so a Good Idea
    tune dropped onto a cello part lands in the cello's register and not the
    violin's (decision 0006).
*/

#pragma once

#include "Edit.h"
#include "Instruments.h"
#include "LuaEngine.h"

namespace nt
{

// The octave shift that puts the most notes inside the instrument's sweet
// register, then each note still outside its range folded in by octaves.
std::vector<Note> fitToInstrument (std::vector<Note> notes, const Instrument& inst);

// The context a generator is asked in: the key, meter and tempo at `at`, the
// part's instrument, and the selected notes, if any, relative to the bar the
// selection starts in.
GeneratorContext contextFor (const Score& score, uint32_t partId, Tick at, const Selection& selection);

struct InsertOptions
{
    bool fitToInstrument = true;
    bool replace = true;          // clear what the target parts had in the span first
};

struct InsertReport
{
    Selection newNotes;
    std::vector<uint32_t> newParts;
};

// Drops a result into the score at `at`. Its first part goes into the target
// part; any others go into parts of their own, reused when the score already
// has one by that name and instrument, created otherwise.
InsertReport insertResult (Score& score, const GeneratedResult& result, uint32_t targetPartId, Tick at,
                           const InsertOptions& options = {});

// The instrument a generated part should be played by when it gets a part of
// its own.
std::string instrumentForGeneratedPart (const GeneratedPart& part, const Instrument& target);

} // namespace nt
