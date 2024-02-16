/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "math.h"

//==============================================================================
ReverbSEGAudioProcessor::ReverbSEGAudioProcessor()
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
    state->createAndAddParameter(statenames[1], paramNames[1], paramNames[1], juce::NormalisableRange<float>(1.f, 150.f, 0.01f), 0.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[2], paramNames[2], paramNames[2], juce::NormalisableRange<float>(1.0f, 2.0f, 0.1f), 0.f, nullptr, nullptr);



    state->state = juce::ValueTree(statenames[0]);
    state->state = juce::ValueTree(statenames[1]);
    state->state = juce::ValueTree(statenames[2]);

}

ReverbSEGAudioProcessor::~ReverbSEGAudioProcessor()
{
}

//==============================================================================
const juce::String ReverbSEGAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ReverbSEGAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ReverbSEGAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ReverbSEGAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ReverbSEGAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ReverbSEGAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ReverbSEGAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ReverbSEGAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ReverbSEGAudioProcessor::getProgramName (int index)
{
    return {};
}

void ReverbSEGAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ReverbSEGAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    samplerate = sampleRate;

    sizeOfDelayBuffer=sampleRate*5.0;
    delayBuffer.setSize(getTotalNumOutputChannels(),(int)sizeOfDelayBuffer);
}

void ReverbSEGAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ReverbSEGAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
void ReverbSEGAudioProcessor::circularBuffer(juce::AudioBuffer<float>& dbuffer,int channel,int bufferSize,int delayBufferSize,float *channelData){
    //Circular Buffer Found From JUCE 15 Audio Programmer Tutorial
    if (delayBufferSize > bufferSize + writePosition){
        dbuffer.copyFromWithRamp(channel, writePosition,channelData,bufferSize,1.0f,1.0f);
    }
    else
    {
        auto samplesToFillBuffer=delayBufferSize-writePosition;
        dbuffer.copyFromWithRamp(channel,writePosition,channelData,samplesToFillBuffer,1.0f,1.0f);
        auto samplesRemaining=bufferSize-samplesToFillBuffer;
        dbuffer.copyFromWithRamp(channel,0,channelData+samplesToFillBuffer,samplesRemaining,1.0f,1.0f);
    }
}

void ReverbSEGAudioProcessor::writeDelayToOutputBuffer(juce::AudioBuffer<float>& buffer,juce::AudioBuffer<float>& dBuffer,int channel,int bufferSize,int delayBufferSize,float tail,float gain){
    auto readPosition=writePosition-(samplerate/(int)tail);
    if (readPosition<0)
        readPosition+=delayBufferSize;
    if (readPosition+bufferSize<delayBufferSize)
        buffer.addFromWithRamp(channel, 0, dBuffer.getReadPointer(channel, (int) readPosition), bufferSize, gain, gain);
    else
    {
    auto samplesToFill=delayBufferSize-readPosition;
    buffer.addFromWithRamp(channel, 0, dBuffer.getReadPointer(channel, (int) readPosition), (int)samplesToFill, gain, gain);
    auto bufferRemaining=bufferSize-samplesToFill;
    buffer.addFromWithRamp(channel, (int)samplesToFill, dBuffer.getReadPointer(channel, 0), (int)bufferRemaining, gain, gain);

    }

}




void ReverbSEGAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    float maxVal;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
         maxVal=buffer.getMagnitude(i,0,buffer.getNumSamples());
    }

    float length = *state->getRawParameterValue(statenames[0]);
    float size = *state->getRawParameterValue(statenames[1]);
    float tail = *state->getRawParameterValue(statenames[2]);


    auto bufferSize=buffer.getNumSamples();
    int delayBufferSize=delayBuffer.getNumSamples();

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);
        circularBuffer(delayBuffer,channel,bufferSize,delayBufferSize,channelData);


        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,tail*2.f,length*0.01f);
        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,tail*2.5f,length*0.025f);
        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,tail*2.9f,length*0.05f);
        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,tail*3.f,length*0.1f);
        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,tail*4.f,length*0.2f);
        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,tail*6.f,length*0.3f);
        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,tail*8.f,length*0.4f);
        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,tail*16.f,length*0.5f);




    }
    writePosition+=bufferSize;
    writePosition%=delayBufferSize;
}
juce::AudioProcessorValueTreeState& ReverbSEGAudioProcessor::getState() {
    return *state;
}
//==============================================================================
bool ReverbSEGAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ReverbSEGAudioProcessor::createEditor()
{
    return new ReverbSEGAudioProcessorEditor (*this);
}

//==============================================================================
void ReverbSEGAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{

}

void ReverbSEGAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new ReverbSEGAudioProcessor();
}
