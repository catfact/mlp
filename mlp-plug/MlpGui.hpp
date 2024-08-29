// MlpGui.hpp

// encapsulates JUCE-base GUI classes for controlling a MLP processor instance
// keeping this separate from the Editor class, as it may be used in non-plugin contexts
// (for example, in a OSC interface demonstration)

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Mlp.hpp"
#include "mlp/Constants.hpp"

class MlpGui : public juce::Component {


    static constexpr int buttonHeight = 48;
    static constexpr int tapButtonWidth = buttonHeight;
    static constexpr int toggleButtonWidth = buttonHeight * 3 / 2;
    static constexpr int sliderHeight = buttonHeight / 2;
    static constexpr int sliderMinWidth = 128;

    //----------------------------------------------------------------------------------
    //--- widget classes

    class LayerTapControl : public juce::TextButton {
        int layerIndex;
        int tapIndex;
    public:
        LayerTapControl(int aLayerIndex, int aTapIndex, const std::string &label)
                : juce::TextButton(label),
                  layerIndex(aLayerIndex),
                  tapIndex(aTapIndex) {
            setButtonText(label);
            onClick = [this] {
                std::cout << "LayerTapControl::onClick; layer = " << layerIndex << ", tap = " << tapIndex << std::endl;
            };
        }
    };

    class LayerToggleControl : public juce::TextButton {
        int layerIndex;
        int toggleIndex;
    public:
        LayerToggleControl(int aLayerIndex, int aToggleIndex, const std::string &label)
                : juce::TextButton(label),
                  layerIndex(aLayerIndex),
                  toggleIndex(aToggleIndex) {
            setButtonText(label);
            setToggleable(true);
            setClickingTogglesState(true);
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
            setSliderStyle(juce::Slider::LinearBarVertical);
            //setSliderStyle(juce::Slider::RotaryHorizontalDrag);
            setHelpText(aLabel);
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
    struct LayerControlGroup : public juce::GroupComponent {

        template <typename ControlType, unsigned int numWidgets>
        struct LayerWidgetControlGroup: public juce::GroupComponent {

            std::vector<std::unique_ptr<ControlType>> controls;

            void resized() override {

                juce::Grid grid;

                using Track = juce::Grid::TrackInfo;
                using Fr = juce::Grid::Fr;
                grid.templateRows    = {
                        Track (Fr (1)),
                };
                grid.templateColumns = {};
                for (unsigned int i=0; i<numWidgets; ++i) {
                    grid.templateColumns.add(Track (Fr (1)));
                };
                for (auto &control: controls) {
                    grid.items.add(juce::GridItem(*control));
                }
                grid.performLayout(getLocalBounds());
            }
        };

        //// not how it works, at least for now - taps are global
//        struct LayerTapControlGroup : LayerWidgetControlGroup<LayerTapControl, (size_t)mlp::Mlp::TapId::Count> {
//            explicit LayerTapControlGroup(unsigned int layerIndex) {
//                for (int i = 0; i < (int) mlp::Mlp::TapId::Count; ++i) {
//                    controls.push_back(std::make_unique<LayerTapControl>(layerIndex, i, mlp::Mlp::TapIdLabel[i]));
//                    addAndMakeVisible(controls.back().get());
//                }
//            }
//        };

        struct LayerToggleControlGroup : public LayerWidgetControlGroup<LayerToggleControl, (size_t)mlp::Mlp::BoolParamId::Count> {
            explicit LayerToggleControlGroup(int layerIndex) {
                for (int i=0; i<(int)mlp::Mlp::BoolParamId::Count; ++i) {
                    controls.push_back(std::make_unique<LayerToggleControl>(layerIndex, i, mlp::Mlp::IndexBoolParamIdLabel[i]));
                    addAndMakeVisible(controls.back().get());
                }
            }
        };

        struct LayerParameterControlGroup : public LayerWidgetControlGroup<LayerParameterControl, (size_t)mlp::Mlp::FloatParamId::Count> {
            explicit LayerParameterControlGroup(int layerIndex) {
                for (int i=0; i<(int)mlp::Mlp::FloatParamId::Count; ++i) {
                    controls.push_back(std::make_unique<LayerParameterControl>(layerIndex, i, mlp::Mlp::IndexFloatParamIdLabel[i]));
                    addAndMakeVisible(controls.back().get());
                }
            }
        };

        std::unique_ptr<LayerTapControlGroup> tapControlGroup;
        std::unique_ptr<LayerToggleControlGroup> toggleControlGroup;
        std::unique_ptr<LayerParameterControlGroup> parameterControlGroup;

    public:
        explicit LayerControlGroup(int layerIndex) {
            tapControlGroup = std::make_unique<LayerTapControlGroup>(layerIndex);
            toggleControlGroup = std::make_unique<LayerToggleControlGroup>(layerIndex);
            parameterControlGroup = std::make_unique<LayerParameterControlGroup>(layerIndex);

            addAndMakeVisible(tapControlGroup.get());
            addAndMakeVisible(toggleControlGroup.get());
            addAndMakeVisible(parameterControlGroup.get());

        }

        void resized() override {
            juce::Grid grid;

            using Track = juce::Grid::TrackInfo;
            using Fr = juce::Grid::Fr;

            grid.templateRows    = {
                    Track (Fr (1)),
            };
            grid.templateColumns = {
                    Track (Fr (1)),
                    Track (Fr (1)),
                    Track (Fr (1)),
            };
            grid.items.add(juce::GridItem(*tapControlGroup));
            grid.items.add(juce::GridItem(*toggleControlGroup));
            grid.items.add(juce::GridItem(*parameterControlGroup));

            grid.performLayout(getLocalBounds());
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
        juce::Grid grid;

        using Track = juce::Grid::TrackInfo;
        using Fr = juce::Grid::Fr;

        grid.templateRows    = {
                Track (Fr (1)),
                Track (Fr (1)),
                Track (Fr (1)),
                Track (Fr (1)),
        };
        grid.templateColumns = {
                Track (Fr (1))
        };
        for (auto &layer: layerControlGroups) {
            grid.items.add(juce::GridItem(*layer));
        }
        grid.performLayout(getLocalBounds());

    }
};