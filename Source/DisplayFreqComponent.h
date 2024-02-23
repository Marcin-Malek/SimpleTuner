/*
  ==============================================================================

    DisplayFreqComponent.h
    Created: 21 Feb 2024 7:58:40pm
    Author:  1

  ==============================================================================
*/

#pragma once


#include "PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
class DisplayFreqComponent : public juce::Component, public juce::Timer
{
public:
	DisplayFreqComponent(SimpleTunerAudioProcessor&);
	~DisplayFreqComponent() override;

	void timerCallback() override;

	void paint(juce::Graphics& g) override;
	void resized() override;

private:
	SimpleTunerAudioProcessor& audioProcessor;
};