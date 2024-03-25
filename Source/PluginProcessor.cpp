/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "math.h"

//==============================================================================
ChorusSEGAudioProcessor::ChorusSEGAudioProcessor()
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
    state->createAndAddParameter(statenames[0], paramNames[0], paramNames[0], juce::NormalisableRange<float>(0.01f, 1.0f, 0.01f), 0.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[1], paramNames[1], paramNames[1], juce::NormalisableRange<float>(5.f, 90.f, 1.0f), 0.f, nullptr, nullptr);
    state->createAndAddParameter(statenames[2], paramNames[2], paramNames[2], juce::NormalisableRange<float>(1.0f, 2.0f, 0.1f), 1.f, nullptr, nullptr);


    state->state = juce::ValueTree(statenames[0]);
    state->state = juce::ValueTree(statenames[1]);
    state->state = juce::ValueTree(statenames[2]);

}

ChorusSEGAudioProcessor::~ChorusSEGAudioProcessor()
{
}

//==============================================================================
const juce::String ChorusSEGAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ChorusSEGAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ChorusSEGAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ChorusSEGAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ChorusSEGAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ChorusSEGAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ChorusSEGAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ChorusSEGAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ChorusSEGAudioProcessor::getProgramName (int index)
{
    return {};
}

void ChorusSEGAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ChorusSEGAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    samplerate = sampleRate;
    reductionRatio=0.25f;
    sizeOfDelayBuffer=sampleRate*2.0;
    delayBuffer.setSize(getTotalNumOutputChannels(),(int)sizeOfDelayBuffer);   
    //audio programmer part 0 oscillator
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumOutputChannels();
    modSig.prepare(spec);
    chorusEffect.prepare(spec);

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

void ChorusSEGAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChorusSEGAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
void ChorusSEGAudioProcessor::circularBuffer(juce::AudioBuffer<float>& dbuffer,int channel,int bufferSize,int delayBufferSize,float *channelData){
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

void ChorusSEGAudioProcessor::mixAudioBuffers(juce::AudioBuffer<float>& src,juce::AudioBuffer<float>& dst,int channel_src,int channel_dst,float gain){

    int numSamples = dst.getNumSamples();
    float* destChannelData = dst.getWritePointer(channel_dst);
    const float* sourceChannelData = src.getReadPointer(channel_src);
    for (int sample = 0; sample < numSamples; ++sample)
        {
            if (gain==1){
                destChannelData[sample] += sourceChannelData[sample];
            }
            else if (gain==-1){
                destChannelData[sample] -= sourceChannelData[sample];
            }
            else{
                destChannelData[sample] +=gain*sourceChannelData[sample];

            }
        }
    }


void ChorusSEGAudioProcessor::modulateBuffer(juce::AudioBuffer<float>& buffer, int channel){
    //float * buffData=buffer.getWritePointer(channel);
   /* bool enlongate= true;
    for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
        float tune=0.1;
        int idx = int(sample * tune);
        float currentSample=buffer.getSample(channel, idx);


        buffData++;
    }*/
    //chorusEffect.setFeedback(0.5);
    //chorusEffect.setRate(100);
    //chorusEffect.setDepth(0.5);
    //tempBlock audioBlock = tempBlock(buffer);
    //juce::dsp::ProcessContextReplacing context = juce::dsp::ProcessContextReplacing(audioBlock);
    //chorusEffect.process(context);
    ////float modSample= modSig.processSample(currentSample);
    ////*buffData = modSample;

    tempBlock audioBlock = tempBlock(buffer);
    juce::dsp::ProcessContextReplacing context = juce::dsp::ProcessContextReplacing(audioBlock);
    chorusEffect.setFeedback(0.5);
    chorusEffect.setRate(5);
    chorusEffect.setDepth(0.2);
    chorusEffect.setMix(0.5);
    chorusEffect.setCentreDelay(5);
    chorusEffect.process(context);


}



void ChorusSEGAudioProcessor::writeDelayToOutputBuffer(juce::AudioBuffer<float>& buffer,juce::AudioBuffer<float>& dBuffer,int channel,int bufferSize,int delayBufferSize,float gain,float tail){
    virtualBuffer tmpBuff ,tmpBuff2;
    // this holds the delayed signals
    std::vector<int> shuffedOrder {0, 1, 2, 3, 4, 5, 6, 7};
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(shuffedOrder.begin(), shuffedOrder.end(), g);


    tmpBuff.setSize((reverbChannels),bufferSize);
    //this holds the result from hadamard matrix solution
    tmpBuff2.setSize((reverbChannels),bufferSize);

    for (int revChannel=0;revChannel<(int)reverbChannels;revChannel++) {
        int randomDelay=shuffedOrder[revChannel];

        int time =((int)(samplerate)/(int)((float)(randomDelay+2)*tail));
        auto readPosition=writePosition-time;
      // auto readPosition = writePosition - delayTimes[i];


        if (readPosition < 0)
            readPosition += delayBufferSize;
        if (readPosition + bufferSize < delayBufferSize)
            tmpBuff.copyFromWithRamp(revChannel, 0, dBuffer.getReadPointer(channel, (int) readPosition), bufferSize, 1,
                                   1);
        else {
            auto samplesToFill = delayBufferSize - readPosition;
            tmpBuff.copyFromWithRamp(revChannel, 0, dBuffer.getReadPointer(channel, (int) readPosition), (int) samplesToFill,
                                   1, 1);

            auto bufferRemaining = bufferSize - samplesToFill;
            tmpBuff.copyFromWithRamp(revChannel, (int) samplesToFill, dBuffer.getReadPointer(channel, 0),
                                   (int) bufferRemaining, 1, 1);

        }
    }

    for (int row=0;row<(int)reverbChannels;row++){
        for (int col=0;col<(int)reverbChannels;col++) {
            int hadamardGain=hadamard[row][col];
            

            if (col==0){

                tmpBuff2.copyFrom(row, 0, tmpBuff.getReadPointer(row, 0),
                                          bufferSize, hadamardGain);
            }
            else{
                mixAudioBuffers(tmpBuff,tmpBuff2,col,row,hadamardGain);
            }
        }
        
        mixAudioBuffers(tmpBuff2,buffer,row,channel,gain);
        
    }
    //buffer.addFromWithRamp(channel,0,outMix.getReadPointer(0,0),bufferSize,gain,gain);
    tmpBuff.clear();
    tmpBuff2.clear();
}
float ChorusSEGAudioProcessor::calcInvSqRoot( float n ) {
//alg from= https://www.tutorialspoint.com/fast-inverse-square-root-in-cplusplus
    const float threehalfs = 1.5f;
    float y = n;

    long i = * ( long * ) &y;

    i = 0x5f3759df - ( i >> 1 );
    y = * ( float * ) &i;

    y = y * ( threehalfs - ( (n * 0.5F) * y * y ) );

    return y;
}


void ChorusSEGAudioProcessor::generateMixMatrix(juce::AudioBuffer<float>& dBuffer,int bufferSize,int delayBufferSize,float delayBufferMag,int delayTimes[])
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
        //int timeVars[reverbChannels];

        for (int i=0;i< reverbChannels;i++) {
            int time =(int)(((samplerate)/(((float)i*2))));
            auto readPosition=writePosition-time;


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





void ChorusSEGAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());


    }


    float gain = *state->getRawParameterValue(statenames[0]);
    float freq= *state->getRawParameterValue(statenames[1]);
    float tail = *state->getRawParameterValue(statenames[2]);
    int delayTimes[reverbChannels];
    auto bufferSize=buffer.getNumSamples();
    int delayBufferSize=delayBuffer.getNumSamples();
    

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {

        float* channelData = buffer.getWritePointer(channel);

    /*    for (int sample = 0; sample< bufferSize; ++sample) {
           
            *channelData =audioBlock.getSample(channel, sample);
            channelData++;
        }*/
         
         
        
        circularBuffer(delayBuffer, channel, bufferSize, delayBufferSize, channelData);
        writeDelayToOutputBuffer(buffer,delayBuffer,channel,bufferSize,delayBufferSize,gain,tail);
 


        }
    tempBlock audioBlock = tempBlock(buffer);
    juce::dsp::ProcessContextReplacing context = juce::dsp::ProcessContextReplacing(audioBlock);
    chorusEffect.setFeedback(0.5);
    chorusEffect.setRate(freq);
    chorusEffect.setDepth(0.2);
    chorusEffect.setMix(0.5);
    chorusEffect.setCentreDelay(5);
    chorusEffect.process(context);

    writePosition+=bufferSize;
    writePosition%=delayBufferSize;

}
juce::AudioProcessorValueTreeState& ChorusSEGAudioProcessor::getState() {
    return *state;
}
//==============================================================================
bool ChorusSEGAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ChorusSEGAudioProcessor::createEditor()
{
    return new ChorusSEGAudioProcessorEditor (*this);
}

//==============================================================================
void ChorusSEGAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{

}

void ChorusSEGAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new ChorusSEGAudioProcessor();
}
