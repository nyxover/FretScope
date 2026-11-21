#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace theory
{
    struct Scale
    {
        const char* name;
        std::vector<int> steps; // demi-tons depuis la fondamentale
    };

    inline const std::vector<Scale>& scales()
    {
        static const std::vector<Scale> s = {
            { "Off",               {} },
            { "Major",             { 0, 2, 4, 5, 7, 9, 11 } },
            { "Natural Minor",     { 0, 2, 3, 5, 7, 8, 10 } },
            { "Harmonic Minor",    { 0, 2, 3, 5, 7, 8, 11 } },
            { "Melodic Minor",     { 0, 2, 3, 5, 7, 9, 11 } },
            { "Dorian",            { 0, 2, 3, 5, 7, 9, 10 } },
            { "Phrygian",          { 0, 1, 3, 5, 7, 8, 10 } },
            { "Lydian",            { 0, 2, 4, 6, 7, 9, 11 } },
            { "Mixolydian",        { 0, 2, 4, 5, 7, 9, 10 } },
            { "Locrian",           { 0, 1, 3, 5, 6, 8, 10 } },
            { "Phrygian Dominant", { 0, 1, 4, 5, 7, 8, 10 } },
            { "Major Pentatonic",  { 0, 2, 4, 7, 9 } },
            { "Minor Pentatonic",  { 0, 3, 5, 7, 10 } },
            { "Blues",             { 0, 3, 5, 6, 7, 10 } },
            { "Whole Tone",        { 0, 2, 4, 6, 8, 10 } },
            { "Diminished W-H",    { 0, 2, 3, 5, 6, 8, 9, 11 } },
            { "Diminished H-W",    { 0, 1, 3, 4, 6, 7, 9, 10 } },
            { "Chromatic",         { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
        };
        return s;
    }

    inline const char* noteName (int pc)
    {
        static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return n[((pc % 12) + 12) % 12];
    }

    // Degré par rapport à la fondamentale (0..11)
    inline const char* degreeName (int semis)
    {
        static const char* n[12] = { "1", "b2", "2", "b3", "3", "4", "b5", "5", "b6", "6", "b7", "7" };
        return n[((semis % 12) + 12) % 12];
    }

    // Intervalle entre deux notes (en demi-tons, >= 0)
    inline juce::String intervalName (int semis)
    {
        static const char* n[12] = { "P1", "m2", "M2", "m3", "M3", "P4", "TT", "P5", "m6", "M6", "m7", "M7" };
        if (semis == 12) return "P8";
        auto s = juce::String (n[semis % 12]);
        if (semis > 12) s << " +" << (semis / 12) << "oct";
        return s;
    }

    struct Instrument
    {
        const char* name;
        std::vector<int> tuning; // notes MIDI à vide, corde grave -> aiguë
        int frets;
    };

    inline const std::vector<Instrument>& instruments()
    {
        static const std::vector<Instrument> i = {
            { "Guitar", { 40, 45, 50, 55, 59, 64 }, 24 }, // E2 A2 D3 G3 B3 E4
            { "Bass",   { 28, 33, 38, 43 },         24 }, // E1 A1 D2 G2
        };
        return i;
    }
}
