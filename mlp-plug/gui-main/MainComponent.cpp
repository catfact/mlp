#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : myButt("butt") {
    addAndMakeVisible(mlpGui);
    //addAndMakeVisible(myButt);
    setSize (600, 400);
}

//==============================================================================
//void MainComponent::paint (juce::Graphics& g)
//{
//    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
//
//    g.setFont (juce::FontOptions (16.0f));
//    g.setColour (juce::Colours::white);
//    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
//}

void MainComponent::resized()
{
    //myButt.setBounds(this->getBoundsInParent());
    mlpGui.setBounds(this->getBoundsInParent());
}
