#pragma once
#include "PluginProcessor.h"

class FretboardView : public juce::Component
{
public:
    explicit FretboardView (FretScopeAudioProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;

private:
    FretScopeAudioProcessor& proc;
};

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
    juce::String buildReadout() const;

    FretScopeAudioProcessor& proc;
    juce::LookAndFeel_V4 laf;

    juce::ComboBox modeBox, rootBox, scaleBox, labelsBox;
    juce::ToggleButton hexButton { "Hex MIDI" };
    juce::Label modeLabel, rootLabel, scaleLabel, labelsLabel, readout;
    FretboardView board;

    using CBAttach = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BAttach  = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<CBAttach> modeAtt, rootAtt, scaleAtt, labelsAtt;
    std::unique_ptr<BAttach> hexAtt;

    uint64_t lastSignature = 0;
    juce::String lastReadout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FretScopeAudioProcessorEditor)
};
