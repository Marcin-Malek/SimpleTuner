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
    int midiNoteNumber = lround(log(audioProcessor.getFundamental() / 440.0) / log(2.0) * 12 + 69);
    double exactNoteFreq = 440.0 * pow(2.0, (static_cast<double>(midiNoteNumber) - 69.0) / 12.0);
    float centsDeviation = log(audioProcessor.getFundamental() / exactNoteFreq) / log(2.0) * 1200;
    juce::String midiNoteName = getMidiNoteName(midiNoteNumber, true, true, 4);
    juce::String textIndication = "|";
    if (centsDeviation > 3 && centsDeviation < 50) {
        textIndication = textIndication.paddedRight('<', round(centsDeviation / 5));
    } else if (centsDeviation < -3 && centsDeviation > -50) {
        textIndication = textIndication.paddedLeft('>', abs(round(centsDeviation / 5)));
    }
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText(
        "midiNoteName: " + midiNoteName + "\n" +
        "soundLvl: " + juce::String(audioProcessor.getSoundLevel()) + "\n" +
        "frequency: " + juce::String(audioProcessor.getFundamental())
        , getLocalBounds(), juce::Justification::centred, 1);
    g.drawFittedText(textIndication, getLocalBounds(), juce::Justification::centredBottom, 1);
}

void DisplayFreqComponent::resized()
{
}
