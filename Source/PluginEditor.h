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
class ChorusSEGAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ChorusSEGAudioProcessorEditor (ChorusSEGAudioProcessor&);
    ~ChorusSEGAudioProcessorEditor() override;
    void ChorusSEGAudioProcessorEditor::makeStateVisible(ScopedPointer<Slider> State, String name);

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void ChorusSEGAudioProcessorEditor::sliderStyle(ScopedPointer<Slider> State, int order);


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ChorusSEGAudioProcessor& audioProcessor;
    
    ScopedPointer<Slider> lengthState, tailState, sizeState;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment>lengthAtt, tailAtt, sizeAtt;
    ScopedPointer<Label> lengthTxt, tailTxt, sizeTxt;



    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusSEGAudioProcessorEditor)
};
