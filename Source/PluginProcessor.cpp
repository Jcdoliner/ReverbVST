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
    state->createAndAddParameter(statenames[0], paramNames[0], paramNames[0], juce::NormalisableRange<float>(0.5f, 3.f, 0.01f), 0.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[1], paramNames[1], paramNames[1], juce::NormalisableRange<float>(1.f, 150.f, 0.01f), 0.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[2], paramNames[2], paramNames[2], juce::NormalisableRange<float>(0.01f, 1.f, 0.01f), 0.f, nullptr, nullptr);



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
//    samplerate = sampleRate;

    sizeOfDelayBuffer=sampleRate*2.0;
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

void ReverbSEGAudioProcessor::circularBuffer(int channel,int n,int delayBufferSize,float *channelData){
    //Circular Buffer Found From JUCE 15 Audio Programmer Tutorial
    if (delayBufferSize > n + writePosition){
        delayBuffer.copyFromWithRamp(channel, writePosition,channelData,n,1.0f,0.01f);
    }
    else
    {
        auto samplesToFillBuffer=delayBufferSize-writePosition;
        delayBuffer.copyFromWithRamp(channel,writePosition,channelData,samplesToFillBuffer,1.0f,0.01f);
        auto samplesRemaining=n-samplesToFillBuffer;
        delayBuffer.copyFromWithRamp(channel,0,channelData+samplesToFillBuffer,samplesRemaining,1.0f,0.01f);
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
    auto n=buffer.getNumSamples();
    float length = *state->getRawParameterValue(statenames[0]);
    float size = *state->getRawParameterValue(statenames[1]);
    float tail = *state->getRawParameterValue(statenames[2]);
    int delayBufferSize=delayBuffer.getNumSamples();
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);
        circularBuffer(channel,n,delayBufferSize,channelData);

            //        float cleansig=*channelData;
//        FeedBackLoop.process()

        //delayBuffer[delayCounter]=channelData;
        //*channelData+=cleansig+(tail*(&delayBuffer[(delayCounter-1)]));
        //std::cout<<"\nNewSample\n"<<delayBuffer[delayCounter-1]<<"\n"<<cleansig;
        channelData++;
    }
    writePosition+=n;
    writePosition%=delayBufferSize;
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
