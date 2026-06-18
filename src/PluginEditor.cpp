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

    // Dynamic knob.
    dynamicSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynamicSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    dynamicSlider.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(dynamicSlider);

    dynamicLabel.setText("Dynamic", juce::dontSendNotification);
    dynamicLabel.attachToComponent(&dynamicSlider, false);
    dynamicLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dynamicLabel);

    dynamicAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *processorRef.getValueTreeState().getParameter("dynamic"), dynamicSlider);

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

    setSize(520, 310);

    // Repaint the activity meter at ~30 fps.
    startTimerHz(30);
}

GuitarOverdriveAudioProcessorEditor::~GuitarOverdriveAudioProcessorEditor() = default;

void GuitarOverdriveAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawFittedText("VS Guitar Overdrive", getLocalBounds().removeFromTop(30),
                     juce::Justification::centred, 1);

    // Big distortion activity bar at the bottom.
    auto meterArea = getLocalBounds().removeFromBottom(30).reduced(20, 5);

    // Background bar.
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(meterArea.toFloat(), 4.0f);

    // Filled portion based on current distortion activity.
    auto fillArea = meterArea.toFloat();
    fillArea.setWidth(meterArea.getWidth() * smoothedActivity);

    // Color shifts from dim amber at low activity to bright red at high activity.
    g.setColour(juce::Colour::fromHSV(0.08f - smoothedActivity * 0.08f, 0.9f,
                                       0.4f + smoothedActivity * 0.6f, 1.0f));
    g.fillRoundedRectangle(fillArea, 4.0f);

    // Border.
    g.setColour(juce::Colours::lightgrey);
    g.drawRoundedRectangle(meterArea.toFloat(), 4.0f, 2.0f);

    // Label centered over the bar.
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawFittedText("DRIVE ACTIVITY", meterArea.withTrimmedTop(2),
                     juce::Justification::centred, 1);
}

void GuitarOverdriveAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(30); // leave room for title

    auto knobWidth = area.getWidth() / 4;

    driveSlider.setBounds(area.removeFromLeft(knobWidth).reduced(10));
    dynamicSlider.setBounds(area.removeFromLeft(knobWidth).reduced(10));
    toneSlider.setBounds(area.removeFromLeft(knobWidth).reduced(10));
    levelSlider.setBounds(area.removeFromLeft(knobWidth).reduced(10));
}

void GuitarOverdriveAudioProcessorEditor::timerCallback()
{
    // Smooth the activity value for a calmer meter movement.
    const float target = processorRef.distortionActivity.load();
    smoothedActivity += 0.15f * (target - smoothedActivity);

    repaint();
}
