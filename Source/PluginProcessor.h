#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

class FretScopeAudioProcessor : public juce::AudioProcessor
{
public:
    FretScopeAudioProcessor();
    ~FretScopeAudioProcessor() override = default;

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FretScope"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return JucePlugin_IsMidiEffect; }
    bool isMidiEffect() const override { return JucePlugin_IsMidiEffect; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Etat des notes tenues, lu par l'éditeur : canal (0..15) x note (0..127)
    bool isNoteHeld (int channel, int note) const noexcept
    {
        return held[(size_t) (channel * 128 + note)].load (std::memory_order_relaxed) != 0;
    }

    int getLastNote() const noexcept        { return lastNote.load (std::memory_order_relaxed); }
    int getLastChannel() const noexcept     { return lastChannel.load (std::memory_order_relaxed); }
    uint32_t getActivity() const noexcept   { return activity.load (std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::array<std::atomic<uint8_t>, 16 * 128> held;
    std::atomic<int> lastNote { -1 }, lastChannel { 0 };
    std::atomic<uint32_t> activity { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FretScopeAudioProcessor)
};
