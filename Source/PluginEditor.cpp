/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleTunerAudioProcessorEditor::SimpleTunerAudioProcessorEditor (SimpleTunerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), displayFreqComponent(audioProcessor)
{
    addAndMakeVisible(displayFreqComponent);
    setSize (400, 300);
}

SimpleTunerAudioProcessorEditor::~SimpleTunerAudioProcessorEditor()
{
}

//==============================================================================
void SimpleTunerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void SimpleTunerAudioProcessorEditor::resized()
{
    displayFreqComponent.setBounds(getLocalBounds());
}
