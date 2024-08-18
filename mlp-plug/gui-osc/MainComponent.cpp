#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() {
    addAndMakeVisible(mlpGui);
    setSize (1200, 800);
}

void MainComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::lightsteelblue);
}

void MainComponent::resized()
{
    //myButt.setBounds(this->getBoundsInParent());
    mlpGui.setBounds(this->getBoundsInParent());
}
