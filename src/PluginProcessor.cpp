#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::String paramDrive   = "drive";
    juce::String paramTone    = "tone";
    juce::String paramLevel   = "level";
    juce::String paramDynamic = "dynamic";
}

GuitarOverdriveAudioProcessor::GuitarOverdriveAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("GuitarOverdriveParams"),
                 {
                     std::make_unique<juce::AudioParameterFloat>(
                         paramDrive,
                         "Drive",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                         0.2f),
                     std::make_unique<juce::AudioParameterFloat>(
                         paramDynamic,
                         "Dynamic",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                         0.0f),
                     std::make_unique<juce::AudioParameterFloat>(
                         paramTone,
                         "Tone",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                         0.5f),
                     std::make_unique<juce::AudioParameterFloat>(
                         paramLevel,
                         "Level",
                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                         0.7f)
                 })
{
    driveParam   = parameters.getRawParameterValue(paramDrive);
    dynamicParam = parameters.getRawParameterValue(paramDynamic);
    toneParam    = parameters.getRawParameterValue(paramTone);
    levelParam   = parameters.getRawParameterValue(paramLevel);
}

GuitarOverdriveAudioProcessor::~GuitarOverdriveAudioProcessor() {}

void GuitarOverdriveAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    toneFilter.reset();
    envelope = 0.0f;
}

void GuitarOverdriveAudioProcessor::releaseResources()
{
}

void GuitarOverdriveAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels.
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const float drive   = driveParam->load();
    const float dynamic = dynamicParam->load();
    const float tone    = toneParam->load();
    const float level   = levelParam->load();

    // Tone knob 0..1: low cutoff 200 Hz -> high cutoff 6000 Hz.
    const float minFreq = 200.0f;
    const float maxFreq = 6000.0f;
    const float cutoff = minFreq * std::pow(maxFreq / minFreq, tone);
    toneFilter.setCutoffFrequency(cutoff);

    // Map 0..1 level knob to dB range -24..+12 dB.
    const float outputGain = juce::Decibels::decibelsToGain(-24.0f + level * 36.0f);

    // Fast attack catches transients, slow release holds level across sustained notes.
    const float sr = static_cast<float>(currentSampleRate);
    const float attackCoeff  = 1.0f - std::exp(-1.0f / (0.005f * sr));
    const float releaseCoeff = 1.0f - std::exp(-1.0f / (0.150f * sr));

    // Tuned to real guitar levels: quiet playing stays below threshold,
    // loud playing saturates. The old 0.0015/0.15 pair pegged at 1.0 constantly.
    const float threshold = 0.02f;
    const float knee = 0.25f;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float x = channelData[sample];

            // Envelope detector: attack on rising input, release on falling.
            const float inputAbs = std::abs(x);
            const float envCoeff = (inputAbs > envelope) ? attackCoeff : releaseCoeff;
            envelope += envCoeff * (inputAbs - envelope);

            // 0 = quiet, 1 = loud. Linear across the knee for musical response.
            float intensity = juce::jlimit(0.0f, 1.0f, (envelope - threshold) / knee);

            // Center on 0.5 so medium playing leaves Drive alone. Loud adds grit,
            // soft trims it. Range goes past 1.0 so loud notes exceed the knob.
            float dynamicFactor = (intensity - 0.5f) * 2.0f;
            float effectiveDrive = juce::jlimit(0.0f, 1.5f, drive + dynamic * dynamicFactor * 0.8f);

            // Expose activity for the UI meter.
            distortionActivity.store(juce::jlimit(0.0f, 1.0f, effectiveDrive));

            // Map effective drive to input gain.
            const float inputGain = 1.0f + effectiveDrive * 39.0f;

            // Push the signal harder as drive increases.
            x *= inputGain;

            // Soft clipping with hyperbolic tangent for smooth overdrive.
            x = std::tanh(x);

            // Tone shaping filter.
            x = toneFilter.processSample(channel, x);

            // Output level.
            x *= outputGain;

            channelData[sample] = x;
        }
    }
}

juce::AudioProcessorEditor* GuitarOverdriveAudioProcessor::createEditor()
{
    return new GuitarOverdriveAudioProcessorEditor(*this);
}

bool GuitarOverdriveAudioProcessor::hasEditor() const { return true; }

const juce::String GuitarOverdriveAudioProcessor::getName() const { return JucePlugin_Name; }

bool GuitarOverdriveAudioProcessor::acceptsMidi() const { return false; }
bool GuitarOverdriveAudioProcessor::producesMidi() const { return false; }
bool GuitarOverdriveAudioProcessor::isMidiEffect() const { return false; }
double GuitarOverdriveAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int GuitarOverdriveAudioProcessor::getNumPrograms() { return 1; }
int GuitarOverdriveAudioProcessor::getCurrentProgram() { return 0; }
void GuitarOverdriveAudioProcessor::setCurrentProgram(int index) { }
const juce::String GuitarOverdriveAudioProcessor::getProgramName(int index) { return {}; }
void GuitarOverdriveAudioProcessor::changeProgramName(int index, const juce::String& newName) { }

void GuitarOverdriveAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GuitarOverdriveAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState& GuitarOverdriveAudioProcessor::getValueTreeState()
{
    return parameters;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarOverdriveAudioProcessor();
}
