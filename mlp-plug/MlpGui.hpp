// MlpGui.hpp

// encapsulates JUCE-base GUI classes for controlling a MLP processor instance
// keeping this separate from the Editor class, as it may be used in non-plugin contexts
// (for example, in a OSC interface demonstration)

// in design-pattern terms this is a View class,
// with its inputs and outputs abstracted separately
// (output ==Model and input == ViewModel, if you like)

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Mlp.hpp"
#include "mlp/Constants.hpp"
#include "MlpLookAndFeel.hpp"
#include "MlpGuiIo.hpp"

class MlpGui : public juce::Component {



    //==============================================================================
    //=== I/O

    //MlpGuiInput* input = nullptr;
    MlpGuiOutput *output = nullptr;

    void SetOutput(MlpGuiOutput *aOutput) {
        output = aOutput;
    }

    //==============================================================================
    //=== GUI components

    static constexpr int buttonHeight = 64;
    static constexpr int tapButtonWidth = buttonHeight * 3 / 2;
//    static constexpr int toggleButtonWidth = buttonHeight * 3 / 2;
    static constexpr int sliderHeight = buttonHeight * 3 / 4;
//    static constexpr int sliderMinWidth = 128;

    //----------------------------------------------------------------------------------
    //--- widget classes

    class TapControl : public juce::TextButton {
        int tapIndex;
    public:
        TapControl(int aTapIndex, const std::string &label)
                : juce::TextButton(label),
                  tapIndex(aTapIndex) {
            setButtonText(label);
            onClick = [this] {
                std::cout << "TapControl::onClick; tap = " << tapIndex << std::endl;

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
            //setSliderStyle(juce::Slider::LinearBarVertical);
            setSliderStyle(juce::Slider::LinearBar);
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


    struct LayerRadioButton : public juce::ToggleButton {
        /// TODO
    };

    struct TapControlGroup : public juce::Component {
        std::vector<std::unique_ptr<TapControl>> controls;
        static constexpr size_t numTaps = (size_t) mlp::Mlp::TapId::Count;

        explicit TapControlGroup() {
            for (size_t i = 0; i < numTaps; ++i) {
                controls.push_back(std::make_unique<TapControl>(i, mlp::Mlp::TapIdLabel[i]));
                addAndMakeVisible(controls.back().get());
            }
        }

        void resized() override {
            juce::Grid grid;

            grid.setGap(juce::Grid::Px(4));
            grid.templateRows = {
                    juce::Grid::TrackInfo(juce::Grid::Fr(1)),
            };
            grid.templateColumns = {};
            for (unsigned int i = 0; i < numTaps; ++i) {
                grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth)));
            };
            for (auto &control: controls) {
                grid.items.add(juce::GridItem(*control));
            }
            grid.performLayout(getLocalBounds());
        }
    };

    //----------------------------------------------------------------------------------
    struct LayerControlGroup : public juce::Component {

        template<typename ControlType, unsigned int numWidgets, bool isVertical = false, int px = -1>
        struct LayerWidgetControlGroup : public juce::Component {
            std::vector<std::unique_ptr<ControlType>> controls;

            void resized() override {

                juce::Grid grid;

                using Track = juce::Grid::TrackInfo;
                using Fr = juce::Grid::Fr;
                using Px = juce::Grid::Px;

                grid.setGap(Px(4));

                if (isVertical) {
                    grid.templateRows = {};
                    for (unsigned int i = 0; i < numWidgets; ++i) {
                        if (px > 0) {
                            grid.templateRows.add(Track(Px(px)));
                        } else {
                            grid.templateRows.add(Track(Fr(1)));
                        }
                    };
                    grid.templateColumns = {
                            Track(Fr(1)),
                    };
                } else {
                    grid.templateRows = {
                            Track(Fr(1)),
                    };
                    grid.templateColumns = {};
                    for (unsigned int i = 0; i < numWidgets; ++i) {
                        if (px > 0) {
                            grid.templateColumns.add(Track(Px(px)));
                        } else {
                            grid.templateColumns.add(Track(Fr(1)));
                        }
                    };
                }
                for (auto &control: controls) {
                    grid.items.add(juce::GridItem(*control));
                }
                grid.performLayout(getLocalBounds());
            }
        };

        struct LayerToggleControlGroup
                : public LayerWidgetControlGroup<LayerToggleControl, (size_t) mlp::Mlp::BoolParamId::Count> {
            explicit LayerToggleControlGroup(int layerIndex) {
                for (int i = 0; i < (int) mlp::Mlp::IndexBoolParamId::Count; ++i) {
                    controls.push_back(
                            std::make_unique<LayerToggleControl>(layerIndex, i, mlp::Mlp::IndexBoolParamIdLabel[i]));
                    addAndMakeVisible(controls.back().get());
                }
            }
        };

        struct LayerParameterControlGroup
                : public LayerWidgetControlGroup<LayerParameterControl, (size_t) mlp::Mlp::FloatParamId::Count, true, sliderHeight> {
            explicit LayerParameterControlGroup(int layerIndex) {
                for (int i = 0; i < (int) mlp::Mlp::FloatParamId::Count; ++i) {
                    controls.push_back(std::make_unique<LayerParameterControl>(layerIndex, i,
                                                                               mlp::Mlp::IndexFloatParamIdLabel[i]));
                    addAndMakeVisible(controls.back().get());
                }
            }
        };

        std::unique_ptr<LayerToggleControlGroup> toggleControlGroup;
        std::unique_ptr<LayerParameterControlGroup> parameterControlGroup;

    public:
        explicit LayerControlGroup(int layerIndex) {
            toggleControlGroup = std::make_unique<LayerToggleControlGroup>(layerIndex);
            parameterControlGroup = std::make_unique<LayerParameterControlGroup>(layerIndex);
            addAndMakeVisible(toggleControlGroup.get());
            addAndMakeVisible(parameterControlGroup.get());

        }

        void resized() override {
            juce::Grid grid;

            using Track = juce::Grid::TrackInfo;
            using Fr = juce::Grid::Fr;
            using Px = juce::Grid::Px;

            grid.setGap(Px(4));

            grid.templateColumns = {
                    Track(Fr(1)),
            };
            grid.templateRows = {
                    Track(Px(buttonHeight)),
                    Track(Fr(1)),
                    //Track(Fr(1)),
            };
            grid.items.add(juce::GridItem(*toggleControlGroup));
            grid.items.add(juce::GridItem(*parameterControlGroup));
            // other things:
            // - layer select radio buttons (across layers)
            // - mode select radio buttons (per layer)
            // - outputs log (per layer)

            grid.performLayout(getLocalBounds());
        }
    };

    //----------------------------------------------------------------------------------
private:
    MlpLookAndFeel lookAndFeel;
    std::unique_ptr<TapControlGroup> tapControlGroup;

    struct LayerControlStack : public juce::Component {
        std::array<std::unique_ptr<LayerControlGroup>, mlp::numLoopLayers> layerControlGroups;

        LayerControlStack() {
            for (unsigned int i = 0; i < mlp::numLoopLayers; ++i) {
                layerControlGroups[i] = std::make_unique<LayerControlGroup>(i);
                addAndMakeVisible(layerControlGroups[i].get());
            }
        }

        void resized() override {
            juce::Grid grid;
            grid.setGap(juce::Grid::Px(8));
            for (unsigned int i=0; i<layerControlGroups.size(); ++i) {
                grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Fr(1)));
            }
            grid.templateRows = {juce::Grid::TrackInfo(juce::Grid::Fr(1))};
            for (auto &layer: layerControlGroups) {
                grid.items.add(juce::GridItem(*layer));
            }
            grid.performLayout(getLocalBounds());
        }
    };

    std::unique_ptr<LayerControlStack> layerControlStack;

public:
    MlpGui() {
        tapControlGroup = std::make_unique<TapControlGroup>();
        layerControlStack = std::make_unique<LayerControlStack>();
        addAndMakeVisible(tapControlGroup.get());
        addAndMakeVisible(layerControlStack.get());
    }

    void resized() override {
        juce::Grid grid;

        using Track = juce::Grid::TrackInfo;
        using Fr = juce::Grid::Fr;
        using Px = juce::Grid::Px;

        grid.setGap(Px(8));

        grid.templateRows = {
                Track(Px(buttonHeight)),
                Track(Fr(1)),
        };
        grid.templateColumns = {
                Track(Fr(1))
        };

        grid.items.add(juce::GridItem(*tapControlGroup));
        grid.items.add(juce::GridItem(*layerControlStack));

        grid.performLayout(getLocalBounds());

    }

    void SetLayerToggleState(unsigned int layerIndex, unsigned int toggleIndex, bool state) {
        auto &but = layerControlStack->layerControlGroups[layerIndex]->toggleControlGroup->controls[toggleIndex];
        but->setToggleState(state, juce::NotificationType::dontSendNotification);
    }

    void SetLayerSelection(unsigned int layerIndex) {
        //// TODO
        (void) layerIndex;
    }

};
