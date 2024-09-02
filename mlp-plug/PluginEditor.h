#pragma once

#include "PluginProcessor.h"

#include "Mlp.hpp"
#include "MlpGui.hpp"
#include "MlpGuiIo.hpp"

class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);

    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;

    void resized() override;

private:
    /// input to the GUI / output from the processor
    class EditorInput : public MlpGuiInput, public juce::Timer {
        mlp::Mlp &mlp;
        MlpGui &gui;

    public:
        EditorInput(mlp::Mlp &aMlp, MlpGui &aGui) : mlp(aMlp), gui(aGui) {
            startTimerHz(120);
        }

        void timerCallback() override {
            using namespace mlp;

            auto &layerFlagsQ = mlp.GetLayerFlagsQ();
            LayerFlagsMessageData flagsData{};
            while (layerFlagsQ.try_dequeue(flagsData)) {
                std::cout << "layer " << flagsData.layer << ": ";

                for (int i = 0; i < static_cast<int>(LayerOutputFlagId::Count); ++i) {
                    auto flag = static_cast<LayerOutputFlagId>(i);
                    if (flagsData.flags.Test(flag)) {
                        std::cout << LayerOutputFlagLabel[i] << ", ";

                        gui.AddLogLine(flagsData.layer, LayerOutputFlagLabel[i]);

                        switch (flag) {
                            case LayerOutputFlagId::Selected:
                                gui.SetLayerSelection(flagsData.layer);
                                break;
                            case LayerOutputFlagId::Inner:
                                ////  ...etc
                                break;
                            case LayerOutputFlagId::Outer:
                                break;
                            case LayerOutputFlagId::Wrapped:
                                break;
                            case LayerOutputFlagId::Looped:
                                break;
                            case LayerOutputFlagId::Triggered:
                                break;
                            case LayerOutputFlagId::Paused:
                                break;
                            case LayerOutputFlagId::Resumed:
                                break;
                            case LayerOutputFlagId::Reset:
                                break;
                            case LayerOutputFlagId::Restarted:
                                break;
                            case LayerOutputFlagId::Silent:
                                break;
                            case LayerOutputFlagId::Active:
                                break;
                            case LayerOutputFlagId::Stopped:
                                break;
                            case LayerOutputFlagId::Clearing:
                                gui.SetLayerToggleState(flagsData.layer, (unsigned int)mlp::Mlp::IndexBoolParamId::LayerClearEnabled, true);
                                break;
                            case LayerOutputFlagId::NotClearing:
                                gui.SetLayerToggleState(flagsData.layer, (unsigned int)mlp::Mlp::IndexBoolParamId::LayerClearEnabled, false);
                                break;
                            case LayerOutputFlagId::Writing:
                                gui.SetLayerToggleState(flagsData.layer, (unsigned int)mlp::Mlp::IndexBoolParamId::LayerWriteEnabled, true);
                                break;
                            case LayerOutputFlagId::NotWriting:
                                gui.SetLayerToggleState(flagsData.layer, (unsigned int)mlp::Mlp::IndexBoolParamId::LayerWriteEnabled, false);
                                break;
                            case LayerOutputFlagId::Reading:
                                gui.SetLayerToggleState(flagsData.layer, (unsigned int)mlp::Mlp::IndexBoolParamId::LayerReadEnabled, true);
                                break;
                            case LayerOutputFlagId::NotReading:
                                gui.SetLayerToggleState(flagsData.layer, (unsigned int)mlp::Mlp::IndexBoolParamId::LayerReadEnabled, false);
                                break;
                            case LayerOutputFlagId::Opened:
                                break;
                            case LayerOutputFlagId::Closed:
                                gui.SetLayerLoopEndFrame(flagsData.layer, mlp.GetLoopEndFrame((unsigned int)flagsData.layer));
                                break;
                            case LayerOutputFlagId::Count:
                                break;
                            case LayerOutputFlagId::LoopEnabled:
                                gui.SetLayerToggleState(flagsData.layer, (unsigned int)mlp::Mlp::IndexBoolParamId::LayerLoopEnabled, true);
                                break;
                            case LayerOutputFlagId::LoopDisabled:
                                gui.SetLayerToggleState(flagsData.layer, (unsigned int)mlp::Mlp::IndexBoolParamId::LayerLoopEnabled, false);
                                break;
                        }

                    }
                }
                std::cout << std::endl;
            }

            auto &layerPositionQ = mlp.GetLayerPositionQ();
            LayerPositionMessageData posData{};
            while (layerPositionQ.try_dequeue(posData)) {
                /// TODO: show the positions somewhere
//                std::cout << "layer position: " << posData.layer << "; "
//                          << posData.positionRange[0] << " - " << posData.positionRange[1] << std::endl;
            }
        }

    };  // EditorInput

    class EditorOutput: public MlpGuiOutput {
        mlp::Mlp &mlp;

    public:
        explicit EditorOutput(mlp::Mlp &aMlp) : mlp(aMlp) {}

        void SendTap(mlp::Mlp::TapId id) override {
            mlp.Tap(id);
        }

        void SendBool(mlp::Mlp::BoolParamId id, bool value) override {
            mlp.BoolParamChange(id, value);
        }

        void SendIndex(mlp::Mlp::IndexParamId id, unsigned int index) override {
            mlp.IndexParamChange(id, index);
        }

        void SendIndexBool(mlp::Mlp::IndexBoolParamId id, unsigned int index, bool value) override {
            mlp.IndexBoolParamChange(id, index, value);
        }

        void SendIndexFloat(mlp::Mlp::IndexFloatParamId id, unsigned int index, float value) override {
            mlp.IndexFloatParamChange(id, index, value);
        }

    };

private:
    AudioPluginAudioProcessor &processorRef;

    MlpGui mlpGui;
    EditorInput editorInput;
    EditorOutput editorOutput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
