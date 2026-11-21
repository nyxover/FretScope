#pragma once
#include "PluginProcessor.h"
#include <cmath>

namespace ui
{
    inline const juce::Colour bg       { 0xff0b0d10 };
    inline const juce::Colour card     { 0xff101319 };
    inline const juce::Colour cardLine { 0xff2a3340 };
    inline const juce::Colour boardBg  { 0xff131820 };
    inline const juce::Colour line     { 0xff2a3340 };
    inline const juce::Colour green    { 0xff58d36f };
    inline const juce::Colour purple   { 0xff7c5cdb };
    inline const juce::Colour red      { 0xffff5c6c };
    inline const juce::Colour muted    { 0xff8a94a3 };
    inline const juce::Colour dotLight { 0xffcfd2d9 };
    inline const juce::Colour dark     { 0xff15181d };
    inline const juce::Colour cellOff  { 0xff1b1f26 };
}

inline int choiceOf (juce::AudioProcessorValueTreeState& s, const char* id)
{
    return (int) std::lround (s.getRawParameterValue (id)->load());
}

//==============================================================================
struct InfoState
{
    bool hasNote = false, held = false;
    int note = 0, stringNo = 0;           // stringNo 0 = inconnue
    bool scaleOn = false, inScale = false;
    int root = 0;
    uint16_t scaleMask = 0, heldMask = 0; // bit i = degré i (relatif à la fondamentale)
    int id = 0, parent = 0, rotation = 0;
    bool namesMode = false;
    bool targetMode = false, matched = false;
    int targetPc = -1, score = 0;
    juce::String scaleName;
};

class InfoView : public juce::Component
{
public:
    void setState (const InfoState& s) { st = s; repaint(); }
    void paint (juce::Graphics&) override;
private:
    InfoState st;
};

//==============================================================================
class FretboardView : public juce::Component
{
public:
    explicit FretboardView (FretScopeAudioProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    FretScopeAudioProcessor& proc;
};

class KeyboardView : public juce::Component
{
public:
    explicit KeyboardView (FretScopeAudioProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    FretScopeAudioProcessor& proc;
};

//==============================================================================
class FretScopeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit FretScopeAudioProcessorEditor (FretScopeAudioProcessor&);
    ~FretScopeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setChoice (const char* id, int index);
    void pickTarget (int root, const std::vector<int>& steps);

    FretScopeAudioProcessor& proc;
    juce::LookAndFeel_V4 laf;

    juce::TextButton setupButton { "Setup" }, namesButton { "Notes and Names" }, numbersButton { "Numbers and Shapes" },
                     fretButton { "Fretboard" }, keysButton { "Keyboard" };
    juce::ComboBox modeBox, rootBox, scaleBox, trainingBox;
    juce::ToggleButton hexButton { "Hex MIDI" };
    juce::Label modeLabel, rootLabel, scaleLabel, trainingLabel;

    InfoView info;
    FretboardView fret;
    KeyboardView keys;

    using CBAttach = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BAttach  = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<CBAttach> modeAtt, rootAtt, scaleAtt, trainingAtt;
    std::unique_ptr<BAttach> hexAtt;

    juce::Rectangle<int> bottomCard, midiPillBounds;
    bool panelVisible = true;

    // état de la boucle d'entraînement
    juce::Random rng;
    int targetPc = -1, score = 0, flash = 0;
    uint64_t lastParamSig = 0, lastSignature = 0;
    uint32_t lastActivity = 0, lastActivityMs = 0;
    bool midiActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FretScopeAudioProcessorEditor)
};
