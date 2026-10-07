/*
    Perform - the score as the MIDI a player or a file receives.

    One function turns the score into a sorted list of channel events, and
    playback, audio export and MIDI export all read that list, so what you hear
    while writing is what the bounce and the .mid hold (decision 0007).
*/

#pragma once

#include "AutoCC.h"
#include "Score.h"

#include <vector>

namespace nt
{

struct PlayEvent
{
    enum Type { noteOff = 0, controller = 1, program = 2, noteOn = 3 };

    Tick at = 0;
    Type type = noteOn;
    int channel = 0;      // 0..15
    int data1 = 0;        // pitch, controller or program
    int data2 = 0;        // velocity or value
    int part = -1;        // index into score.parts
    uint32_t noteId = 0;
};

struct PerformOptions
{
    Tick from = 0;
    Tick to = -1;                     // -1: the end of the score
    bool autoCC = true;
    // Which AutoCC controllers to send. General MIDI reads CC1 as vibrato
    // depth, so playback through the built-in sounds sends expression and
    // volume only; a file for a sample library gets all four (decision 0008).
    std::vector<int> autoCCControllers;
    bool honourMuteAndSolo = true;
    bool includeSetup = true;         // program and volume at the top of each channel
};

// The channel each part plays on: in order, skipping 10, which belongs to the
// drums. Past fifteen pitched parts the channels are shared.
std::vector<int> channelsForParts (const Score& score);

// Events from `from` to `to`, shifted so `from` is tick 0, sorted with
// note-offs before note-ons at the same tick.
std::vector<PlayEvent> perform (const Score& score, const PerformOptions& options = {});

bool partAudible (const Score& score, size_t partIndex);

} // namespace nt
