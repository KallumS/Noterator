--[[ Starting Blocks, in Noterator.

     The smallest useful pieces of music: a chord, an arpeggio, a run, a
     melodic step or leap, a bass note, a single drum. One result per degree
     of the scale, so the whole key can be auditioned at a glance and the
     block you want dropped in. ]]

local C = engine("adapters/common.lua")
local E = engine("starting-blocks/sb_engine.lua")

local A = {
  id = "starting-blocks",
  name = "Starting Blocks",
  description = "The smallest useful pieces - chords, arpeggios, runs, steps and leaps, bass notes, drum hits - on every degree of the key.",
  input = "none",
}

local function range(a, b) local out = {} for i = a, b do out[#out + 1] = i end return out end
local function names(list) local out = {} for i, x in ipairs(list) do out[i] = x.name or x end return out end
local function cat(...)
  local set = {}
  for _, c in ipairs({ ... }) do set[c] = true end
  return function(st) return set[st.cat] == true end
end

local barValues, barNames = {}, {}
for i, b in ipairs(E.BAR_LENGTHS) do barValues[i], barNames[i] = b.bars, b.name .. (b.bars == 1 and " bar" or " bars") end

local LIST = {
  C.setting("cat", "Block", E.CATEGORIES),
  C.setting("dia", "Chord", range(1, #E.DIATONIC), names(E.DIATONIC), { when = cat("Chord", "Arpeggio", "Bass") }),
  C.setting("inv", "Inversion", range(0, 3), E.INVERSIONS, { when = cat("Chord", "Arpeggio") }),
  C.setting("chop", "Strike every", range(1, #E.RATES), names(E.RATES), { when = cat("Chord") }),
  C.setting("pattern", "Direction", range(1, #E.DIRECTIONS), E.DIRECTIONS, { when = cat("Arpeggio") }),
  C.setting("runDir", "Direction", range(1, #E.DIRECTIONS), E.DIRECTIONS, { when = cat("Run") }),
  C.setting("interval", "Interval", range(1, #E.INTERVALS), names(E.INTERVALS), { when = cat("Melody") }),
  C.setting("melDir", "Direction", { 1, 2 }, { "Up", "Down" }, { when = function(st) return st.cat == "Melody" and not E.isSustain(st) end }),
  C.setting("shape", "Shape", range(1, #E.SHAPES), E.SHAPES, { when = function(st) return st.cat == "Melody" and not E.isSustain(st) end }),
  C.setting("bassTone", "Bass note", range(1, #E.BASS_TONES), E.BASS_TONES, { when = cat("Bass") }),
  C.setting("drumPiece", "Drum", range(1, #E.DRUM_PIECES), names(E.DRUM_PIECES), { when = cat("Drums") }),
  C.setting("rate", "Rate", range(1, #E.RATES), names(E.RATES), { when = cat("Arpeggio", "Run", "Melody", "Bass") }),
  C.setting("drumRate", "Rate", {}, {}, {
    when = function(st) return st.cat == "Drums" and #E.DRUM_PIECES[st.drumPiece].rates > 0 end,
    dynamic = function(st)
      local r = E.DRUM_PIECES[st.drumPiece].rates
      return r, r, {}
    end }),
  C.setting("rateMod", "Feel", range(1, #E.RATE_MODS), names(E.RATE_MODS), { when = cat("Chord", "Arpeggio", "Run", "Melody", "Bass", "Drums") }),
  C.setting("octaves", "Octaves", range(1, 4), nil, { when = cat("Arpeggio", "Run") }),
  C.setting("lengthMode", "Measure by", E.LENGTH_MODES, nil, { when = cat("Arpeggio", "Run") }),
  C.setting("repeats", "Repeats", { 1, 2, 3, 4, 6, 8, 12, 16 }, nil, {
    when = function(st) return (st.cat == "Arpeggio" or st.cat == "Run") and st.lengthMode == "Repeats" end }),
  C.setting("bars", "Length", barValues, barNames, {
    when = function(st)
      if st.cat == "Arpeggio" or st.cat == "Run" then return st.lengthMode == "Bars" end
      return st.cat == "Chord" or st.cat == "Bass" or st.cat == "Drums"
    end }),
  C.setting("oct", "Octave", range(-2, 2), { "-2", "-1", "0", "+1", "+2" }, { when = cat("Chord", "Arpeggio", "Run", "Melody") }),
}

function A.newState() return E.newState() end
function A.settings(st, ctx) return C.describe(LIST, st, ctx) end
function A.set(st, id, index, ctx)
  C.set(LIST, st, id, index, ctx)
  E.clampState(st)
end
function A.useKey(st, root, scale)
  st.root, st.scale = root, scale
  E.clampState(st)
end

function A.generate(st, ctx, seed, count)
  st.barBeats = ctx.barBeats
  E.clampState(st)
  math.randomseed(seed)
  local out = {}
  local degrees = st.cat == "Drums" and 1 or E.scaleLen(st)
  for d = 0, degrees - 1 do
    st.degree = d
    local block = E.generate(st)
    local title = st.cat == "Drums" and block.name
                  or ("%s  %s"):format(E.degreeNumeral(st, d), block.name)
    out[#out + 1] = { title = title, detail = E.degreeTitle(st, d), beats = block.beats,
                      parts = { { name = st.cat, drums = st.cat == "Drums", notes = C.notes(block.notes) } } }
  end
  st.degree = 0
  return { results = out }
end

return A
