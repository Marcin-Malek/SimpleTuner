/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DisplayFreqComponent.h"

//==============================================================================
class SimpleTunerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
	SimpleTunerAudioProcessorEditor(SimpleTunerAudioProcessor&);
	~SimpleTunerAudioProcessorEditor() override;

	void paint(juce::Graphics&) override;
	void resized() override;

private:
	SimpleTunerAudioProcessor& audioProcessor;
	DisplayFreqComponent displayFreqComponent;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleTunerAudioProcessorEditor)
};
