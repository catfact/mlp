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
#include "mlp/LayerBehavior.hpp"
#include "MlpLookAndFeel.hpp"
#include "MlpGuiIo.hpp"

class MlpGui : public juce::Component {

//==============================================================================
//=== I/O

    // MlpGuiOutput *output = nullptr;

    /// FIXME: this is lame, but i don't to deal with passing the output pointer down to all child widgets
    /// for now, making the output interface into a global variable :(
    static MlpGuiOutput *output;

    //==============================================================================
    //=== GUI components

    static constexpr int buttonHeight = 64;
    static constexpr int tapButtonWidth = buttonHeight * 3 / 2;
    static constexpr int sliderHeight = buttonHeight * 3 / 4;

    //==============================================================================
    //====  widget classes

    //----------------------------------------------------------------------------------
    class Spacer : public juce::Component {
    public:
        void paint(juce::Graphics &g) override {
            g.fillAll(juce::Colours::transparentBlack);
        }
    };

    class TapControl : public juce::TextButton {
        int tapIndex;
    public:
        TapControl(int aTapIndex, const std::string &label)
                : juce::TextButton(label),
                  tapIndex(aTapIndex) {
            setButtonText(label);
            onClick = [this] {
                // std::cout << "TapControl::onClick; tap = " << tapIndex << std::endl;
                if (output) {
                    output->SendTap(static_cast<mlp::Mlp::TapId>(tapIndex));
                }
            };
        }
    };

    //----------------------------------------------------------------------------------
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
                // std::cout << "LayerToggleControl::onClick; layer = " << layerIndex << ", toggle = " << toggleIndex
                //           << ", state = " << (getToggleState() ? "ON" : "OFF") << std::endl;
                if (output) {
                    output->SendIndexBool(static_cast<mlp::Mlp::IndexBoolParamId>(toggleIndex),
                                          (unsigned int) layerIndex,
                                          getToggleState());
                }
            };
        }
    };

    //----------------------------------------------------------------------------------
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
                // std::cout << "LayerParameterControl::onValueChange; layer = " << layerIndex << ", parameter = "
                //          << parameterIndex << "(" << aLabel << "), value = " << getValue() << std::endl;

                if (output) {
                    output->SendIndexFloat(static_cast<mlp::Mlp::IndexFloatParamId>(parameterIndex),
                                           static_cast<unsigned int>(layerIndex), static_cast<float>(getValue()));
                }
            };
        }
    };

    //----------------------------------------------------------------------------------
    struct LayerOutputLog : public juce::TextEditor {
        static constexpr int numLines = 24;
        std::array<std::string, numLines> lines{};
        unsigned int lineIndex = 0;

        explicit LayerOutputLog() {
            setMultiLine(true);
            setReadOnly(true);
        }

        void AddLine(const std::string &line) {
            lines[lineIndex] = line;
            lineIndex = (lineIndex + 1) % numLines;
            std::string text;
            for (unsigned int i = 0; i < numLines; ++i) {
                text += lines[(lineIndex + i) % numLines] + "\n";
            }
            setText(text, juce::dontSendNotification);
        }

    };


//==============================================================================================
//====== container classes


    //----------------------------------------------------------------------------------
    struct GlobalControlGroup : public juce::Component {
        std::vector<std::unique_ptr<TapControl>> tapControls;
        static constexpr size_t numTaps = (size_t) mlp::Mlp::TapId::Count;
        std::vector<std::unique_ptr<juce::TextButton>> modeButtons;

        Spacer space1;
        Spacer space2;

        explicit GlobalControlGroup() {
            for (size_t i = 0; i < numTaps; ++i) {
                tapControls.push_back(std::make_unique<TapControl>(i, mlp::Mlp::TapIdLabel[i]));
                addAndMakeVisible(tapControls.back().get());
            }

            auto addModeBut = [this](const std::string &label, mlp::LayerBehaviorModeId mode) {
                auto but = std::make_unique<juce::TextButton>(label);
                but->setRadioGroupId(1);
                but->setClickingTogglesState(true);
                but->setToggleable(true);
                but->onClick = [mode] {
                    // std::cout << "GlobalControlGroup::modeButton::onClick; mode = " << (int)mode << std::endl;
                    if (output) {
                        output->SendIndex(mlp::Mlp::IndexParamId::Mode, static_cast<unsigned int>(mode));
                    }
                };
                addAndMakeVisible(but.get());
                modeButtons.push_back(std::move(but));
            };

            addModeBut("ASYNC", mlp::LayerBehaviorModeId::ASYNC);
            addModeBut("MULTIPLY", mlp::LayerBehaviorModeId::MULTIPLY_UNQUANTIZED);
            addModeBut("INSERT", mlp::LayerBehaviorModeId::INSERT_UNQUANTIZED);

            addAndMakeVisible(space1);
            addAndMakeVisible(space2);
        }

        void resized() override {
            juce::Grid grid;

            grid.setGap(juce::Grid::Px(4));
            grid.templateRows = {
                    juce::Grid::TrackInfo(juce::Grid::Fr(1)),
            };
            grid.templateColumns = {};

            // spacer
            grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth/2)));

            for (unsigned int i = 0; i < numTaps; ++i) {
                grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth)));
            };

            // spacer
            grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth/2)));

            /// mode buttons
            grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth)));
            grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth)));
            grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth)));

            grid.items.add(juce::GridItem(space1));

            for (auto &control: tapControls) {
                grid.items.add(juce::GridItem(*control));
            }

            grid.items.add(juce::GridItem(space2));

            for (auto &control: modeButtons) {
                grid.items.add(juce::GridItem(*control));
            }

            grid.performLayout(getLocalBounds());
        }
    };

    //----------------------------------------------------------------------------------
    struct LayerControlGroup : public juce::Component {

        mlp::frame_t loopEndFrame{mlp::bufferFrames};

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

    //----------------------------------------------------------------------------------
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

    //----------------------------------------------------------------------------------
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

        struct LayerPositionDislpay: juce::Component {
            // unit-scaled positions
            float startPos, endPos;
            void paint(juce::Graphics &g) override {
                g.fillAll(juce::Colours::black);
                if (startPos < endPos) {
                    g.setColour(juce::Colours::white);
                    //g.fillRect(startPos * getWidth(), 0, (endPos - startPos) * getWidth(), getHeight());
                    g.fillRect(startPos * getWidth(), 0.f, (endPos - startPos) * getWidth(), (float)getHeight());
                } else {
                    // fill from zero to start
                    g.fillRect(0.f, 0.f, startPos * getWidth(), (float)getHeight());
                    // fill from end to one
                    g.fillRect(endPos * getWidth(), 0.f, (1.f - endPos) * getWidth(), (float)getHeight());
                }
            }

        };


        std::unique_ptr<juce::TextButton> selectedToggle;
        std::unique_ptr<LayerToggleControlGroup> toggleControlGroup;
        std::unique_ptr<LayerParameterControlGroup> parameterControlGroup;
        std::unique_ptr<LayerPositionDislpay> positionDisplay;
        std::unique_ptr<LayerOutputLog> outputLog;


    public:
        explicit LayerControlGroup(int layerIndex) {
            selectedToggle = std::make_unique<juce::TextButton>("SELECTED");
            selectedToggle->setToggleable(true);
            // FIXME: for now, we don't have explicit layer selection!
            selectedToggle->setClickingTogglesState(false);

            toggleControlGroup = std::make_unique<LayerToggleControlGroup>(layerIndex);
            parameterControlGroup = std::make_unique<LayerParameterControlGroup>(layerIndex);
            outputLog = std::make_unique<LayerOutputLog>();

            addAndMakeVisible(selectedToggle.get());
            addAndMakeVisible(toggleControlGroup.get());
            addAndMakeVisible(parameterControlGroup.get());
            addAndMakeVisible(outputLog.get());
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
                    Track(Px(buttonHeight)),
                    Track(Fr(1)),
                    Track(Px(buttonHeight)),
                    Track(Fr(2)),
            };

            grid.items.add(juce::GridItem(*selectedToggle));
            grid.items.add(juce::GridItem(*toggleControlGroup));
            grid.items.add(juce::GridItem(*parameterControlGroup));
            grid.items.add(juce::GridItem(*positionDisplay));
            grid.items.add(juce::GridItem(*outputLog));

            grid.performLayout(getLocalBounds());
        }
    };

//----------------------------------------------------------------------------------
private:
    MlpLookAndFeel lookAndFeel;
    std::unique_ptr<GlobalControlGroup> globalControlGroup;

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
            for (unsigned int i = 0; i < layerControlGroups.size(); ++i) {
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
        globalControlGroup = std::make_unique<GlobalControlGroup>();
        layerControlStack = std::make_unique<LayerControlStack>();
        addAndMakeVisible(globalControlGroup.get());
        addAndMakeVisible(layerControlStack.get());
        output = nullptr;
    }

    void resized()

    override {
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

        grid.items.add(juce::GridItem(*globalControlGroup));
        grid.items.add(juce::GridItem(*layerControlStack));

        grid.performLayout(getLocalBounds());

    }

    void SetLayerToggleState(unsigned int layerIndex, unsigned int toggleIndex, bool state) {
        auto &but = layerControlStack->layerControlGroups[layerIndex]->toggleControlGroup->controls[toggleIndex];
        but->setToggleState(state, juce::NotificationType::dontSendNotification);
    }

    void SetLayerSelection(unsigned int layerIndex) {
        for (unsigned int i = 0; i < mlp::numLoopLayers; ++i) {
            layerControlStack->layerControlGroups[i]->selectedToggle->setToggleState(layerIndex == i,
                                                                                     juce::NotificationType::dontSendNotification);
        }
    }

    void AddLogLine(unsigned int layerIndex, const std::string &line) {
        layerControlStack->layerControlGroups[layerIndex]->outputLog->AddLine(line);
    }

    void SetOutput(MlpGuiOutput *aOutput) {
        output = aOutput;
    }

    void SetLayerLoopEndFrame(unsigned int aLayerIndex, mlp::frame_t aFrame) {
        layerControlStack->layerControlGroups[aLayerIndex]->loopEndFrame = aFrame;
    }
};
