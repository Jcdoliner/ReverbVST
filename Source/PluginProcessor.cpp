/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "math.h"

//==============================================================================
DelaySEGAudioProcessor::DelaySEGAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    state = new juce::AudioProcessorValueTreeState(*this, nullptr);
    state->createAndAddParameter(statenames[0], paramNames[0], paramNames[0], juce::NormalisableRange<float>(0.0f, 0.9f, 0.01f), 0.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[1], paramNames[1], paramNames[1], juce::NormalisableRange<float>(1.f, 5.f, 1.f), 1.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[2], paramNames[2], paramNames[2], juce::NormalisableRange<float>(1.0f, 10.0f, 1.0f), 0.f, nullptr, nullptr);



    state->state = juce::ValueTree(statenames[0]);
    state->state = juce::ValueTree(statenames[1]);
    state->state = juce::ValueTree(statenames[2]);

}

DelaySEGAudioProcessor::~DelaySEGAudioProcessor()
{
}

//==============================================================================
const juce::String DelaySEGAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelaySEGAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelaySEGAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelaySEGAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelaySEGAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelaySEGAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelaySEGAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelaySEGAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelaySEGAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelaySEGAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DelaySEGAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    samplerate = sampleRate;

    sizeOfDelayBuffer=sampleRate*2.0;
    delayBuffer.setSize(getTotalNumOutputChannels(),(int)sizeOfDelayBuffer);
}

void DelaySEGAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelaySEGAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void DelaySEGAudioProcessor::writeDelayToOutputBuffer(juce::AudioBuffer<float>& buffer,int channel,int n,int delayBufferSize,float tail,float gain){
    auto readPosition=writePosition-tail;
    if (readPosition<0)
        readPosition+=delayBufferSize;
    if (readPosition+n<delayBufferSize)
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, (int) readPosition), n, gain, gain);
    else
    {
    auto samplesToFill=delayBufferSize-readPosition;
    buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, (int) readPosition), (int)samplesToFill, gain, gain);
    auto bufferRemaining=n-samplesToFill;
    buffer.addFromWithRamp(channel, (int)samplesToFill, delayBuffer.getReadPointer(channel, 0), (int)bufferRemaining, gain, gain);

    }

}

void DelaySEGAudioProcessor::circularBuffer(int channel,int n,int delayBufferSize,float *channelData){
    //Circular Buffer Found From JUCE 15 Audio Programmer Tutorial
    if (delayBufferSize > n + writePosition){
        delayBuffer.copyFromWithRamp(channel, writePosition,channelData,n,1.0f,1.0f);
    }
    else
    {
        auto samplesToFillBuffer=delayBufferSize-writePosition;
        delayBuffer.copyFromWithRamp(channel,writePosition,channelData,samplesToFillBuffer,1.0f,1.0f);
        auto samplesRemaining=n-samplesToFillBuffer;
        delayBuffer.copyFromWithRamp(channel,0,channelData+samplesToFillBuffer,samplesRemaining,1.0f,1.0f);
    }
}


void DelaySEGAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    float maxVal;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
         maxVal=buffer.getMagnitude(i,0,buffer.getNumSamples());
    }
    auto n=buffer.getNumSamples();
    float mix = *state->getRawParameterValue(statenames[0]);
    float taps = *state->getRawParameterValue(statenames[1]);
    float time = *state->getRawParameterValue(statenames[2]);

    int delayBufferSize=delayBuffer.getNumSamples();




    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);
        circularBuffer(channel,n,delayBufferSize,channelData);
        for (int i = 0; i < taps; i++) {
            float tapTime = (samplerate /((int)(11-time)+((i)/2)));
            writeDelayToOutputBuffer(buffer, channel, n, delayBufferSize, tapTime, mix);
        }
//        auto readPosition=writePosition-(samplerate/(int)tail);
//        if (readPosition<0)
//            readPosition+=delayBufferSize;
//        if (readPosition+n<delayBufferSize) {
//            buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, (int) readPosition), n, 0.6f, 0.6f);
//        }
//        else
//        {
//            auto samplesToFill=delayBufferSize-readPosition;
//            buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, (int) readPosition), (int)samplesToFill, 0.6f, 0.6f);
//            auto bufferRemaining=n-samplesToFill;
//            buffer.addFromWithRamp(channel, (int)samplesToFill, delayBuffer.getReadPointer(channel, 0), (int)bufferRemaining, 0.6f, 0.6f);
//
//        }

        //channelData++;
    }
    writePosition+=n;
    writePosition%=delayBufferSize;
}
juce::AudioProcessorValueTreeState& DelaySEGAudioProcessor::getState() {
    return *state;
}
//==============================================================================
bool DelaySEGAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelaySEGAudioProcessor::createEditor()
{
    return new DelaySEGAudioProcessorEditor (*this);
}

//==============================================================================
void DelaySEGAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{

}

void DelaySEGAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ValueTree tree = juce::ValueTree::readFromData(data,sizeInBytes);
    if (tree.isValid()) {
        state->state = tree;

    }

}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelaySEGAudioProcessor();
}
