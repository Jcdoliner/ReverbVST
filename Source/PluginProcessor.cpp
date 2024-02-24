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
    state->createAndAddParameter(statenames[0], paramNames[0], paramNames[0], juce::NormalisableRange<float>(0.01f, 0.9f, 0.01f), 0.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[1], paramNames[1], paramNames[1], juce::NormalisableRange<float>(1.f, 8.f, 1.f), 0.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[2], paramNames[2], paramNames[2], juce::NormalisableRange<float>(1.0f, 2.0f, 0.1f), 1.f, nullptr, nullptr);


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
    reductionRatio=0.25f;
    sizeOfDelayBuffer=sampleRate*2.0;
    delayBuffer.setSize(getTotalNumOutputChannels(),(int)sizeOfDelayBuffer);

    int hadamardOrderFour[4][4]={{1,1,1,1},{1,-1,1,-1},{1,1,-1,-1},{1,-1,-1,1}};
    int negativeHadamardOrderFour[4][4]={{-1,-1,-1,-1},{-1,1,-1,1},{-1,-1,1,1},{-1,1,1,-1}};
    for (int quarter=0 ;quarter<4;quarter++){
    for (int i=0 ;i<4;i++){
        for (int j=0 ;j<4;j++){
            if (quarter<2)
                hadamard[i][j+(quarter*4)]=hadamardOrderFour[i][j];
            if (quarter==2)
                hadamard[i+(4)][j]=hadamardOrderFour[i][j];
            else
                hadamard[i+(4)][j+(4)]=negativeHadamardOrderFour[i][j];
    }


    }}






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
        dbuffer.copyFromWithRamp(channel, writePosition,channelData,bufferSize,1.f,1.f);
    }
    else
    {
        auto samplesToFillBuffer=delayBufferSize-writePosition;
        dbuffer.copyFromWithRamp(channel,writePosition,channelData,samplesToFillBuffer,1.f,1.f);
        auto samplesRemaining=bufferSize-samplesToFillBuffer;
        dbuffer.copyFromWithRamp(channel,0,channelData+samplesToFillBuffer,samplesRemaining,1.f,1.f);
    }
}

void ReverbSEGAudioProcessor::writeDelayToOutputBuffer(juce::AudioBuffer<float>& buffer,juce::AudioBuffer<float>& dBuffer,int channel,int bufferSize,int delayBufferSize,float delayBufferMag,int tail,float gain){
    auto readPosition=writePosition-tail;

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
float ReverbSEGAudioProcessor::calcInvSqRoot( float n ) {
//alg from= https://www.tutorialspoint.com/fast-inverse-square-root-in-cplusplus
    const float threehalfs = 1.5f;
    float y = n;

    long i = * ( long * ) &y;

    i = 0x5f3759df - ( i >> 1 );
    y = * ( float * ) &i;

    y = y * ( threehalfs - ( (n * 0.5F) * y * y ) );

    return y;
}


void ReverbSEGAudioProcessor::generateMixMatrix(juce::AudioBuffer<float>& dBuffer,int bufferSize,int delayBufferSize,float delayBufferMag,int delayTimes[])
{

    if (delayBufferMag>0.02){       //avoid divide by zero error
        float relativeMagnitudes[reverbChannels]={0,0,0,0,0,0,0,0};// reset relative magnitudes
        /*
        going column-wise we read the max value found within our delay-time. (tapMagnitude)
        from this we determine its relative size to the max in the entire buffer. (tapMagnitude/delayBufferMag)
        this relation is then used as an input of a hadamard product, in summary what we aim to do is the following:
        take these magnitudes as inputs by assigning them a respective column withing the hadamard matrix so example,
        if we have 4 columns and 4 delayed samples a,b,c,d we would then have a matrix:

         [1, 1, 1, 1]     [a]       a= a+b+c+d
         [1,-1 ,1,-1]  *  [b]  =    b= a-b+c-d
         [1, 1,-1,-1]     [c]       c= a+b-c-d
         [1,-1,-1, 1]     [d]       d= a-b-c+d

        */
        for (int i=0;i< reverbChannels;i++) {
            auto readPosition=writePosition-delayTimes[i];
            if (readPosition<0)
                readPosition+=delayBufferSize;
            if (readPosition+bufferSize<delayBufferSize)
                relativeMagnitudes[i] =
                        (dBuffer.getMagnitude(readPosition, bufferSize)) / delayBufferMag;

            else {
                auto samplesToFill = delayBufferSize - readPosition;
                float a = (dBuffer.getMagnitude(readPosition, samplesToFill)) / delayBufferMag;
                auto bufferRemaining=bufferSize-samplesToFill;
                float b = (dBuffer.getMagnitude(0, bufferRemaining)) / delayBufferMag;
                if (a>=b)
                    relativeMagnitudes[i] =a/delayBufferMag;
                else
                    relativeMagnitudes[i] =b/delayBufferMag;
            }
            for (int j=0;j< reverbChannels;j++){
                hadamardProduct[i]+=relativeMagnitudes[j]*(float)hadamard[j][i];

            }


        }
        // we search through the array and store maximum value
        // we do this to normalize the value we pass to the gain of our delay lines
        for (int i=0;i<reverbChannels;i++) {
            if (hadamardProduct[i]>hadamardMax)
                hadamardMax=hadamardProduct[i];
        }
    }

}


void ReverbSEGAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    float gain = *state->getRawParameterValue(statenames[0]);
    float size = *state->getRawParameterValue(statenames[1]);
    float tail = *state->getRawParameterValue(statenames[2]);
    int delayTimes[reverbChannels];
    auto bufferSize=buffer.getNumSamples();

    for (int i=1;i<=reverbChannels;i++) {
        delayTimes[i-1]=(int)(((samplerate)/(tail*((float)i*2))));

    }
    int delayBufferSize=delayBuffer.getNumSamples();


    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {

        float* channelData = buffer.getWritePointer (channel);
        circularBuffer(delayBuffer,channel,bufferSize,delayBufferSize,channelData);
        float delayBufferMag=delayBuffer.getMagnitude(0,delayBufferSize);
        //updates hadamarard array with new mixin values
        generateMixMatrix(delayBuffer,bufferSize,delayBufferSize,delayBufferMag,delayTimes);

        for (int i=0;i<(int)reverbChannels;i++){

            writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,delayBufferMag,delayTimes[i],gain*(hadamardProduct[i]/hadamardMax));//reductionRatio);

        }



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
