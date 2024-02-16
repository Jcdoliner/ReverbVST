/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once
#define sliderN 3
#define msDelayResponse 40
#define reverbChannels 4
#include <JuceHeader.h>

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
    void writeDelayToOutputBuffer(juce::AudioBuffer<float>& buffer,juce::AudioBuffer<float>& dbuffer,int channel,int bufferSize,int delayBufferSize,float tail,float gain);

    void addDelayToBuffer(juce::AudioBuffer<float>& source,juce::AudioBuffer<float>& dest,int channel);



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

private:
      double samplerate;

    double sizeOfDelayBuffer;

    using virtualBuffer=juce::AudioBuffer<float>;

    virtualBuffer delayBuffer;





    int writePosition {0};
    juce::ScopedPointer<juce::AudioProcessorValueTreeState> state;
    const char* paramNames[sliderN] = { "Length","Size","Tail" };
    const char* statenames[sliderN] = { "length","size","tail" };





//    using Delay= juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;
//
//
//    struct FeedBackLoop{
//        float delayMs=80;
//        float decayGain=0.85;
//        int delaySamples;
//        Delay delay;
//
//
//        void configure (double sampleRate){
//            delaySamples=delayMs*0.001*sampleRate;
//            delay.setMaximumDelayInSamples(delaySamples);
//            delay.reset();
//        }
//        float process (float inputSample){
//            float delayed=delay.getDelay();
//            float out=inputSample+delayed*decayGain;
//            return delayed;
//

        //}
    //};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbSEGAudioProcessor)
};
