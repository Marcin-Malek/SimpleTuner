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
                        forwardFFT(fftOrder),
                        fifo{},
                        fftData{},
                        fundamentalFrequency(0),
                        nextFFTBlockReady(false),
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

    for (auto i = 0; i < buffer.getNumSamples(); ++i) {
        // Since it is a tuner plugin only mono signal is needed
        pushNextSampleIntoFifo(buffer.getReadPointer(0)[i]);
    }

    findFundamental();
}

float SimpleTunerAudioProcessor::getFundamental() {
    return fundamentalFrequency;
}

void SimpleTunerAudioProcessor::pushNextSampleIntoFifo(float sample) {
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            juce::zeromem(fftData, sizeof(fftData));
            memcpy(fftData, fifo, sizeof(fifo));
            nextFFTBlockReady = true;
        }

        fifoIndex = 0;
    }

    fifo[fifoIndex++] = sample;
}

void SimpleTunerAudioProcessor::findFundamental()
{
    if (nextFFTBlockReady)
    {
        forwardFFT.performFrequencyOnlyForwardTransform(fftData);

        float maxMagnitude = 0.0f;
        int peakFrequencyIndex = 0;

        for (int i = 0; i < fftSize * 2; ++i) {
            if (fftData[i] > maxMagnitude) {
                maxMagnitude = fftData[i];
                peakFrequencyIndex = i;
            }
        }
        fundamentalFrequency = peakFrequencyIndex * getSampleRate() / fftSize;
        nextFFTBlockReady = false;
    }
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
