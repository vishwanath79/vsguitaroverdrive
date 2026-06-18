#include "PluginEditor.h"

GuitarOverdriveAudioProcessorEditor::GuitarOverdriveAudioProcessorEditor(GuitarOverdriveAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Drive knob.
    driveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    driveSlider.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(driveSlider);

    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.attachToComponent(&driveSlider, false);
    driveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driveLabel);

    driveAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *processorRef.getValueTreeState().getParameter("drive"), driveSlider);

    // Tone knob.
    toneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    toneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    toneSlider.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(toneSlider);

    toneLabel.setText("Tone", juce::dontSendNotification);
    toneLabel.attachToComponent(&toneSlider, false);
    toneLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(toneLabel);

    toneAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *processorRef.getValueTreeState().getParameter("tone"), toneSlider);

    // Level knob.
    levelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    levelSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    levelSlider.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(levelSlider);

    levelLabel.setText("Level", juce::dontSendNotification);
    levelLabel.attachToComponent(&levelSlider, false);
    levelLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(levelLabel);

    levelAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *processorRef.getValueTreeState().getParameter("level"), levelSlider);

    setSize(400, 250);
}

GuitarOverdriveAudioProcessorEditor::~GuitarOverdriveAudioProcessorEditor() = default;

void GuitarOverdriveAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawFittedText("VS Guitar Overdrive", getLocalBounds().removeFromTop(30),
                     juce::Justification::centred, 1);
}

void GuitarOverdriveAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(30); // leave room for title

    auto knobWidth = area.getWidth() / 3;

    driveSlider.setBounds(area.removeFromLeft(knobWidth).reduced(10));
    toneSlider.setBounds(area.removeFromLeft(knobWidth).reduced(10));
    levelSlider.setBounds(area.removeFromLeft(knobWidth).reduced(10));
}
