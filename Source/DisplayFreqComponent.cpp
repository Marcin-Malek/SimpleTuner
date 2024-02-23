/*
  ==============================================================================

    DisplayFreqComponent.cpp
    Created: 21 Feb 2024 7:58:18pm
    Author:  1

  ==============================================================================
*/

#include "DisplayFreqComponent.h"


DisplayFreqComponent::DisplayFreqComponent(SimpleTunerAudioProcessor& p) : audioProcessor(p)
{
    startTimerHz(100);
    setSize(200, 200);
}

DisplayFreqComponent::~DisplayFreqComponent()
{
}

void DisplayFreqComponent::timerCallback() 
{
    repaint();
}

void DisplayFreqComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText(juce::String(audioProcessor.getFundamental()), getLocalBounds(), juce::Justification::centred, 1);
}

void DisplayFreqComponent::resized()
{
}
