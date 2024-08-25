#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "LayerBehavior.hpp"
#include "LoopLayer.hpp"
#include "Types.hpp"

namespace mlp {

    //==============================================
    //==== core processor class

    class Kernel {

    public:
        static constexpr unsigned int numChannels = 2;
        static constexpr unsigned int numLayers = 4;

    private:
        typedef LoopLayer<numChannels, bufferFrames> Layer;
        std::array<Layer, numLayers> layer;
        std::array<LayerBehavior, numLayers> behavior;
        std::array<LayerInterface, numLayers> layerInterface;

        //--------------
        // currently-selected layer index
        unsigned int currentLayer{0};
        // the "innermost" layer doesn't perform actions on the layer "below" it
        // when all layers are stopped, the next activated layer becomes the innermost
        unsigned int innerLayer{0};

        // non-interleaved stereo buffers
        typedef std::array<float, bufferFrames * numChannels> LayerBuffer;
        std::array<LayerBuffer, numLayers> buffer{};

        bool shouldAdvanceLayerOnNextTap{false};

        /// global interface flags (not governed by layer behaviors)
        bool clearLayerOnStop{true};
        bool clearLayerOnSet{true};

        bool advanceLayerOnLoopOpen{true};

    private:
        /// FIXME: just store this, duh
        bool IsOuterLayer(unsigned int i) {
            return i == (innerLayer + 1) % numLayers;
        }

    public:

        Kernel() {
            /// initialize buffers
            /// FIXME: not the most efficient design, but it is simple:
            /// we are giving each layer its own full-sized buffer, which its unlikely to use
            /// we could use a single large buffer, and set a pointer into it for each layer when a loop is opened
            /// the difficulty there becomes how to reclaim segmented buffer space,
            /// when layers can be stopped/cleared in any order
            for (unsigned int i = 0; i < numLayers; ++i) {
                buffer[i].fill(0.f);
                layer[i].buffer = buffer[i].data();
            }
            /// initialize interfaces
            for (unsigned int i = 0; i < numLayers; ++i) {
                layerInterface[i].SetLayer(&layer[i]);
            }
            /// initialize behaviors
            for (unsigned int i = 0; i < numLayers; ++i) {
                behavior[i].thisLayer = &layerInterface[i];
                behavior[i].layerBelow = &layerInterface[(i - 1) % numLayers];
                behavior[i].layerAbove = &layerInterface[(i + 2) % numLayers];
            }
            /// initialize modes
            for (unsigned int i = 0; i < numLayers; ++i) {
                SetLayerBehaviorMode(behavior[i], LayerBehaviorModeId::MULTIPLY_UNQUANTIZED);
            }
        }

        // process a single *stereo interleaved* audio frame
        void ProcessFrame(const float *&src, float *&dst) {
            float x[2];
            float y[2]{0.f};
            x[0] = *src++;
            x[1] = *src++;
            for (unsigned int i = 0; i < numLayers; ++i) {
                auto phaseUpdateResult = layer[i].ProcessFrame(x, y);
                if (phaseUpdateResult.Test(PhasorAdvanceResultFlag::WRAPPED_LOOP)) {
                    behavior[i].ProcessCondition(LayerConditionId::Wrap, i == innerLayer, IsOuterLayer(i));
                }
                if (phaseUpdateResult.Test(PhasorAdvanceResultFlag::CROSSED_TRIGGER)) {
                    behavior[i].ProcessCondition(LayerConditionId::Trigger, i == innerLayer, IsOuterLayer(i));
                }
            }
            *dst++ = y[0];
            *dst++ = y[1];
        }

        //------------------------------------------------
        //-- control

        // first action: opens/closes the loop, advances the layer
        void SetLoopTap() {


            switch (layer[currentLayer].state) {
                case LoopLayerState::STOPPED:
                case LoopLayerState::LOOPING:
                    if (advanceLayerOnLoopOpen && shouldAdvanceLayerOnNextTap) {
                        std::cout << "SetLoopTap(): advancing layer; current layer = " << currentLayer << std::endl;
                        if (++currentLayer >= numLayers) {
                            std::cout << "SetLoopTap(): reached end of layers; wrapping to first" << std::endl;
                            currentLayer = 0;
                        } else {
                            std::cout << "SetLoopTap(): current layer now = " << currentLayer << std::endl;
                        }
                        shouldAdvanceLayerOnNextTap = false;
                    }
                    if (currentLayer >= numLayers) {
                        return;
                    }

                    std::cout << "TapLoop(): opening loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].OpenLoop();
                    behavior[currentLayer].ProcessCondition(LayerConditionId::OpenLoop, currentLayer == innerLayer,
                                                            IsOuterLayer(currentLayer));
                    if (clearLayerOnSet) {
                        layer[currentLayer].preserveLevel = 0.f;
                    }
                    shouldAdvanceLayerOnNextTap = false;
                    break;

                case LoopLayerState::SETTING:
                    std::cout << "TapLoop(): closing loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].CloseLoop();
                    behavior[currentLayer].ProcessCondition(LayerConditionId::CloseLoop, currentLayer == innerLayer,
                                                            IsOuterLayer(currentLayer));
                    shouldAdvanceLayerOnNextTap = true;
                    break;
            }
        }

        void ToggleOverdub() {
            if (currentLayer >= 0 && currentLayer < numLayers)
                layer[currentLayer].ToggleWrite();
        }


        void ToggleMute() {
            if (currentLayer >= 0 && currentLayer < numLayers)
                layer[currentLayer].ToggleRead();
        }

        void SetLayerWrite(unsigned int layerIndex, bool value) {
            assert(layerIndex >= 0 && layerIndex < numLayers);
            layer[layerIndex].SetWrite(value);
        }

        void SetLayerClear(unsigned int layerIndex, bool value) {
            assert(layerIndex >= 0 && layerIndex < numLayers);
            layer[layerIndex].SetClear(value);
        }

        void SetLayerRead(unsigned int layerIndex, bool value) {
            assert(layerIndex >= 0 && layerIndex < numLayers);
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
            assert(currentLayer >= 0 && currentLayer < numLayers);
            // std::cout << "(stopping current layer)" << std::endl;
            layer[currentLayer].Stop();
            if (clearLayerOnStop) {
                layer[currentLayer].preserveLevel = 0.f;
            }

            bool anyLayersActive = false;
            for (auto &theLayer: layer) {
                anyLayersActive |= theLayer.GetIsActive();
            }
            if (!anyLayersActive) {
                std::cout << "(resetting inner layer)" << std::endl;
                innerLayer = currentLayer;
            } else {
                std::cout << "(decrementing selection)" << std::endl;
                if (--currentLayer < 0) {
                    currentLayer = numLayers - 1;
                }
            }
            std::cout << "stopped layer; current = " << currentLayer << std::endl;
            // just making sure...
            shouldAdvanceLayerOnNextTap = false;
        }

        void SetPreserveLevel(float level, int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            layer[layerIndex].preserveLevel = level;
        }

        void SetRecordLevel(float level, int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            layer[layerIndex].recordLevel = level;
        }

        void SetPlaybackLevel(float level, int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            layer[layerIndex].playbackLevel = level;
        }

        void SelectLayer(int layerIndex) {
            currentLayer = (unsigned int)layerIndex;
        }

        void SetLoopStartFrame(frame_t frame, int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            layer[layerIndex].loopStartFrame = frame;
            if (layer[layerIndex].resetFrame < layer[layerIndex].loopStartFrame) {
                layer[layerIndex].resetFrame = layer[layerIndex].loopStartFrame;
            }
        }

        void SetLoopEndFrame(frame_t frame, int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            std::cout << "SetLoopEndFrame(): layer = " << layerIndex << "; frame = " << frame << std::endl;
            layer[layerIndex].loopEndFrame = frame;
            if (layer[layerIndex].resetFrame > layer[layerIndex].loopEndFrame) {
                layer[layerIndex].resetFrame = layer[layerIndex].loopEndFrame;
            }

        }

        void SetLoopResetFrame(frame_t frame, int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            layer[layerIndex].resetFrame = frame;
            if (layer[layerIndex].resetFrame > layer[layerIndex].loopEndFrame) {
                layer[layerIndex].resetFrame = layer[layerIndex].loopEndFrame;
            }

        }

        void SetFadeIncrement(float increment, int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            layer[layerIndex].SetFadeIncrement(increment);
        }

        void SetLoopEnabled(bool enabled, int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            layer[layerIndex].loopEnabled = enabled;
        }

        void SetClearOnStop(bool clear) {
            clearLayerOnStop = clear;
        }

        void SetClearOnSet(bool clear) {
            clearLayerOnSet = clear;
        }

        void SetAdvanceLayerOnLoopOpen(bool advance) {
            advanceLayerOnLoopOpen = advance;
        }

        void ResetLayer(int aLayerIndex = -1) {
            unsigned int layerIndex;
            if (aLayerIndex < 0) {
                layerIndex = currentLayer;
            } else { layerIndex = (unsigned int) aLayerIndex; }
            if (layerIndex >= 0 && layerIndex < numLayers) {
                layer[layerIndex].Reset();
            }
        }
    };
}