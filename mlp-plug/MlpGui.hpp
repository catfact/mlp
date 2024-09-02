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

    static constexpr int buttonHeight = 32;
    static constexpr int tapButtonWidth = buttonHeight * 2;
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
        static constexpr int numLines = 16;
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

            auto addModeBut = [this](const std::string &label) {
                auto but = std::make_unique<juce::TextButton>(label);
                but->setRadioGroupId(1);
                but->setClickingTogglesState(true);
                but->setToggleable(true);
                addAndMakeVisible(but.get());
                modeButtons.push_back(std::move(but));
            };

            addModeBut("ASYNC");
            addModeBut("MULTIPLY");
            addModeBut("INSERT");

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
            grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth / 2)));

            for (unsigned int i = 0; i < numTaps; ++i) {
                grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth)));
            };

            // spacer
            grid.templateColumns.add(juce::Grid::TrackInfo(juce::Grid::Px(tapButtonWidth / 2)));

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
        mlp::frame_t initialPosition{0};
        mlp::frame_t finalPosition{0};

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
        struct LayerModeControlGroup
                : public LayerWidgetControlGroup<juce::TextButton, (size_t) mlp::LayerBehaviorModeId::COUNT> {
            explicit LayerModeControlGroup(int layerIndex) {
                for (int i = 0; i < (int) mlp::LayerBehaviorModeId::COUNT; ++i) {
                    controls.push_back(std::make_unique<juce::TextButton>(mlp::LayerBehaviorModeIdLabel[i]));
                    controls.back()->setRadioGroupId(layerIndex + 1);
                    controls.back()->setToggleable(true);
                    controls.back()->setClickingTogglesState(true);
                    controls.back()->onClick = [layerIndex, i] {
                        // std::cout << "LayerModeControlGroup::onClick; layer = " << layerIndex << ", mode = " << i << std::endl;
                        if (output) {
                            output->SendIndexIndex(mlp::Mlp::IndexIndexParamId::LayerMode,
                                                   static_cast<unsigned int>(layerIndex), static_cast<unsigned int>(i));
                        }
                    };
                    addAndMakeVisible(controls.back().get());
                }
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

        struct LayerPositionDisplay : juce::Component {
            // unit-scaled positions
            float startPos{}, endPos{};

            void paint(juce::Graphics &g) override {
                g.fillAll(juce::Colours::black);

                const float ins = 2.f;
                auto ww = static_cast<float>(getWidth()) - (ins*2);
                auto hh = static_cast<float>(getHeight()) - (ins*2);
                float x, w;
                if (startPos < endPos) {

                    g.setColour(juce::Colours::grey);

                    x = startPos * ww;
                    x = std::max(0.f, std::min(x, ww - 1));
                    w = (endPos - startPos) * ww;
                    w = std::max(1.f, std::min(ww - x - 1, w));
                    //w = std::min(1.f - x, w);
                    g.fillRect(x + ins, ins, w, hh-4);
                } else {
                    // fill from zero to start
                    x = 0;
                    w = startPos * ww;
                    w = std::max(1.f, std::min(ww - x - 1, w));
                    g.fillRect(ins + x,  ins, w, hh);
                    // fill from end to one
                    x = endPos * ww;
                    x = std::max(0.f, std::min(x, ww - 1));
                    w = (1.f - endPos) * ww;
                    w = std::max(1.f, std::min(ww - x - 1, w));
                    g.fillRect(x + ins, ins, w, (float) hh);
                }
            }

        };

        std::unique_ptr<juce::TextButton> selectedToggle;
        std::unique_ptr<LayerModeControlGroup> modeControlGroup;
        std::unique_ptr<LayerToggleControlGroup> toggleControlGroup;
        std::unique_ptr<LayerParameterControlGroup> parameterControlGroup;
        std::unique_ptr<LayerPositionDisplay> positionDisplay;
        std::unique_ptr<LayerOutputLog> outputLog;

    public:
        explicit LayerControlGroup(int layerIndex) {
            selectedToggle = std::make_unique<juce::TextButton>("SELECTED");
            selectedToggle->setToggleable(true);
            // FIXME: for now, we don't have explicit layer selection!
            selectedToggle->setClickingTogglesState(false);
            modeControlGroup = std::make_unique<LayerModeControlGroup>(layerIndex);
            toggleControlGroup = std::make_unique<LayerToggleControlGroup>(layerIndex);
            parameterControlGroup = std::make_unique<LayerParameterControlGroup>(layerIndex);
            positionDisplay = std::make_unique<LayerPositionDisplay>();
            outputLog = std::make_unique<LayerOutputLog>();

            addAndMakeVisible(selectedToggle.get());
            addAndMakeVisible(modeControlGroup.get());
            addAndMakeVisible(toggleControlGroup.get());
            addAndMakeVisible(parameterControlGroup.get());
            addAndMakeVisible(positionDisplay.get());
            addAndMakeVisible(outputLog.get());
        }

        void SetLoopEndFrame(mlp::frame_t aFrame) {
            loopEndFrame = aFrame;
            if (positionDisplay) {
                positionDisplay->startPos = (float) initialPosition / (float) loopEndFrame;
                positionDisplay->endPos = (float) finalPosition / (float) loopEndFrame;
                positionDisplay->repaint();
            }
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
                    Track(Px(buttonHeight)),
                    Track(Fr(1)),
                    Track(Px(buttonHeight / 2)),
                    Track(Fr(1)),
            };

            grid.items.add(juce::GridItem(*selectedToggle));
            grid.items.add(juce::GridItem(*modeControlGroup));
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

        /// feels a bit hacky to define this here,
        /// but need visibility on various branches of the hierarchy at once
        /// make global mode buttons set all layer mode buttons
        for (unsigned int globalMode = 0; globalMode < (unsigned int) mlp::LayerBehaviorModeId::COUNT; ++globalMode) {
            auto &but = globalControlGroup->modeButtons[globalMode];
            //
            but->onClick = [globalMode, this] {
                if (output) {
                    output->SendIndex(mlp::Mlp::IndexParamId::Mode, globalMode);
                }

                for (unsigned int layer = 0; layer < mlp::numLoopLayers; ++layer) {
                    auto &modeControlGroup = layerControlStack->layerControlGroups[layer]->modeControlGroup;
                    for (unsigned int localMode = 0;
                         localMode < (unsigned int) mlp::LayerBehaviorModeId::COUNT; ++localMode) {
                        modeControlGroup->controls[localMode]->setToggleState(localMode == globalMode,
                                                                               juce::NotificationType::dontSendNotification);
                    }
                }
            };

        }
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

    void SetLayerLoopEndFrame(unsigned int layerIndex, mlp::frame_t frame) {
        layerControlStack->layerControlGroups[layerIndex]->SetLoopEndFrame(frame);
    }

    void SetLayerPosition(unsigned int layerIndex, mlp::frame_t start, mlp::frame_t end) {
        auto &layer = layerControlStack->layerControlGroups[layerIndex];
        //std::cout << "setLayerPosition: layer " << layerIndex << ", start = " << start << ", end = " << end << std::endl;
        layer->initialPosition = start;
        layer->finalPosition = end;
        layer->positionDisplay->startPos = (float) start / (float) layer->loopEndFrame;
        layer->positionDisplay->endPos = (float) end / (float) layer->loopEndFrame;
        layer->positionDisplay->repaint();
    }
};
