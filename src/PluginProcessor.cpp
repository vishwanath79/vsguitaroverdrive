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

    // Envelope follower time constant: about 2 ms attack/release for responsiveness.
    const float envCoeff = 1.0f - std::exp(-1.0f / (0.002f * static_cast<float>(currentSampleRate)));

    // Very low threshold and a steep transition for an extreme dynamic response.
    const float threshold = 0.0015f;
    const float knee = 0.15f;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float x = channelData[sample];

            // Update envelope detector from the input.
            const float inputAbs = std::abs(x);
            envelope += envCoeff * (inputAbs - envelope);

            // Compute dynamic intensity: 0 for soft playing, 1 for loud playing.
            float intensity = juce::jlimit(0.0f, 1.0f, (envelope - threshold) / knee);

            // Extreme dynamic mode:
            // When Dynamic is 1, soft notes are forced almost clean (factor 0),
            // loud notes add up to 2x the normal drive boost on top of the Drive knob.
            float dynamicFactor = intensity * 2.0f;

            // Blend between static drive and extreme dynamic drive.
            float effectiveDrive = drive + dynamic * dynamicFactor;
            effectiveDrive = juce::jlimit(0.0f, 1.0f, effectiveDrive);

            // Expose the activity and threshold marker for the UI meter.
            distortionActivity.store(effectiveDrive);
            float marker = juce::jlimit(0.0f, 1.0f, (threshold * 2.0f)); // arbitrary scale for visibility
            distortionThresholdMarker.store(marker);

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
