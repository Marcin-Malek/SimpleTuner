/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class SimpleTunerAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleTunerAudioProcessor();
    ~SimpleTunerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    float getFundamental();
    float getSoundLevel();

private:
    //==============================================================================
    void pushNextSampleIntoFifo(float sample);
    void findFundamental();

    static constexpr auto fftOrder = 15;
    static constexpr auto fftSize = 1 << fftOrder;
    static constexpr float noiseThreshold = -60;

    float soundLevel = 0;
    juce::dsp::FFT forwardFFT;
    float fftData[2 * fftSize];
    float fundamentalFrequency = 0;
    float fifo[fftSize];
    int fifoIndex = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleTunerAudioProcessor);
};
