/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleTunerAudioProcessor::SimpleTunerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
                        fft(fftOrder),
                        fifo{},
                        fftData{},
                        fundamentalFrequency(0),
                        fifoIndex(0)
#endif
{
}

SimpleTunerAudioProcessor::~SimpleTunerAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleTunerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleTunerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleTunerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleTunerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleTunerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleTunerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleTunerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleTunerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleTunerAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleTunerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleTunerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SimpleTunerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleTunerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleTunerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
		buffer.clear(i, 0, buffer.getNumSamples());
	}

    // dBFS
    soundLevel = juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));

    if (soundLevel > noiseThreshold) {
        for (auto i = 0; i < buffer.getNumSamples(); ++i) {
            // Since it is a tuner plugin only mono signal is needed
            pushNextSampleIntoFifo(buffer.getReadPointer(0)[i]);
        }
    }
}

float SimpleTunerAudioProcessor::getFundamental() {
    return fundamentalFrequency;
}

float SimpleTunerAudioProcessor::getSoundLevel() {
    return soundLevel;
}

int SimpleTunerAudioProcessor::getFifoIndex() {
    return fifoIndex;
}

float SimpleTunerAudioProcessor::getMaxThreshold() {
    return maxThreshold;
}

int SimpleTunerAudioProcessor::getPitchPeriod() {
    return pitchPeriod;
}

int SimpleTunerAudioProcessor::getLocalMaxIndex() {
    return tempMaxIdx;
}

//int SimpleTunerAudioProcessor::getFirstNegIndex() {
//    return firstNegativeNsdfIndex;
//}

void SimpleTunerAudioProcessor::pushNextSampleIntoFifo(float sample) {
    fifo[fifoIndex++] = sample;

    if (fifoIndex == windowSize)
    {
        juce::zeromem(fftData, sizeof(fftData));
        memcpy(fftData, fifo, sizeof(fifo));
        findFundamental();
        fifoIndex = windowSize - step;
    }
}

void SimpleTunerAudioProcessor::peakPicking()
{
    int pos = 1;

    while (pos < (windowSize - 1) && nsdf[pos - 1] <= 0.0 && nsdf[pos] > 0.0) {
        pos++;
    }

    while (pos < windowSize - 1)
    {
        if (nsdf[pos] > nsdf[pos - 1] && nsdf[pos] >= nsdf[pos + 1] &&
            (tempMaxIdx == 0 || nsdf[pos] > nsdf[tempMaxIdx]))
        {
            tempMaxIdx = pos;
        }
        pos++;
        if (pos < windowSize - 1 && nsdf[pos] <= 0)
        {
            if (tempMaxIdx > 0)
            {
                keyMaxima.push_back(tempMaxIdx);
                tempMaxIdx = 0;
            }
            while (pos < windowSize - 1 && nsdf[pos] <= 0.0)
            {
                pos++;
            }
        }
    }
    if (tempMaxIdx > 0)
    {
        keyMaxima.push_back(tempMaxIdx);
    }
}

void SimpleTunerAudioProcessor::calculateAutocorrelation() 
{
    fft.performRealOnlyForwardTransform(fftData, true);

    // [R,I,R,I,...] - fftData
    for (auto i = 1; i < (fftSize / 2) + 1; i = i + 2) {
        //fftData[i - 1] = (fftData[i - 1] * fftData[i - 1]) - (fftData[i] * -fftData[i]); // real
        //fftData[i] = (fftData[i - 1] * -fftData[i]) + (fftData[i - 1] * fftData[i]); // imaginary

        fftData[i - 1] = ((fftData[i - 1] * fftData[i - 1]) - (fftData[i] * -fftData[i])) * (1/(2*fftSize)); // real
        fftData[i] = ((fftData[i - 1] * -fftData[i]) + (fftData[i - 1] * fftData[i])) * (1 / (2 * fftSize)); // imaginary
    }

    fft.performRealOnlyInverseTransform(fftData);
    memcpy(nsdf, fftData, fftSize * 2);
    //nsdf.assign(fftData, fftData + 512);
}

void SimpleTunerAudioProcessor::findFundamental()
{
    calculateAutocorrelation();
    peakPicking();
    std::vector<std::pair<float, float>> estimates;

    highestKeyMaximum = 0;

    for (int i : keyMaxima)
    {
        highestKeyMaximum = std::max(highestKeyMaximum, nsdf[i]);
        /*if (nsdf[i] > 0)
        {
            auto x = parabolicInterpolation(nsdf, i);
            estimates.push_back(x);
            highest_amplitude = std::max(highest_amplitude, std::get<1>(x));
        }*/
    }

    if (keyMaxima.empty())
        fundamentalFrequency = -1;

    maxThreshold = kParam * highestKeyMaximum;

    for (int i : keyMaxima)
    {
        if (nsdf[i] >= maxThreshold)
        {
            pitchPeriod = i;
            break;
        }
    }



    //juce::zeromem(acf, sizeof(acf));
    //juce::zeromem(sdf, sizeof(sdf));
    //juce::zeromem(nsdf, sizeof(nsdf));
    //juce::zeromem(keyMaxima, sizeof(keyMaxima));
    //firstNegativeNsdfIndex = -1;
    //tempMaxIdx = -1;

    /*for (auto i = 0; i < windowSize - 1; ++i)
    {
        for (auto j = 1; j < windowSize - i; ++j) {
            acf[i] += fifo[j] * fifo[i + j];
            sdf[i] += fifo[j] * fifo[j] + fifo[i + j] * fifo[i + j];
        }
        nsdf[i] = (2.0f * acf[i + 1]) / sdf[i + 1];
        firstNegativeNsdfIndex = nsdf[i];
        if (firstNegativeNsdfIndex == -1 && nsdf[i] < 0) {
            firstNegativeNsdfIndex = i;
        }
        else if (firstNegativeNsdfIndex != -1 && nsdf[i] > 0) {
            if (tempMaxIdx < firstNegativeNsdfIndex || nsdf[tempMaxIdx] < nsdf[i]) {
                tempMaxIdx = i;
            }
        }
        else if (firstNegativeNsdfIndex != -1 && nsdf[i] < 0 && tempMaxIdx > 0) {
            keyMaxima[tempMaxIdx] = nsdf[tempMaxIdx];
            tempMaxIdx = -1;
        }
    }*/

    /*highestKeyMaximum = std::max_element(std::begin(keyMaxima), std::end(keyMaxima));
    maxThreshold = *highestKeyMaximum * kParam;
    pitchPeriod = std::distance(
        std::begin(keyMaxima),
        std::find_if(std::begin(keyMaxima), std::end(keyMaxima), [this](float maximum) {
            return maximum > maxThreshold;
        })
    );*/

    fundamentalFrequency = getSampleRate() / pitchPeriod;
}

//==============================================================================
bool SimpleTunerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleTunerAudioProcessor::createEditor()
{
    return new SimpleTunerAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleTunerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleTunerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleTunerAudioProcessor();
}
