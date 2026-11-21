#include "PluginEditor.h"
#include "Theory.h"

namespace
{
    void drawCheck (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
    {
        juce::Path p;
        p.startNewSubPath (r.getX(), r.getY() + r.getHeight() * 0.55f);
        p.lineTo (r.getX() + r.getWidth() * 0.38f, r.getBottom());
        p.lineTo (r.getRight(), r.getY());
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawCross (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
    {
        g.setColour (c);
        g.drawLine (r.getX(), r.getY(), r.getRight(), r.getBottom(), 3.0f);
        g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 3.0f);
    }

    juce::String noteWithOctave (int note) { return juce::MidiMessage::getMidiNoteName (note, true, true, 4); }

    bool anyHeld (FretScopeAudioProcessor& p, int note)
    {
        for (int ch = 0; ch < 16; ++ch)
            if (p.isNoteHeld (ch, note)) return true;
        return false;
    }

    void styleButton (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId, ui::cellOff);
        b.setColour (juce::TextButton::buttonOnColourId, ui::purple);
        b.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        b.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }
}

//==============================================================================
// Carte d'information : dernière note, identifiant de l'ensemble, rangée des 12 degrés
void InfoView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (ui::card);
    g.fillRoundedRectangle (b, 14.0f);
    g.setColour (ui::cardLine);
    g.drawRoundedRectangle (b, 14.0f, 1.5f);

    // --- pastille d'état (haut gauche)
    juce::Colour pillCol = ui::muted;
    juce::String pillText = "Waiting for MIDI...";
    enum { None, Check, Cross } icon = None;

    if (st.targetMode)
    {
        pillText = "Score " + juce::String (st.score);
        if (st.hasNote) pillText << " | MIDI Note " << st.note;
        if (st.matched) { pillCol = ui::green; icon = Check; pillText = "Correct | Score " + juce::String (st.score); }
    }
    else if (st.hasNote)
    {
        pillText = "MIDI Note " + juce::String (st.note);
        if (st.stringNo > 0) pillText << " | String " << st.stringNo;
        if (st.scaleOn) { pillCol = st.inScale ? ui::green : ui::red; icon = st.inScale ? Check : Cross; }
        else pillCol = ui::dotLight;
    }

    auto pill = juce::Rectangle<float> (20.0f, 18.0f, 330.0f, 40.0f);
    g.setColour (pillCol.withAlpha (0.12f));
    g.fillRoundedRectangle (pill, 12.0f);
    g.setColour (pillCol.withAlpha (0.9f));
    g.drawRoundedRectangle (pill, 12.0f, 1.8f);
    auto iconRect = juce::Rectangle<float> (pill.getX() + 16.0f, pill.getCentreY() - 8.0f, 16.0f, 16.0f);
    if (icon == Check) drawCheck (g, iconRect, pillCol);
    if (icon == Cross) drawCross (g, iconRect, pillCol);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText (pillText, pill.withTrimmedLeft (icon == None ? 16.0f : 42.0f), juce::Justification::centredLeft);

    // --- gamme (haut, centre)
    g.setColour (ui::muted);
    g.setFont (juce::FontOptions (15.0f));
    g.drawText (st.scaleOn ? juce::String (theory::noteName (st.root)) + " " + st.scaleName : "No scale selected",
                getLocalBounds().withHeight (60).withTrimmedLeft (360).withTrimmedRight (330),
                juce::Justification::centred);

    // --- grande note
    juce::String big = "-";
    float bigAlpha = 1.0f;
    if (st.targetMode && st.targetPc >= 0)
    {
        big = st.namesMode ? juce::String (theory::noteName (st.targetPc))
                           : juce::String (theory::degreeName ((st.targetPc - st.root + 12) % 12));
        g.setColour (ui::muted);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("FIND THIS NOTE", getLocalBounds().withY (58).withHeight (20), juce::Justification::centred);
    }
    else if (st.hasNote)
    {
        big = theory::noteName (st.note % 12);
        bigAlpha = st.held ? 1.0f : 0.45f;
    }
    g.setColour (juce::Colours::white.withAlpha (bigAlpha));
    g.setFont (juce::FontOptions (70.0f, juce::Font::bold));
    g.drawText (big, getLocalBounds().withY (72).withHeight (80), juce::Justification::centred);

    // --- identifiant de l'ensemble
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText (st.scaleOn ? "ID: " + juce::String (st.id) : "ID: -",
                getLocalBounds().withY (156).withHeight (32), juce::Justification::centred);
    g.setColour (ui::muted);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (st.scaleOn ? "Parent:" + juce::String (st.parent) + " | Rotation: R" + juce::String (st.rotation) : " ",
                getLocalBounds().withY (190).withHeight (22), juce::Justification::centred);

    // --- rangée des 12 degrés
    const float margin = 26.0f, gap = 8.0f;
    const float cellW = (b.getWidth() - 2 * margin - 11 * gap) / 12.0f;
    const float cellH = juce::jmin (64.0f, b.getHeight() - 228.0f);
    const float cy = b.getBottom() - 24.0f - cellH;
    const int outlineDeg = st.targetMode ? (st.targetPc >= 0 ? (st.targetPc - st.root + 12) % 12 : -1) : 0;

    g.setFont (juce::FontOptions (juce::jlimit (14.0f, 26.0f, cellW * 0.42f), juce::Font::bold));
    for (int i = 0; i < 12; ++i)
    {
        auto cell = juce::Rectangle<float> (b.getX() + margin + (float) i * (cellW + gap), cy, cellW, cellH);
        const bool inS = (st.scaleMask >> i) & 1;
        const bool played = (st.heldMask >> i) & 1;

        juce::Colour fill = ui::cellOff, text = ui::muted.withAlpha (0.6f);
        if (played)      { fill = (st.scaleOn && ! inS) ? ui::red : ui::green; text = ui::dark; }
        else if (inS)    { fill = ui::dotLight;                                text = ui::dark; }

        if (played) { g.setColour (fill.withAlpha (0.25f)); g.fillRoundedRectangle (cell.expanded (4.0f), 12.0f); }
        g.setColour (fill);
        g.fillRoundedRectangle (cell, 10.0f);
        if (i == outlineDeg && (st.scaleOn || st.targetMode))
        {
            g.setColour (ui::purple);
            g.drawRoundedRectangle (cell.reduced (1.0f), 10.0f, 2.5f);
        }
        g.setColour (text);
        g.drawText (st.namesMode ? juce::String (theory::noteName ((st.root + i) % 12)) : juce::String (i),
                    cell, juce::Justification::centred);
    }
}

//==============================================================================
void FretboardView::paint (juce::Graphics& g)
{
    auto& apvts = proc.apvts;
    const auto& inst   = theory::instruments()[(size_t) choiceOf (apvts, "mode")];
    const auto& scale  = theory::scales()[(size_t) choiceOf (apvts, "scale")];
    const int root     = choiceOf (apvts, "root");
    const bool noteLbl = choiceOf (apvts, "labels") == 1;
    const bool hex     = apvts.getRawParameterValue ("hex")->load() > 0.5f;

    bool inScale[12] = {};
    for (int s : scale.steps) inScale[(root + s) % 12] = true;
    const bool scaleOn = ! scale.steps.empty();

    const int nStrings = (int) inst.tuning.size();
    const int nFrets   = inst.frets;

    auto area  = getLocalBounds().toFloat().reduced (4.0f);
    auto board = area.withTrimmedLeft (64.0f).withTrimmedTop (22.0f).withTrimmedBottom (4.0f);

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

    g.setColour (ui::boardBg);
    g.fillRoundedRectangle (board, 4.0f);

    g.setFont (juce::FontOptions (11.0f));
    for (int f = 1; f <= nFrets; ++f)
    {
        const bool single = (f == 3 || f == 5 || f == 7 || f == 9 || f == 15 || f == 17 || f == 19 || f == 21);
        const bool dbl    = (f == 12 || f == 24);
        g.setColour (ui::line.withAlpha (0.55f));
        if (single || dbl)
        {
            const float cx = noteX (f), cy = board.getCentreY();
            if (single) g.fillEllipse (cx - 6, cy - 6, 12, 12);
            if (dbl)
            {
                g.fillEllipse (cx - 6, cy - rowH * 0.45f - 6, 12, 12);
                g.fillEllipse (cx - 6, cy + rowH * 0.45f - 6, 12, 12);
            }
            g.setColour (ui::muted);
            g.drawText (juce::String (f), juce::Rectangle<float> (cx - 14, area.getY(), 28, 18), juce::Justification::centred);
        }
    }

    for (int f = 0; f <= nFrets; ++f)
    {
        g.setColour (f == 0 ? juce::Colour (0xffd8dee9) : ui::line);
        const float x = fretX (f);
        g.drawLine (x, board.getY(), x, board.getBottom(), f == 0 ? 4.0f : 1.5f);
    }

    for (int s = 0; s < nStrings; ++s)
    {
        const float thickness = 1.0f + 2.2f * (float) (nStrings - 1 - s) / (float) nStrings;
        g.setColour (juce::Colour (0xff9aa5b4).withAlpha (0.75f));
        g.drawLine (board.getX(), stringY (s), board.getRight(), stringY (s), thickness);
    }

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
                played = proc.isNoteHeld (nStrings - 1 - s, pitch);   // canal 1 = corde aiguë
            else
                played = anyHeld (proc, pitch);

            if (! played && ! (scaleOn && inS)) continue;

            const float cx = noteX (f), cy = stringY (s);
            const auto label = noteLbl ? juce::String (theory::noteName (pc)) : juce::String (theory::degreeName (deg));

            if (played)
            {
                const auto col = (scaleOn && ! inS) ? ui::red : ui::green;
                g.setColour (col.withAlpha (0.25f));
                g.fillEllipse (cx - r - 4, cy - r - 4, 2 * (r + 4), 2 * (r + 4));
                g.setColour (col);
                g.fillEllipse (cx - r, cy - r, 2 * r, 2 * r);
            }
            else
            {
                g.setColour (ui::dotLight);
                g.fillEllipse (cx - r * 0.85f, cy - r * 0.85f, 1.7f * r, 1.7f * r);
            }
            if (deg == 0 && scaleOn)
            {
                g.setColour (ui::purple);
                g.drawEllipse (cx - r - 1, cy - r - 1, 2 * r + 2, 2 * r + 2, 2.5f);
            }
            g.setColour (ui::dark);
            g.drawText (label, juce::Rectangle<float> (cx - r, cy - r, 2 * r, 2 * r), juce::Justification::centred);
        }
    }

    g.setColour (ui::muted);
    g.setFont (juce::FontOptions (11.0f));
    for (int s = 0; s < nStrings; ++s)
        g.drawText (noteWithOctave (inst.tuning[(size_t) s]),
                    juce::Rectangle<float> (area.getX() - 2, stringY (s) - 8, 24, 16), juce::Justification::centredRight);
}

//==============================================================================
void KeyboardView::paint (juce::Graphics& g)
{
    auto& apvts = proc.apvts;
    const int modeIdx = choiceOf (apvts, "mode");
    const auto& scale = theory::scales()[(size_t) choiceOf (apvts, "scale")];
    const int root    = choiceOf (apvts, "root");

    bool inScale[12] = {};
    for (int s : scale.steps) inScale[(root + s) % 12] = true;
    const bool scaleOn = ! scale.steps.empty();

    const int nOct  = modeIdx == 0 ? 5 : 4;          // guitare : C2-B6, basse : C1-B4
    const int start = modeIdx == 0 ? 36 : 24;
    const int total = nOct * 12;
    static const bool isBlack[12] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

    auto area = getLocalBounds().toFloat().reduced (4.0f);
    const int nWhite = nOct * 7;
    const float ww = area.getWidth() / (float) nWhite;
    const float bw = ww * 0.62f, bh = area.getHeight() * 0.62f;

    auto stateColour = [&] (int pitch, juce::Colour normal)
    {
        if (! anyHeld (proc, pitch)) return normal;
        return (scaleOn && ! inScale[pitch % 12]) ? ui::red : ui::green;
    };

    // touches blanches
    int wi = 0;
    for (int i = 0; i < total; ++i)
    {
        const int pitch = start + i, pc = pitch % 12;
        if (isBlack[pc]) continue;
        auto key = juce::Rectangle<float> (area.getX() + (float) wi * ww, area.getY(), ww, area.getHeight()).reduced (1.0f, 0.0f);
        g.setColour (stateColour (pitch, juce::Colour (0xfff2f3f5)));
        g.fillRoundedRectangle (key, 4.0f);

        const float cx = key.getCentreX(), cy = key.getBottom() - 46.0f;
        if (scaleOn && inScale[pc])
        {
            g.setColour (ui::dark);
            g.fillEllipse (cx - 6, cy - 6, 12, 12);
            if (pc == root)
            {
                g.setColour (ui::purple);
                g.drawEllipse (cx - 10, cy - 10, 20, 20, 2.5f);
            }
        }
        g.setColour (ui::dark.withAlpha (0.75f));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (pc == 0 ? noteWithOctave (pitch) : juce::String (theory::noteName (pc)),
                    key.withTrimmedBottom (6.0f), juce::Justification::centredBottom);
        ++wi;
    }

    // touches noires
    wi = 0;
    for (int i = 0; i < total; ++i)
    {
        const int pitch = start + i, pc = pitch % 12;
        if (! isBlack[pc]) { ++wi; continue; }
        auto key = juce::Rectangle<float> (area.getX() + (float) wi * ww - bw * 0.5f, area.getY(), bw, bh);
        g.setColour (stateColour (pitch, juce::Colour (0xff0e1013)));
        g.fillRoundedRectangle (key, 3.0f);

        if (scaleOn && inScale[pc])
        {
            const float cx = key.getCentreX(), cy = key.getBottom() - 20.0f;
            g.setColour (juce::Colours::white);
            g.fillEllipse (cx - 6, cy - 6, 12, 12);
            if (pc == root)
            {
                g.setColour (ui::purple);
                g.drawEllipse (cx - 10, cy - 10, 20, 20, 2.5f);
            }
        }
    }
}

//==============================================================================
FretScopeAudioProcessorEditor::FretScopeAudioProcessorEditor (FretScopeAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), fret (p), keys (p)
{
    laf.setColourScheme (juce::LookAndFeel_V4::getMidnightColourScheme());
    setLookAndFeel (&laf);

    auto setupLabel = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, ui::muted);
        l.setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (l);
    };
    setupLabel (modeLabel, "Instrument");
    setupLabel (rootLabel, "Root");
    setupLabel (scaleLabel, "Scale");
    setupLabel (trainingLabel, "Training Mode");
    trainingLabel.setJustificationType (juce::Justification::centredRight);
    trainingLabel.setFont (juce::FontOptions (14.0f));

    for (auto& i : theory::instruments()) modeBox.addItem (i.name, modeBox.getNumItems() + 1);
    for (int i = 0; i < 12; ++i)          rootBox.addItem (theory::noteName (i), i + 1);
    for (auto& s : theory::scales())      scaleBox.addItem (s.name, scaleBox.getNumItems() + 1);
    trainingBox.addItem ("Free", 1);
    trainingBox.addItem ("Find the note", 2);

    for (auto* b : { &setupButton, &namesButton, &numbersButton, &fretButton, &keysButton })
    {
        styleButton (*b);
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    setupButton.setToggleState (true, juce::dontSendNotification);
    namesButton.setRadioGroupId (1);
    numbersButton.setRadioGroupId (1);
    fretButton.setRadioGroupId (2);
    keysButton.setRadioGroupId (2);
    namesButton.setConnectedEdges (juce::Button::ConnectedOnRight);
    numbersButton.setConnectedEdges (juce::Button::ConnectedOnLeft);
    fretButton.setConnectedEdges (juce::Button::ConnectedOnRight);
    keysButton.setConnectedEdges (juce::Button::ConnectedOnLeft);

    setupButton.onClick   = [this] { panelVisible = setupButton.getToggleState(); resized(); };
    namesButton.onClick   = [this] { setChoice ("labels", 1); };
    numbersButton.onClick = [this] { setChoice ("labels", 0); };
    fretButton.onClick    = [this] { setChoice ("view", 0); };
    keysButton.onClick    = [this] { setChoice ("view", 1); };

    for (auto* c : { &modeBox, &rootBox, &scaleBox, &trainingBox }) addAndMakeVisible (c);
    addAndMakeVisible (hexButton);
    addAndMakeVisible (info);
    addAndMakeVisible (fret);
    addAndMakeVisible (keys);
    trainingBox.toFront (false);
    trainingLabel.toFront (false);

    modeAtt     = std::make_unique<CBAttach> (proc.apvts, "mode", modeBox);
    rootAtt     = std::make_unique<CBAttach> (proc.apvts, "root", rootBox);
    scaleAtt    = std::make_unique<CBAttach> (proc.apvts, "scale", scaleBox);
    trainingAtt = std::make_unique<CBAttach> (proc.apvts, "training", trainingBox);
    hexAtt      = std::make_unique<BAttach>  (proc.apvts, "hex", hexButton);

    setResizable (true, true);
    setResizeLimits (860, 560, 1900, 1000);
    setSize (1060, 740);
    startTimerHz (30);
}

FretScopeAudioProcessorEditor::~FretScopeAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void FretScopeAudioProcessorEditor::setChoice (const char* id, int index)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
        p->endChangeGesture();
    }
}

void FretScopeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);

    // titre
    g.setColour (ui::purple.brighter (0.35f));
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText ("FretScope", 20, 12, 220, 40, juce::Justification::centredLeft);

    // carte du bas
    auto bc = bottomCard.toFloat();
    g.setColour (ui::card);
    g.fillRoundedRectangle (bc, 14.0f);
    g.setColour (ui::cardLine);
    g.drawRoundedRectangle (bc.reduced (1.0f), 14.0f, 1.5f);

    // pastille MIDI
    auto pill = midiPillBounds.toFloat();
    g.setColour (ui::cellOff);
    g.fillRoundedRectangle (pill, pill.getHeight() / 2);
    g.setColour (ui::cardLine);
    g.drawRoundedRectangle (pill, pill.getHeight() / 2, 1.2f);
    g.setColour (midiActive ? ui::green : ui::muted.withAlpha (0.5f));
    g.fillEllipse (pill.getX() + 12, pill.getCentreY() - 5, 10, 10);
    g.setColour (midiActive ? ui::green : ui::muted);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText ("MIDI", pill.withTrimmedLeft (30.0f), juce::Justification::centredLeft);
}

void FretScopeAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (18, 12);

    auto header = r.removeFromTop (48);
    numbersButton.setBounds (header.removeFromRight (180).reduced (0, 6));
    namesButton.setBounds (header.removeFromRight (170).reduced (0, 6));
    header.removeFromRight (14);
    setupButton.setBounds (header.removeFromRight (90).reduced (0, 6));
    r.removeFromTop (8);

    for (auto* c : std::initializer_list<juce::Component*> { &modeLabel, &rootLabel, &scaleLabel, &modeBox, &rootBox, &scaleBox, &hexButton })
        c->setVisible (panelVisible);

    if (panelVisible)
    {
        auto top = r.removeFromTop (46);
        auto cell = [&] (juce::Label& l, juce::Component& c, int w)
        {
            auto a = top.removeFromLeft (w);
            l.setBounds (a.removeFromTop (16));
            c.setBounds (a.reduced (0, 2));
            top.removeFromLeft (12);
        };
        cell (modeLabel, modeBox, 120);
        cell (rootLabel, rootBox, 80);
        cell (scaleLabel, scaleBox, 190);
        hexButton.setBounds (top.removeFromLeft (120).withTrimmedTop (18));
        r.removeFromTop (10);
    }

    auto infoArea = r.removeFromTop (juce::jmin (300, juce::jmax (260, r.getHeight() / 2)));
    info.setBounds (infoArea);
    auto tr = infoArea.reduced (20, 18).removeFromRight (320).removeFromTop (40);
    trainingLabel.setBounds (tr.removeFromLeft (120));
    trainingBox.setBounds (tr.reduced (4, 2));

    r.removeFromTop (12);
    bottomCard = r;
    auto inner = r.reduced (14, 12);
    auto row = inner.removeFromBottom (40);
    inner.removeFromBottom (8);
    fret.setBounds (inner);
    keys.setBounds (inner);

    fretButton.setBounds (row.removeFromLeft (110).reduced (0, 2));
    keysButton.setBounds (row.removeFromLeft (110).reduced (0, 2));
    midiPillBounds = juce::Rectangle<int> (0, 0, 96, 32).withCentre ({ inner.getCentreX(), row.getCentreY() });
}

void FretScopeAudioProcessorEditor::pickTarget (int root, const std::vector<int>& steps)
{
    std::vector<int> pcs;
    if (steps.empty()) for (int i = 0; i < 12; ++i) pcs.push_back (i);
    else               for (int s : steps) pcs.push_back ((root + s) % 12);

    int next = targetPc;
    for (int tries = 0; tries < 20 && next == targetPc; ++tries)
        next = pcs[(size_t) rng.nextInt ((int) pcs.size())];
    targetPc = next;
}

void FretScopeAudioProcessorEditor::timerCallback()
{
    auto& apvts = proc.apvts;
    const int modeIdx  = choiceOf (apvts, "mode");
    const int root     = choiceOf (apvts, "root");
    const int scaleIdx = choiceOf (apvts, "scale");
    const int labels   = choiceOf (apvts, "labels");
    const int view     = choiceOf (apvts, "view");
    const int training = choiceOf (apvts, "training");
    const bool hex     = apvts.getRawParameterValue ("hex")->load() > 0.5f;
    const auto& scale  = theory::scales()[(size_t) scaleIdx];
    const auto& inst   = theory::instruments()[(size_t) modeIdx];

    // synchronise les boutons segmentés
    namesButton.setToggleState (labels == 1, juce::dontSendNotification);
    numbersButton.setToggleState (labels == 0, juce::dontSendNotification);
    fretButton.setToggleState (view == 0, juce::dontSendNotification);
    keysButton.setToggleState (view == 1, juce::dontSendNotification);
    fret.setVisible (view == 0);
    keys.setVisible (view == 1);

    // activité MIDI
    const auto act = proc.getActivity();
    const auto nowMs = juce::Time::getMillisecondCounter();
    if (act != lastActivity) { lastActivity = act; lastActivityMs = nowMs; }
    midiActive = (nowMs - lastActivityMs) < 800 && act != 0;

    // notes tenues
    uint16_t heldMask = 0;
    int heldAbsMask = 0;
    uint64_t sig = 1469598103934665603ull;
    auto mix = [&sig] (uint64_t v) { sig = (sig ^ v) * 1099511628211ull; };
    for (int n = 0; n < 128; ++n)
        for (int ch = 0; ch < 16; ++ch)
            if (proc.isNoteHeld (ch, n))
            {
                heldMask |= (uint16_t) (1 << (((n % 12) - root + 12) % 12));
                heldAbsMask |= 1 << (n % 12);
                mix ((uint64_t) (ch * 128 + n + 1));
            }

    // entraînement : "Find the note"
    const uint64_t paramSig = (uint64_t) (modeIdx + 1) * 1000003ull + (uint64_t) root * 10007ull
                            + (uint64_t) scaleIdx * 101ull + (uint64_t) training;
    if (paramSig != lastParamSig) { lastParamSig = paramSig; targetPc = -1; score = 0; flash = 0; }

    if (training == 1)
    {
        if (targetPc < 0) pickTarget (root, scale.steps);
        if (flash > 0) --flash;
        if (targetPc >= 0 && (heldAbsMask & (1 << targetPc)))
        {
            ++score;
            flash = 18;
            pickTarget (root, scale.steps);
        }
    }

    // état de la carte d'info
    InfoState s;
    s.root = root;
    s.scaleOn = ! scale.steps.empty();
    s.scaleName = scale.name;
    for (int st : scale.steps) s.scaleMask |= (uint16_t) (1 << st);
    s.heldMask = heldMask;
    s.namesMode = (labels == 1);
    const auto sid = theory::describeSet (s.scaleMask);
    s.id = sid.id; s.parent = sid.parent; s.rotation = sid.rotation;
    s.targetMode = (training == 1);
    s.targetPc = targetPc;
    s.score = score;
    s.matched = flash > 0;

    const int last = proc.getLastNote();
    if (last >= 0)
    {
        s.hasNote = true;
        s.note = last;
        s.held = anyHeld (proc, last);
        s.inScale = (s.scaleMask >> (((last % 12) - root + 12) % 12)) & 1;
        const int ch = proc.getLastChannel();
        if (hex && ch < (int) inst.tuning.size()) s.stringNo = ch + 1;
    }

    mix ((uint64_t) paramSig);
    mix ((uint64_t) (labels + 1) * 31 + (uint64_t) (view + 1) * 7 + (hex ? 5 : 0));
    mix ((uint64_t) (last + 2) * 13 + (uint64_t) (targetPc + 2) * 17 + (uint64_t) score * 19 + (uint64_t) flash * 23);
    mix (midiActive ? 99 : 98);

    if (sig != lastSignature)
    {
        lastSignature = sig;
        info.setState (s);
        fret.repaint();
        keys.repaint();
        repaint (midiPillBounds);
    }
}
