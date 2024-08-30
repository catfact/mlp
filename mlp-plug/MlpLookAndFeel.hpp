#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

class MlpLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MlpLookAndFeel() {
//        setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::lightsteelblue);
//        setColour(juce::Slider::thumbColourId, juce::Colours::black);
//        setColour(juce::Slider::trackColourId, juce::Colours::cornflowerblue);
//        setColour(juce::Slider::textBoxTextColourId, juce::Colours::ghostwhite);
//        setColour(juce::TextButton::textColourOnId, juce::Colours::lightsteelblue);
//        setColour(juce::TextButton::textColourOffId, juce::Colours::lightslategrey);
//        setColour(juce::TextButton::buttonColourId, juce::Colours::lightsteelblue);
//        setColour(juce::TextButton::buttonOnColourId, juce::Colours::cornflowerblue);



        setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);
        setColour(juce::Slider::thumbColourId, juce::Colours::grey);
        setColour(juce::Slider::trackColourId, juce::Colours::lightslategrey);
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::ghostwhite);
        setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        setColour(juce::TextButton::textColourOffId, juce::Colours::ghostwhite);
        setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
        setColour(juce::TextButton::buttonOnColourId, juce::Colours::ghostwhite);

        LookAndFeel::setDefaultLookAndFeel (this);
    }
};