#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() {
    addAndMakeVisible(mlpGui);
    //addAndMakeVisible(myButt);
    setSize (1200, 800);
}

void MainComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::whitesmoke);
}

void MainComponent::resized()
{
    //myButt.setBounds(this->getBoundsInParent());
    mlpGui.setBounds(this->getBoundsInParent());
}
