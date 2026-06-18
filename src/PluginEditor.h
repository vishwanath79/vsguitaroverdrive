#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GuitarOverdriveAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit GuitarOverdriveAudioProcessorEditor(GuitarOverdriveAudioProcessor&);
    ~GuitarOverdriveAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    GuitarOverdriveAudioProcessor& processorRef;

    juce::Slider driveSlider;
    juce::Slider dynamicSlider;
    juce::Slider toneSlider;
    juce::Slider levelSlider;

    juce::Label driveLabel;
    juce::Label dynamicLabel;
    juce::Label toneLabel;
    juce::Label levelLabel;

    std::unique_ptr<juce::SliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> dynamicAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> toneAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> levelAttachment;

    float smoothedActivity = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarOverdriveAudioProcessorEditor)
};
