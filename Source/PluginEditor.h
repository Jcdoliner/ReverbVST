/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once
#define NSTATES 3
#include <JuceHeader.h>
#include "PluginProcessor.h"
using namespace juce;



//==============================================================================
/**
*/
class ReverbSEGAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ReverbSEGAudioProcessorEditor (ReverbSEGAudioProcessor&);
    ~ReverbSEGAudioProcessorEditor() override;
    void ReverbSEGAudioProcessorEditor::makeStateVisible(ScopedPointer<Slider> State, String name);

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void ReverbSEGAudioProcessorEditor::sliderStyle(ScopedPointer<Slider> State, int order);


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ReverbSEGAudioProcessor& audioProcessor;
    
    ScopedPointer<Slider> lengthState, tailState, sizeState;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment>lengthAtt, tailAtt, sizeAtt;
    ScopedPointer<Label> lengthTxt, tailTxt, sizeTxt;



    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbSEGAudioProcessorEditor)
};
