#include "Check.h"

#include "Generators.h"
#include "LuaEngine.h"

using namespace nt;

namespace
{
LuaEngine& engineInstance()
{
    static LuaEngine e;
    return e;
}

Score withViola()
{
    Score s;
    Part p; p.id = s.newId(); p.name = "Viola"; p.instrument = "vla";
    s.parts = { p };
    return s;
}
} // namespace

TEST ("generators: all five load")
{
    auto& e = engineInstance();
    CHECK_EQ (e.error(), std::string());
    CHECK_EQ (e.generators().size(), size_t (5));
}

TEST ("generators: every one makes music, and every setting can be chosen")
{
    auto& e = engineInstance();
    auto s = withViola();
    // A tune to give the selection-based ones something to read.
    Selection sel;
    const int tune[] = { 60, 60, 67, 67, 69, 69, 67 };
    for (int i = 0; i < 7; ++i) sel.insert (writeNote (s, s.parts[0].id, i * PPQ, i == 6 ? 2 * PPQ : PPQ, tune[i], 0));

    for (const auto& g : e.generators())
    {
        auto ctx = contextFor (s, s.parts[0].id, 0, g.needsSelection ? sel : Selection {});
        const auto settings = e.settings (g.id, ctx);
        CHECK (! settings.empty());
        for (const auto& st : settings)
            for (int i = 0; i < static_cast<int> (st.names.size()); ++i)
            {
                e.set (g.id, st.id, i, ctx);
                CHECK_EQ (e.error(), std::string());
            }
        e.reset (g.id);
        e.useKey (g.id, ctx.root, ctx.scale);
        const auto out = e.generate (g.id, ctx, 1, 3);
        CHECK_EQ (out.error, std::string());
        CHECK (out.ok);
        CHECK (! out.results.empty());
        for (const auto& r : out.results)
        {
            CHECK (! r.parts.empty());
            CHECK (r.length > 0);
        }
    }
}

TEST ("generators: the catalogue writes a viola line in the viola's range")
{
    auto& e = engineInstance();
    auto s = withViola();
    const auto ctx = contextFor (s, s.parts[0].id, 0, {});
    e.reset ("midi-catalogue");
    const auto out = e.generate ("midi-catalogue", ctx, 1, 6);
    CHECK (! out.results.empty());
    const auto& vla = instrumentById ("vla");
    for (const auto& r : out.results)
        for (const auto& p : r.parts)
            for (const auto& n : p.notes)
                CHECK (n.pitch >= vla.low && n.pitch <= vla.high);
}

TEST ("generators: a result goes into the part, fitted to its instrument")
{
    auto s = withViola();
    GeneratedResult r;
    r.length = 4 * PPQ;
    GeneratedPart melody;
    melody.name = "Melody";
    for (int i = 0; i < 4; ++i) { Note n; n.start = i * PPQ; n.length = PPQ; n.pitch = 88 + i; melody.notes.push_back (n); }
    GeneratedPart bass;
    bass.name = "Bass";
    { Note n; n.start = 0; n.length = 4 * PPQ; n.pitch = 36; bass.notes.push_back (n); }
    r.parts = { melody, bass };
    const auto report = insertResult (s, r, s.parts[0].id, 4 * PPQ);
    CHECK_EQ (s.parts.size(), size_t (2));
    CHECK_EQ (report.newNotes.size(), size_t (5));
    const auto& vla = instrumentById ("vla");
    for (const auto& n : s.parts[0].notes)
    {
        CHECK (n.pitch >= vla.sweetLow && n.pitch <= vla.sweetHigh);
        CHECK (n.start >= 4 * PPQ);
    }
    CHECK_EQ (s.parts[1].instrument, std::string ("vc"));   // a bass under strings is a cello
}

TEST ("generators: fitting moves by octaves only")
{
    const auto& vc = instrumentById ("vc");
    std::vector<Note> notes;
    for (int p : { 72, 76, 79 }) { Note n; n.pitch = p; notes.push_back (n); }
    const auto fitted = fitToInstrument (notes, vc);
    for (size_t i = 0; i < notes.size(); ++i)
    {
        CHECK_EQ ((fitted[i].pitch - notes[i].pitch) % 12, 0);
        CHECK (fitted[i].pitch >= vc.sweetLow && fitted[i].pitch <= vc.sweetHigh);
    }
}
