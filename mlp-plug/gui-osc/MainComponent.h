#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "MlpGui.hpp"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent final : public juce::Component
{
public:
    //==============================================================================
    MainComponent();

    //==============================================================================
    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    //==============================================================================
    MlpGui mlpGui;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
