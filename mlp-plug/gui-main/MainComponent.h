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
    // void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    MlpGui mlpGui;

    class MyButt : public juce::Button {
    public:
        MyButt(const juce::String &buttonName) : Button(buttonName) {
            setButtonText("My Button");
            onClick = [] {
                std::cout << "MyButton::onClick" << std::endl;
            };
        }

        void paintButton(juce::Graphics &g, bool isMouseOverButton, bool isButtonDown) override {
            (void)isMouseOverButton;
            (void)isButtonDown;
            (void)g;
            //g.fillAll(juce::Colours::black);
        }
    };

    MyButt myButt;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
