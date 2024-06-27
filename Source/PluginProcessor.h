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
    float getMaxThreshold();
    int getFifoIndex();
    int getPitchPeriod();

    int getFirstNegIndex();
    int getLocalMaxIndex();
    float highestKeyMaximum;
    float nsdf[1024];
    std::vector<int> keyMaxima;

private:
    //==============================================================================
    void pushNextSampleIntoFifo(float sample);
    void calculateAutocorrelation();
    void findFundamental();
    void peakPicking();

    static constexpr auto fftOrder = 10;
    static constexpr auto fftSize = 1 << fftOrder;
    static constexpr auto windowSize = 1 << 10;
    static constexpr auto step = windowSize / 4;
    static constexpr float noiseThreshold = -60;
    static constexpr float kParam = 0.8;

    float soundLevel = 0;
    juce::dsp::FFT fft;
    float fftData[2 * fftSize];
    float fundamentalFrequency = 0;
    float fifo[windowSize];
    int fifoIndex = 0;

    int tempMaxIdx = 0;
    
    //float acf[windowSize];
    //float sdf[windowSize];
    //float keyMaxima[windowSize];
    float maxThreshold = 0.0;
    //int firstNegativeNsdfIndex = 0;
    int pitchPeriod = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleTunerAudioProcessor);
};
