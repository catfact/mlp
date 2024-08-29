// MlpGui.hpp

// encapsulates JUCE-base GUI classes for controlling a MLP processor instance
// keeping this separate from the Editor class, as it may be used in non-plugin contexts
// (for example, in a OSC interface demonstration)

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Mlp.hpp"
#include "mlp/Constants.hpp"

class MlpGui : public juce::Component {

    //----------------------------------------------------------------------------------
    //--- widget classes

    class LayerTapControl : public juce::Button {
        int layerIndex;
        int tapIndex;
    public:
        LayerTapControl(int aLayerIndex, int aTapIndex, const std::string &label)
                : juce::Button(label),
                  layerIndex(aLayerIndex),
                  tapIndex(aTapIndex) {
            setButtonText("label");
            onClick = [this] {
                std::cout << "LayerTapControl::onClick; layer = " << layerIndex << ", tap = " << tapIndex << std::endl;
                //mlp.TapLayer(layerIndex, tapIndex);
            };
        }

        // ?? why do we need to override this..
        void paintButton(juce::Graphics &g, bool isMouseOverButton, bool isButtonDown) override {
            (void) isMouseOverButton;
            (void) isButtonDown;
            (void) g;
            //g.fillAll(juce::Colours::black);
        }
    };

    class LayerToggleControl : public juce::ToggleButton {
        int layerIndex;
        int toggleIndex;
    public:
        LayerToggleControl(int aLayerIndex, int aToggleIndex, const std::string &label)
                : juce::ToggleButton(label),
                  layerIndex(aLayerIndex),
                  toggleIndex(aToggleIndex) {
            setButtonText("label");
            onClick = [this] {
                std::cout << "LayerToggleControl::onClick; layer = " << layerIndex << ", toggle = " << toggleIndex
                          << ", state = " << (getToggleState() ? "ON" : "OFF") << std::endl;
            };
        }
    };

    class LayerParameterControl : public juce::Slider {
        int layerIndex;
        int parameterIndex;
    public:
        LayerParameterControl(int aLayerIndex, int aParameterIndex, const std::string &aLabel) :
                layerIndex(aLayerIndex),
                parameterIndex(aParameterIndex) {

            // TODO: show the label somewhere
            (void) aLabel;
            setSliderStyle(juce::Slider::LinearBar);
            setRange(0.0, 1.0, 0.001);
            //setValue(mlp.GetLayerParameter(layerIndex, parameterIndex));
            onValueChange = [=] {
                std::cout << "LayerParameterControl::onValueChange; layer = " << layerIndex << ", parameter = "
                          << parameterIndex << "(" << aLabel << "), value = " << getValue() << std::endl;
            };
        }
    };


    class LayerRadioButton : public juce::ToggleButton {
        /// TODO
    };

    //----------------------------------------------------------------------------------
    struct LayerControlGroup : public juce::Component {

        struct LayerTapControlGroup : public juce::GroupComponent {
            std::vector<std::unique_ptr<LayerTapControl>> tapControls;

            explicit LayerTapControlGroup(unsigned int layerIndex) {
                for (int i=0; i<(int)mlp::Mlp::TapId::Count; ++i) {
                    tapControls.push_back(std::make_unique<LayerTapControl>(layerIndex, i, mlp::Mlp::TapIdLabel[i]));
                    addAndMakeVisible(tapControls.back().get());
                }
            }

//            void AddTapControl(int layerIndex, int tapIndex, const std::string &label) {
//                tapControls.push_back(std::make_unique<LayerTapControl>(layerIndex, tapIndex, label));
//                addAndMakeVisible(tapControls.back().get());
//            }

            void resized() override {
                juce::FlexBox fb;
                for (auto &control: tapControls) {
                    fb.items.add(juce::FlexItem(*control));
                }
                fb.performLayout(getLocalBounds());
            }
        };

        struct LayerToggleControlGroup : public juce::GroupComponent {
            std::vector<std::unique_ptr<LayerToggleControl>> toggleControls;

            explicit LayerToggleControlGroup(int layerIndex) {
                for (int i=0; i<(int)mlp::Mlp::BoolParamId::Count; ++i) {
                    toggleControls.push_back(std::make_unique<LayerToggleControl>(layerIndex, i, mlp::Mlp::BoolParamIdLabel[i]));
                    addAndMakeVisible(toggleControls.back().get());
                }
            }

//            void AddToggleControl(int layerIndex, int toggleIndex, const std::string &label) {
//                toggleControls.push_back(std::make_unique<LayerToggleControl>(layerIndex, toggleIndex, label));
//                addAndMakeVisible(toggleControls.back().get());
//            }

            void resized() override {
                juce::FlexBox fb;
                for (auto &control: toggleControls) {
                    fb.items.add(juce::FlexItem(*control));
                }
                fb.performLayout(getLocalBounds());
            }
        };

        struct LayerParameterControlGroup : public juce::GroupComponent {
            std::vector<std::unique_ptr<LayerParameterControl>> parameterControls;

            explicit LayerParameterControlGroup(int layerIndex) {
                for (int i=0; i<(int)mlp::Mlp::FloatParamId::Count; ++i) {
                    parameterControls.push_back(std::make_unique<LayerParameterControl>(layerIndex, i, mlp::Mlp::FloatParamIdLabel[i]));
                    addAndMakeVisible(parameterControls.back().get());
                }
            }

//            void AddParameterControl(int layerIndex, int parameterIndex, const std::string &label) {
//                parameterControls.push_back(std::make_unique<LayerParameterControl>(layerIndex, parameterIndex, label));
//                addAndMakeVisible(parameterControls.back().get());
//            }

            void resized() override {
                juce::FlexBox fb;
                for (auto &control: parameterControls) {
                    fb.items.add(juce::FlexItem(*control));
                }
                fb.performLayout(getLocalBounds());
            }
        };

        std::unique_ptr<LayerTapControlGroup> tapControlGroup;
        std::unique_ptr<LayerToggleControlGroup> toggleControlGroup;
        std::unique_ptr<LayerParameterControlGroup> parameterControlGroup;

#if 0
        void AddTapControl(int layerIndex, int tapIndex, const std::string &label) {
            tapControlGroup.AddTapControl(layerIndex, tapIndex, label);
        }

        void AddToggleControl(int layerIndex, int toggleIndex, const std::string &label) {
            toggleControlGroup.AddToggleControl(layerIndex, toggleIndex, label);
        }

        void AddParameterControl(int layerIndex, int parameterIndex, const std::string &label) {
            parameterControlGroup.AddParameterControl(layerIndex, parameterIndex, label);
        }
#endif

    public:
        explicit LayerControlGroup(int layerIndex) {
            tapControlGroup = std::make_unique<LayerTapControlGroup>(layerIndex);
            addAndMakeVisible(tapControlGroup.get());
            toggleControlGroup = std::make_unique<LayerToggleControlGroup>(layerIndex);
            addAndMakeVisible(toggleControlGroup.get());
            parameterControlGroup = std::make_unique<LayerParameterControlGroup>(layerIndex);
            addAndMakeVisible(parameterControlGroup.get());
#if 0
            for (int i=0; i<(int)mlp::Mlp::TapId::Count; ++i) {
                AddTapControl(0, i, mlp::Mlp::TapIdLabel[i]);
            }

            for (int i=0; i<(int)mlp::Mlp::BoolParamId::Count; ++i) {
                AddToggleControl(0, i, mlp::Mlp::BoolParamIdLabel[i]);
            }

            for (int i=0; i<(int)mlp::Mlp::FloatParamId::Count; ++i) {
                AddParameterControl(0, i, mlp::Mlp::FloatParamIdLabel[i]);
            }
#endif
        }

        void resized() override {
            juce::FlexBox fb;
            fb.items.add(juce::FlexItem(*tapControlGroup));
            fb.items.add(juce::FlexItem(*toggleControlGroup));
            fb.items.add(juce::FlexItem(*parameterControlGroup));
            fb.performLayout(getLocalBounds());
        }
    };

    //----------------------------------------------------------------------------------
private:
    std::array<std::unique_ptr<LayerControlGroup>, mlp::numLoopLayers> layerControlGroups;

public:
    MlpGui() {
        for (unsigned int i = 0; i < mlp::numLoopLayers; ++i) {
            layerControlGroups[i] = std::make_unique<LayerControlGroup>(i);
            addAndMakeVisible(layerControlGroups[i].get());
        }
    }

    void resized() override {
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::column;
        for (auto &layer: layerControlGroups) {
            fb.items.add(juce::FlexItem(*layer).withMinWidth(100).withMinHeight(100));
        }
        fb.performLayout(getLocalBounds());
    }
};
