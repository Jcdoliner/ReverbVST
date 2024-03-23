/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#define sliderN 3
#define msDelayResponse 40
#define reverbChannels 8
#include <JuceHeader.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <vector>
//==============================================================================
/**
*/
class ReverbSEGAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    ReverbSEGAudioProcessor();
    ~ReverbSEGAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void circularBuffer(juce::AudioBuffer<float>& dbuffer,int channel,int bufferSize,int delayBufferSize,float *channelData);
    void writeDelayToOutputBuffer(juce::AudioBuffer<float>& buffer,juce::AudioBuffer<float>& dbuffer,int channel,int bufferSize,int delayBufferSize,float gain,float tail,float size);
    void generateMixMatrix(juce::AudioBuffer<float>& dBuffer,int bufferSize,int delayBufferSize,float delayBufferMag,int delayTimes[]);
    void ReverbSEGAudioProcessor::mixAudioBuffers(juce::AudioBuffer<float>& src,juce::AudioBuffer<float>& dst,int channel_src,int channel_dst,float gain);

    //void addDelayToBuffer(juce::AudioBuffer<float>& source,juce::AudioBuffer<float>& dest,int channel);

    //float[] CreateHadamardMatrix(int numReverbChannels);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    juce::AudioProcessorValueTreeState& getState();
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    float calcInvSqRoot(float n);

private:
      double samplerate;

    double sizeOfDelayBuffer;

    using virtualBuffer=juce::AudioBuffer<float>;
    using bufferMatrix=juce::dsp::Matrix<virtualBuffer>;

    virtualBuffer delayBuffer;
    //virtualBuffer delayLines[reverbChannels];
    //virtualBuffer outputMix[reverbChannels];





    float hadamardProduct[reverbChannels]={0,0,0,0,0,0,0,0};
    float hadamardMax=0;

    int writePosition {0};
    juce::ScopedPointer<juce::AudioProcessorValueTreeState> state;
    const char* paramNames[sliderN] = { "Length","Size","Tail" };
    const char* statenames[sliderN] = { "length","size","tail" };

    float reductionRatio;
    int hadamard [reverbChannels][reverbChannels];


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbSEGAudioProcessor)
};
