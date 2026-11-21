#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Theory.h"

juce::AudioProcessorValueTreeState::ParameterLayout FretScopeAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray instrumentNames, scaleNames;
    for (auto& i : theory::instruments()) instrumentNames.add (i.name);
    for (auto& s : theory::scales())      scaleNames.add (s.name);

    StringArray roots;
    for (int i = 0; i < 12; ++i) roots.add (theory::noteName (i));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 },   "Instrument", instrumentNames, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "root", 1 },   "Root",       roots, 4));  // E
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 },  "Scale",      scaleNames, 2)); // Natural Minor
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "labels", 1 }, "Labels",     StringArray { "Intervals", "Notes" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "view", 1 },     "View",          StringArray { "Fretboard", "Keyboard" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "training", 1 }, "Training Mode", StringArray { "Free", "Find the note" }, 0));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { "hex", 1 },    "Hex MIDI (1 channel / string)", false));
    return layout;
}

FretScopeAudioProcessor::FretScopeAudioProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
 #if ! JucePlugin_IsSynth
                      .withInput ("Input", juce::AudioChannelSet::stereo(), true)
 #endif
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                      ),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto& h : held) h.store (0);
}

bool FretScopeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
#endif
}

void FretScopeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int ch = juce::jlimit (0, 15, m.getChannel() - 1);

        activity.fetch_add (1, std::memory_order_relaxed);

        if (m.isNoteOn())
        {
            held[(size_t) (ch * 128 + m.getNoteNumber())].store (1);
            lastNote.store (m.getNoteNumber());
            lastChannel.store (ch);
        }
        else if (m.isNoteOff())
            held[(size_t) (ch * 128 + m.getNoteNumber())].store (0);
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            for (int n = 0; n < 128; ++n)
                held[(size_t) (ch * 128 + n)].store (0);
    }

#if ! JucePlugin_IsMidiEffect
    buffer.clear();      // le plugin ne produit pas de son
    midi.clear();
#else
    juce::ignoreUnused (buffer); // le MIDI traverse le plugin tel quel
#endif
}

juce::AudioProcessorEditor* FretScopeAudioProcessor::createEditor()
{
    return new FretScopeAudioProcessorEditor (*this);
}

void FretScopeAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void FretScopeAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FretScopeAudioProcessor();
}
