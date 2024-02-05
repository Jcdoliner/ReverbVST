/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;
//==============================================================================
ReverbSEGAudioProcessorEditor::ReverbSEGAudioProcessorEditor (ReverbSEGAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{

    addAndMakeVisible(lengthState = new Slider("Length")); 
    lengthState->Slider::setSliderStyle(Slider::LinearBarVertical);
    lengthState->Slider::setTextBoxStyle(Slider::NoTextBox, false, 100, 100);
    addAndMakeVisible(sizeState = new Slider("Size"));
    sizeState->Slider::setSliderStyle(Slider::LinearBarVertical);
    sizeState->Slider::setTextBoxStyle(Slider::NoTextBox, false, 100, 100);
    addAndMakeVisible(tailState = new Slider("Tail"));
    tailState->Slider::setSliderStyle(Slider::LinearBarVertical);
    tailState->Slider::setTextBoxStyle(Slider::NoTextBox, false, 100, 100);



    addAndMakeVisible(lengthTxt = new Label("lenghtTxt"));
    lengthTxt->Label::setText("Length", dontSendNotification);
    lengthTxt->Label::attachToComponent(lengthState, false);

    addAndMakeVisible(sizeTxt = new Label("sizeTxt"));
    sizeTxt->Label::setText("Size", dontSendNotification);
    sizeTxt->Label::attachToComponent(sizeState, false);

    addAndMakeVisible(tailTxt = new Label("tailTxt"));
    tailTxt->Label::setText("Tail", dontSendNotification);
    tailTxt->Label::attachToComponent(tailState, false);

    //lengthAtt = new AudioProcessorValueTreeState::SliderAttachment(p.getState(), "length", *lengthSlider);
    //sizeAtt = new AudioProcessorValueTreeState::SliderAttachment(p.getState(), "size", *sizeSlider);
    //tailAtt = new AudioProcessorValueTreeState::SliderAttachment(p.getState(), "tail", *tailSlider);

        //addAndMakeVisible(lengthState = new Slider(""));
        //lengthState->Slider::setSliderStyle(Slider::LinearBarVertical);
        //lengthState->Slider::setTextBoxStyle(Slider::NoTextBox, false, 100, 100);
    

     
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (300, 200);
}
void ReverbSEGAudioProcessorEditor::makeStateVisible(ScopedPointer<Slider> State, String name){

   
    State->Slider::setSliderStyle(Slider::LinearBarVertical);
    State->Slider::setTextBoxStyle(Slider::NoTextBox, false, 100, 100);

    }


void ReverbSEGAudioProcessorEditor::sliderStyle(ScopedPointer<Slider> sliderState,int order) {
    int xhalf = ((300 / 2));
    int yhalf = (200 / 2);
    int sliderWidth = 50;
    sliderState->setBounds(
            (xhalf - 100) + (order * 100)
            , yhalf -70
            , sliderWidth
            , yhalf + 20);//
     
}
ReverbSEGAudioProcessorEditor::~ReverbSEGAudioProcessorEditor()
{
}

//==============================================================================
void ReverbSEGAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour(Colours::white);
    g.setFont(15.0f);

    
}

void ReverbSEGAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    int xhalf = ((300 / 2)-125);
    int yhalf = (200 / 2);
    int sliderWidth = 50;
    lengthState->setBounds((xhalf)
        , yhalf-60
        , sliderWidth
        , yhalf + 30);

    sizeState->setBounds((xhalf ) + ( 100)
        , yhalf-60
        , sliderWidth
        , yhalf + 30);
    tailState->setBounds((xhalf) + ( 200)
        , yhalf-60
        , sliderWidth
        , yhalf + 30);

}



