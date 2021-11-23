/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace MyColours
{
	const juce::Colour main = juce::Colour::fromRGB(0, 128, 128);
	const juce::Colour black = juce::Colour::fromFloatRGBA(0.08f, 0.08f, 0.08f, 1.0f);
}

struct ResponseCurveComponent : juce::Component,
	juce::AudioProcessorParameter::Listener,
	juce::Timer
{
	ResponseCurveComponent(EasyFilterAudioProcessor&);
	~ResponseCurveComponent();

	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

	void timerCallback() override;

	void paint(juce::Graphics& g) override;

private:
	EasyFilterAudioProcessor& audioProcessor;
	juce::Atomic<bool> parametersChanged{ false };

	MonoChain monoChain;
};

//==============================================================================
/**
*/
class EasyFilterAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    EasyFilterAudioProcessorEditor (EasyFilterAudioProcessor&);
    ~EasyFilterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EasyFilterAudioProcessor& audioProcessor;

	juce::Slider lowCutFreqSlider,
	highCutFreqSlider,
	lowCutSlopeSlider,
	highCutSlopeSlider;

	ResponseCurveComponent responseCurveComponent;

	std::vector<juce::Component*> getComps();

	using APVTS = juce::AudioProcessorValueTreeState;
	using Attachment = APVTS::SliderAttachment;

	Attachment lowCutFreqSliderAttachment,
	highCutFreqSliderAttachment,
	lowCutSlopeSliderAttachment,
	highCutSlopeSliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyFilterAudioProcessorEditor)
};
