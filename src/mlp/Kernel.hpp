#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "LayerBehavior.hpp"
#include "LoopLayer.hpp"

#include "Outputs.hpp"
#include "Types.hpp"

namespace mlp {

    //==============================================
    //==== core processor class

    class Kernel {

    private:
        typedef LoopLayer<numLoopChannels, bufferFrames> Layer;
        std::array<Layer, numLoopLayers> layer;
        std::array<LayerBehavior, numLoopLayers> layerBehavior;
        std::array<LayerInterface, numLoopLayers> layerInterface;

        // non-interleaved stereo buffers
        typedef std::array<float, bufferFrames * numLoopLayers> LayerBuffer;
        std::array<LayerBuffer, numLoopLayers> buffer{};

        OutputsData *outputs{};

        //----------------------------------------
        //--- other runtime state

        double sampleRate{48000};
        // currently-selected layer index
        unsigned int currentLayer{0};
        // the "innermost" layer will not trigger actions on layers "below" it
        unsigned int innerLayer{0};
        // the "outermost" layer will not be triggered by layers "above" it
        unsigned int outerLayer{0};

        // temp flag
        bool shouldAdvanceLayerOnNextTap{false};

        /// global interface flags (not governed by layer behaviors)
        bool clearLayerOnStop{true};
        bool clearLayerOnSet{true};
        bool advanceLayerOnLoopOpen{true};
    public:

        Kernel() {
            /// initialize buffers
            /// NB: not the most efficient design, but it is simple:
            /// we are giving each layer its own full-sized buffer, which it's unlikely to use completely
            /// we could use a single large buffer, and set a pointer into it for each layer when a loop is opened
            /// the difficulty there becomes how to reclaim segmented buffer space,
            /// when layers can be stopped/cleared in any order
            for (unsigned int i = 0; i < numLoopLayers; ++i) {
                buffer[i].fill(0.f);
                layer[i].buffer = buffer[i].data();
            }
            /// initialize interfaces
            for (unsigned int i = 0; i < numLoopLayers; ++i) {
                layerInterface[i].SetLayer(&layer[i]);
            }
            /// initialize behaviors
            for (unsigned int i = 0; i < numLoopLayers; ++i) {
                layerBehavior[i].thisLayer = &layerInterface[i];
                layerBehavior[i].layerBelow = &layerInterface[(i - 1) % numLoopLayers];
                layerBehavior[i].layerAbove = &layerInterface[(i + 1) % numLoopLayers];
            }
            /// initialize modes
            for (unsigned int i = 0; i < numLoopLayers; ++i) {
                SetLayerBehaviorMode(layerBehavior[i], LayerBehaviorModeId::MULTIPLY_UNQUANTIZED);
            }

            SetInnerLayer(0);
            SetOuterLayer(0);
        }

        void SetSampleRate(float aSampleRate) {
            sampleRate = aSampleRate;
        }

        // process a single *stereo interleaved* audio frame
        void ProcessFrame(const float *&src, float *&dst) {
            float x[2];
            float y[2]{0.f};
            x[0] = *src++;
            x[1] = *src++;
            for (unsigned int i = 0; i < numLoopLayers; ++i) {
                auto phaseUpdateResult = layer[i].ProcessFrame(x, y);
                if (phaseUpdateResult.Test(PhasorAdvanceResultFlag::WRAPPED_LOOP)) {
                    layerBehavior[i].ProcessCondition(LayerConditionId::Wrap);
                    SetOutputLayerFlag(i, LayerOutputFlagId::Wrapped);
                }
                if (phaseUpdateResult.Test(PhasorAdvanceResultFlag::CROSSED_TRIGGER)) {
                    layerBehavior[i].ProcessCondition(LayerConditionId::Trigger);
                    SetOutputLayerFlag(i, LayerOutputFlagId::Triggered);
                }
                if (phaseUpdateResult.Test(PhasorAdvanceResultFlag::DONE_FADEOUT)) {
                    SetOutputLayerFlag(i, LayerOutputFlagId::Silent);
                }
            }
            *dst++ = y[0];
            *dst++ = y[1];
        }

        void SetOutputLayerFlag(unsigned int layerIndex, LayerOutputFlagId flag) {
            if (outputs) {
                outputs->layers[layerIndex].flags.Set(flag);
            }
        }

        void InitializeOutputs(OutputsData *aOutputs) {
            outputs = aOutputs;
            *outputs = defaultOutputsData;
            for (unsigned int i = 0; i < numLoopLayers; ++i) {
                outputs->layers[i].positionRange[0] = layer[i].GetCurrentFrame();
                layerInterface[i].outputs = &outputs->layers[i];
                layer[i].outputs = &outputs->layers[i];
            }
        }

        void FinalizeOutputs() {
            if (outputs == nullptr) {
                return;
            }
            for (unsigned int i = 0; i < numLoopLayers; ++i) {
                outputs->layers[i].positionRange[1] = layer[i].GetCurrentFrame();
            }
        }

        void SetInnerLayer(unsigned int aLayerIndex) {
            std::cout << "setting inner layer: " << aLayerIndex << std::endl;
            layerInterface[innerLayer].isInner = false;
            innerLayer = aLayerIndex;
            layerInterface[innerLayer].isInner = true;
            SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Inner);
        }

        void SetOuterLayer(unsigned int aLayerIndex) {
            std::cout << "setting outer layer: " << aLayerIndex << std::endl;
            layerInterface[outerLayer].isOuter = false;
            outerLayer = aLayerIndex;
            layerInterface[outerLayer].isOuter = true;
            SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Outer);
        }

        //------------------------------------------------
        //-- control

        // first action: opens/closes the loop, advances the layer
        void SetLoopTap() {
            switch (layer[currentLayer].state) {
                case LoopLayerState::STOPPED:
                case LoopLayerState::LOOPING:
                    if (advanceLayerOnLoopOpen && shouldAdvanceLayerOnNextTap) {
                        //std::cout << "SetLoopTap(): advancing layer; current layer = " << currentLayer << std::endl;
                        if (currentLayer >= (numLoopLayers - 1)) {
                            SetCurrentLayer(0);
                        } else {
                            SetCurrentLayer(currentLayer + 1);
                        }
                        shouldAdvanceLayerOnNextTap = false;
                    }
                    if (currentLayer >= numLoopLayers) {
                        return;
                    }

                    std::cout << "TapLoop(): opening loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].OpenLoop();
                    SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Writing);
                    SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Opened);
                    layerBehavior[currentLayer].ProcessCondition(LayerConditionId::OpenLoop);
                    if (clearLayerOnSet) {
                        layer[currentLayer].SetClear(true);
                        SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Clearing);
                    }
                    shouldAdvanceLayerOnNextTap = false;
                    break;

                case LoopLayerState::SETTING:
                    std::cout << "TapLoop(): closing loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].CloseLoop(true, false);
                    layerBehavior[currentLayer].ProcessCondition(LayerConditionId::CloseLoop);
                    shouldAdvanceLayerOnNextTap = true;
                    SetOuterLayer(currentLayer);
                    SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Closed);
                    SetOutputLayerFlag(currentLayer, LayerOutputFlagId::NotWriting);
                    SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Reading);
                    break;
            }
        }

        void ToggleOverdub() {
            if (currentLayer >= 0 && currentLayer < numLoopLayers)
                layer[currentLayer].ToggleWrite();
        }

        void ToggleMute() {
            if (currentLayer >= 0 && currentLayer < numLoopLayers)
                layer[currentLayer].ToggleRead();
        }

        void SetLayerWrite(unsigned int layerIndex, bool value) {
            assert(layerIndex >= 0 && layerIndex < numLoopLayers);
            layer[layerIndex].SetWrite(value);
        }

        void SetLayerClear(unsigned int layerIndex, bool value) {
            assert(layerIndex >= 0 && layerIndex < numLoopLayers);
            layer[layerIndex].SetClear(value);
        }

        void SetLayerRead(unsigned int layerIndex, bool value) {
            assert(layerIndex >= 0 && layerIndex < numLoopLayers);
            layer[layerIndex].SetRead(value);
        }

        void SetWrite(bool value) {
            SetLayerWrite(currentLayer, value);
        }

        void SetClear(bool value) {
            SetLayerClear(currentLayer, value);
        }

        void SetRead(bool value) {
            SetLayerRead(currentLayer, value);
        }

        void StopLoop() {
            assert(currentLayer >= 0 && currentLayer < numLoopLayers);
            // std::cout << "(stopping current layer)" << std::endl;
            layer[currentLayer].Stop();
            SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Stopped);
            if (clearLayerOnStop) {
                //layer[currentLayer].preserveLevel = 0.f;
                layer[currentLayer].clearSwitch.Open();
                SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Clearing);
            }
            bool anyLayersActive = false;
            for (auto &theLayer: layer) {
                anyLayersActive |= theLayer.GetIsActive();
            }
            if (!anyLayersActive) {
                std::cout << "(no layers active; selection is inner)" << std::endl;
                SetInnerLayer(currentLayer);
            } else {
                std::cout << "(decrementing selection)" << std::endl;
                if (currentLayer == 0) {
                    SetCurrentLayer(numLoopLayers - 1);
                } else {
                    SetCurrentLayer(currentLayer - 1);
                }
                if (advanceLayerOnLoopOpen) {
                    shouldAdvanceLayerOnNextTap = true;
                }
            }
            SetOuterLayer(currentLayer);
            std::cout << "stopped layer; current = " << currentLayer << std::endl;
        }

        void SetPreserveLevel(float level, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            layer[layerIndex].preserveLevel = level;
        }

        void SetRecordLevel(float level, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            layer[layerIndex].recordLevel = level;
        }

        void SetPlaybackLevel(float level, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            layer[layerIndex].playbackLevel = level;
        }

        void SetCurrentLayer(unsigned int layerIndex) {
            currentLayer = layerIndex;
            SetOutputLayerFlag(currentLayer, LayerOutputFlagId::Selected);
        }

        void SetLoopStartFrame(frame_t frame, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            layer[layerIndex].loopStartFrame = frame;
            if (layer[layerIndex].resetFrame < layer[layerIndex].loopStartFrame) {
                layer[layerIndex].resetFrame = layer[layerIndex].loopStartFrame;
            }
        }

        void SetLoopEndFrame(frame_t frame, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            std::cout << "SetLoopEndFrame(): layer = " << layerIndex << "; frame = " << frame << std::endl;
            layer[layerIndex].loopEndFrame = frame;
            if (layer[layerIndex].resetFrame > layer[layerIndex].loopEndFrame) {
                layer[layerIndex].resetFrame = layer[layerIndex].loopEndFrame;
            }
        }

        void SetLoopResetFrame(frame_t frame, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            layer[layerIndex].resetFrame = frame;
            if (layer[layerIndex].resetFrame > layer[layerIndex].loopEndFrame) {
                layer[layerIndex].resetFrame = layer[layerIndex].loopEndFrame;
            }
        }

        void SetFadeTime(float aSeconds, int aLayerIndex = -1) {
            SetFadeIncrement(static_cast<float>(1.0 / (aSeconds * sampleRate)), aLayerIndex);
        }

        void SetSwitchTime(float aSeconds, int aLayerIndex = -1) {
            SetSwitchIncrement(static_cast<float>(1.0 / (aSeconds * sampleRate)), aLayerIndex);
        }

        void SetFadeIncrement(float increment, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            layer[layerIndex].SetFadeIncrement(increment);
        }

        void SetSwitchIncrement(float increment, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            layer[layerIndex].SetSwitchIncrement(increment);
        }


        void SetLoopEnabled(bool enabled, int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            layer[layerIndex].loopEnabled = enabled;
        }

        void SetClearOnStop(bool clear) {
            clearLayerOnStop = clear;
        }

        void SetClearOnSet(bool clear) {
            clearLayerOnSet = clear;
        }


        void SetWriteOnSet(bool write) {
            (void) write;
            //....TODO
        }

        void SetReadOnSet(bool read) {
            (void) read;
            //....TODO
        }

        void SetAdvanceLayerOnLoopOpen(bool advance) {
            advanceLayerOnLoopOpen = advance;
        }

        void ResetLayer(int aLayerIndex = -1) {
            unsigned int layerIndex = aLayerIndex < 0 ? currentLayer : (unsigned int) aLayerIndex;
            if (layerIndex >= 0 && layerIndex < numLoopLayers) {
                layer[layerIndex].Reset();
                SetOutputLayerFlag(layerIndex, LayerOutputFlagId::Reset);
            }
        }

        void SetMode(LayerBehaviorModeId mode) {
            /// set for all layers!
            for (unsigned int i = 0; i < numLoopLayers; ++i) {
                SetLayerBehaviorMode(layerBehavior[i], mode);
            }
        }
    };
}