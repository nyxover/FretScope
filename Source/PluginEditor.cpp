#include "PluginEditor.h"
#include "Theory.h"
#include <cmath>

namespace
{
    const juce::Colour kBg      { 0xff0b0d10 };
    const juce::Colour kBoard   { 0xff131820 };
    const juce::Colour kLine    { 0xff2a3340 };
    const juce::Colour kAccent  { 0xff4fd1ff };
    const juce::Colour kRoot    { 0xffffb347 };
    const juce::Colour kOut     { 0xffff5c6c };
    const juce::Colour kMuted   { 0xff8a94a3 };

    int choiceIndex (juce::AudioProcessorValueTreeState& s, const char* id)
    {
        return (int) std::lround (s.getRawParameterValue (id)->load());
    }
}

//==============================================================================
void FretboardView::paint (juce::Graphics& g)
{
    auto& apvts = proc.apvts;
    const auto& inst   = theory::instruments()[(size_t) choiceIndex (apvts, "mode")];
    const auto& scale  = theory::scales()[(size_t) choiceIndex (apvts, "scale")];
    const int root     = choiceIndex (apvts, "root");
    const bool noteLbl = choiceIndex (apvts, "labels") == 1;
    const bool hex     = apvts.getRawParameterValue ("hex")->load() > 0.5f;

    bool inScale[12] = {};
    for (int s : scale.steps) inScale[(root + s) % 12] = true;
    const bool scaleOn = ! scale.steps.empty();

    const int nStrings = (int) inst.tuning.size();
    const int nFrets   = inst.frets;

    auto area  = getLocalBounds().toFloat().reduced (6.0f);
    auto board = area.withTrimmedLeft (40.0f).withTrimmedTop (22.0f).withTrimmedBottom (4.0f);

    auto fretX = [&] (double f)
    {
        return board.getX() + board.getWidth() * (float) ((1.0 - std::pow (2.0, -f / 12.0)) / 0.75);
    };
    const float rowH = board.getHeight() / (float) nStrings;
    auto stringY = [&] (int s) { return board.getBottom() - ((float) s + 0.5f) * rowH; };
    auto noteX = [&] (int f)
    {
        return f == 0 ? board.getX() - 20.0f : 0.5f * (fretX (f - 1) + fretX (f));
    };

    g.setColour (kBoard);
    g.fillRoundedRectangle (board, 4.0f);

    // repères de frettes
    g.setFont (juce::FontOptions (11.0f));
    for (int f = 1; f <= nFrets; ++f)
    {
        const bool single = (f == 3 || f == 5 || f == 7 || f == 9 || f == 15 || f == 17 || f == 19 || f == 21);
        const bool dbl    = (f == 12 || f == 24);
        g.setColour (kLine.withAlpha (0.55f));
        if (single || dbl)
        {
            const float cx = noteX (f);
            const float cy = board.getCentreY();
            if (single) g.fillEllipse (cx - 6, cy - 6, 12, 12);
            if (dbl)
            {
                g.fillEllipse (cx - 6, cy - rowH * 0.9f - 6 + rowH * 0.45f, 12, 12);
                g.fillEllipse (cx - 6, cy + rowH * 0.9f - 6 - rowH * 0.45f, 12, 12);
            }
            g.setColour (kMuted);
            g.drawText (juce::String (f), juce::Rectangle<float> (cx - 14, area.getY(), 28, 18),
                        juce::Justification::centred);
        }
    }

    // frettes + sillet
    for (int f = 0; f <= nFrets; ++f)
    {
        g.setColour (f == 0 ? juce::Colour (0xffd8dee9) : kLine);
        const float x = fretX (f);
        g.drawLine (x, board.getY(), x, board.getBottom(), f == 0 ? 4.0f : 1.5f);
    }

    // cordes
    for (int s = 0; s < nStrings; ++s)
    {
        const float thickness = 1.0f + 2.2f * (float) (nStrings - 1 - s) / (float) nStrings;
        g.setColour (juce::Colour (0xff9aa5b4).withAlpha (0.75f));
        g.drawLine (board.getX(), stringY (s), board.getRight(), stringY (s), thickness);
    }

    // notes
    const float r = juce::jmin (12.0f, rowH * 0.42f);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, r * 0.95f), juce::Font::bold));

    for (int s = 0; s < nStrings; ++s)
    {
        for (int f = 0; f <= nFrets; ++f)
        {
            const int pitch = inst.tuning[(size_t) s] + f;
            if (pitch > 127) continue;
            const int pc  = pitch % 12;
            const int deg = (pc - root + 12) % 12;
            const bool inS = inScale[pc];

            bool played = false;
            if (hex)
            {
                const int ch = nStrings - 1 - s; // canal 1 = corde aiguë
                played = proc.isNoteHeld (ch, pitch);
            }
            else
            {
                for (int ch = 0; ch < 16 && ! played; ++ch)
                    played = proc.isNoteHeld (ch, pitch);
            }

            if (! played && ! (scaleOn && inS)) continue;

            const float cx = noteX (f), cy = stringY (s);
            const auto label = noteLbl ? juce::String (theory::noteName (pc)) : juce::String (theory::degreeName (deg));
            const bool isRoot = (deg == 0);

            if (played)
            {
                const auto col = (scaleOn && ! inS) ? kOut : (isRoot ? kRoot : kAccent);
                g.setColour (col.withAlpha (0.25f));
                g.fillEllipse (cx - r - 4, cy - r - 4, 2 * (r + 4), 2 * (r + 4));
                g.setColour (col);
                g.fillEllipse (cx - r, cy - r, 2 * r, 2 * r);
                g.setColour (kBg);
            }
            else
            {
                const auto col = isRoot ? kRoot : kAccent;
                g.setColour (kBoard.brighter (0.1f));
                g.fillEllipse (cx - r * 0.85f, cy - r * 0.85f, 1.7f * r, 1.7f * r);
                g.setColour (col.withAlpha (0.55f));
                g.drawEllipse (cx - r * 0.85f, cy - r * 0.85f, 1.7f * r, 1.7f * r, 1.3f);
                g.setColour (col.withAlpha (0.85f));
            }
            g.drawText (label, juce::Rectangle<float> (cx - r, cy - r, 2 * r, 2 * r), juce::Justification::centred);
        }
    }

    // noms des cordes à vide (à gauche)
    g.setColour (kMuted);
    g.setFont (juce::FontOptions (11.0f));
    for (int s = 0; s < nStrings; ++s)
        g.drawText (juce::MidiMessage::getMidiNoteName (inst.tuning[(size_t) s], true, true, 4),
                    juce::Rectangle<float> (area.getX() - 2, stringY (s) - 8, 22, 16), juce::Justification::centredRight);
}

//==============================================================================
FretScopeAudioProcessorEditor::FretScopeAudioProcessorEditor (FretScopeAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), board (p)
{
    laf.setColourScheme (juce::LookAndFeel_V4::getMidnightColourScheme());
    setLookAndFeel (&laf);

    auto setup = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, kMuted);
        l.setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (l);
    };
    setup (modeLabel, "Instrument");
    setup (rootLabel, "Root");
    setup (scaleLabel, "Scale");
    setup (labelsLabel, "Labels");

    for (auto& i : theory::instruments()) modeBox.addItem (i.name, modeBox.getNumItems() + 1);
    for (int i = 0; i < 12; ++i)          rootBox.addItem (theory::noteName (i), i + 1);
    for (auto& s : theory::scales())      scaleBox.addItem (s.name, scaleBox.getNumItems() + 1);
    labelsBox.addItem ("Intervals", 1);
    labelsBox.addItem ("Notes", 2);

    for (auto* c : { &modeBox, &rootBox, &scaleBox, &labelsBox }) addAndMakeVisible (c);
    addAndMakeVisible (hexButton);
    addAndMakeVisible (board);

    readout.setFont (juce::FontOptions (15.0f));
    readout.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.9f));
    readout.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (readout);

    modeAtt   = std::make_unique<CBAttach> (proc.apvts, "mode", modeBox);
    rootAtt   = std::make_unique<CBAttach> (proc.apvts, "root", rootBox);
    scaleAtt  = std::make_unique<CBAttach> (proc.apvts, "scale", scaleBox);
    labelsAtt = std::make_unique<CBAttach> (proc.apvts, "labels", labelsBox);
    hexAtt    = std::make_unique<BAttach>  (proc.apvts, "hex", hexButton);

    setResizable (true, true);
    setResizeLimits (720, 240, 1800, 600);
    setSize (1040, 330);
    startTimerHz (30);
}

FretScopeAudioProcessorEditor::~FretScopeAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void FretScopeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
}

void FretScopeAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (10);

    auto top = r.removeFromTop (46);
    auto cell = [&] (juce::Label& l, juce::Component& c, int w)
    {
        auto a = top.removeFromLeft (w);
        l.setBounds (a.removeFromTop (16));
        c.setBounds (a.reduced (0, 2));
        top.removeFromLeft (10);
    };
    cell (modeLabel, modeBox, 110);
    cell (rootLabel, rootBox, 70);
    cell (scaleLabel, scaleBox, 170);
    cell (labelsLabel, labelsBox, 110);
    hexButton.setBounds (top.removeFromLeft (110).withTrimmedTop (20));

    readout.setBounds (r.removeFromBottom (28));
    board.setBounds (r);
}

juce::String FretScopeAudioProcessorEditor::buildReadout() const
{
    const int root = choiceIndex (proc.apvts, "root");
    std::vector<int> notes;
    for (int n = 0; n < 128; ++n)
        for (int ch = 0; ch < 16; ++ch)
            if (proc.isNoteHeld (ch, n)) { notes.push_back (n); break; }

    if (notes.empty())
        return "Play something...";

    if (notes.size() > 8) notes.resize (8);

    juce::StringArray names, degrees, stack;
    for (size_t i = 0; i < notes.size(); ++i)
    {
        names.add (juce::MidiMessage::getMidiNoteName (notes[i], true, true, 4));
        degrees.add (theory::degreeName (((notes[i] % 12) - root + 12) % 12));
        if (i > 0) stack.add (theory::intervalName (notes[i] - notes[i - 1]));
    }

    auto s = names.joinIntoString (" ") + "   |   " + degrees.joinIntoString (" ");
    if (! stack.isEmpty()) s << "   |   " << stack.joinIntoString (", ");
    return s;
}

void FretScopeAudioProcessorEditor::timerCallback()
{
    // signature de l'état (notes + paramètres) : on ne repeint que si ça change
    uint64_t sig = 1469598103934665603ull;
    auto mix = [&sig] (uint64_t v) { sig = (sig ^ v) * 1099511628211ull; };

    for (int ch = 0; ch < 16; ++ch)
        for (int n = 0; n < 128; ++n)
            mix (proc.isNoteHeld (ch, n) ? (uint64_t) (ch * 128 + n + 1) : 0);

    for (auto id : { "mode", "root", "scale", "labels", "hex" })
        mix ((uint64_t) std::lround (proc.apvts.getRawParameterValue (id)->load() * 10.0f) + 7);

    if (sig != lastSignature)
    {
        lastSignature = sig;
        board.repaint();
        readout.setText (buildReadout(), juce::dontSendNotification);
    }
}
