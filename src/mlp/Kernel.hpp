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
        std::array<LayerInterface, numLayers> interface;
        // std::array<LayerConditionState, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditions;

        // currently-selected layer index
        int currentLayer{0};
        // the "innermost" layer doesn't perform actions on the layer "below" it
        // when all layers are stopped, the next activated layer becomes the innermost
        int innerLayer{0};

        // non-interleaved stereo buffers
        typedef std::array<float, bufferFrames * numChannels> LayerBuffer;
        std::array<LayerBuffer, numLayers> buffer{};

        bool advanceLayerOnNextTap{false};

        /// mode/behavior flags
        /// FIXME: maybe encapsulate these binary settings, just to reduce clutter
        bool clearLayerOnStop{true};
        bool clearLayerOnSet{true};
        bool advanceLayerOnLoopOpen{true};

    private:
        bool IsOuterLayer(int i) {
            /// FIXME: woops, this doesn't account for the actual number of active layers`
            return (innerLayer - i) % numLayers == 1;
        }
    public:

        Kernel() {
            /// initialize buffers
            /// FIXME: not the most efficient design, but it is simple:
            /// we are giving each layer its own full-sized buffer, which its unlikely to use
            /// we could use a single large buffer, and set a pointer into it for each layer when a loop is opened
            /// the difficulty there becomes how to reclaim segmented buffer space,
            /// when layers can be stopped/cleared in any order
            for (int i = 0; i < numLayers; ++i) {
                buffer[i].fill(0.f);
                layer[i].buffer = buffer[i].data();
            }
            /// initialize interfaces
            for (int i = 0; i < numLayers; ++i) {
                interface[i].SetLayer(&layer[i]);
            }
            /// initialize behaviors
            for (int i = 0; i < numLayers; ++i) {
                behavior[i].thisLayer = &interface[i];
                behavior[i].layerBelow = &interface[(i - 1) % numLayers];
                behavior[i].layerAbove = &interface[(i + 2) % numLayers];
            }
            /// initialize modes
            for (int i = 0; i < numLayers; ++i) {
                SetLayerBehaviorMode(behavior[i], LayerBehaviorModeId::MULTIPLY_UNQUANTIZED);
            }
        }

        // process a single *stereo interleaved* audio frame
        void ProcessFrame(const float *&src, float *&dst) {
            float x[2];
            float y[2]{0.f};
            x[0] = *src++;
            x[1] = *src++;
            for (int i = 0; i < numLayers; ++i) {
                auto phaseUpdateResult = layer[i].ProcessFrame(x, y);
                auto &face = interface[i];
                if (phaseUpdateResult.Test(PhasorAdvanceResultFlag::WRAPPED_LOOP)) {
                    behavior[i].ProcessCondition(LayerConditionId::Wrap, i == innerLayer, IsOuterLayer(i));
                }
                if (phaseUpdateResult.Test(PhasorAdvanceResultFlag::CROSSED_TRIGGER)) {
                    behavior[i].ProcessCondition(LayerConditionId::Trigger, i == innerLayer, IsOuterLayer(i));
                }
//                if (PhasorAdvanceResultFlag::WRAPPED_LOOP) {
//                    // the loop has wrapped around (next frame will fall at loop start)
////                    /// TODO: different behaviors/modes are possible here
//                    if (layer[i].syncLastLayer && (i != innerLayer)) {
//                        int layerBelow = i > 0 ? i - 1 : (int) numLayers - 1;
//                        layer[layerBelow].Reset();
//                    }
//                }
//                if (phaseUpdateResult.Test(PhasorAdvanceResultFlag::CROSSED_TRIGGER)) {
//                    // the loop has reached the trigger frame
////                    /// TODO: different behaviors/modes are possible here
//                }

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
                    if (advanceLayerOnLoopOpen && advanceLayerOnNextTap) {
                        std::cout << "SetLoopTap(): advancing layer; current layer = " << currentLayer << std::endl;
                        if (++currentLayer >= numLayers) {
                            std::cout << "SetLoopTap(): reached end of layers; wrapping to first" << std::endl;
                            currentLayer = 0;
                        } else {
                            std::cout << "SetLoopTap(): current layer now = " << currentLayer << std::endl;
                        }
                        advanceLayerOnNextTap = false;
                    }
                    if (currentLayer >= numLayers) {
                        return;
                    }

                    std::cout << "TapLoop(): opening loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].OpenLoop();
                    behavior[currentLayer].ProcessCondition(LayerConditionId::OpenLoop, currentLayer == innerLayer,
                                                            IsOuterLayer(currentLayer));
//                    if (currentLayer != innerLayer) {
//                        int layerBelow = currentLayer > 0 ? currentLayer - 1 : (int) numLayers - 1;
//                        layer[layerBelow].resetFrame = layer[layerBelow].GetCurrentFrame();
//                    }
                    if (clearLayerOnSet) {
                        layer[currentLayer].preserveLevel = 0.f;
                    }
                    advanceLayerOnNextTap = false;
                    break;

                case LoopLayerState::SETTING:
                    std::cout << "TapLoop(): closing loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].CloseLoop();
                    behavior[currentLayer].ProcessCondition(LayerConditionId::CloseLoop, currentLayer == innerLayer,
                    IsOuterLayer(currentLayer));
//                    if (currentLayer != innerLayer) {
//                        int layerBelow = currentLayer > 0 ? currentLayer - 1 : (int) numLayers - 1;
//                        layer[layerBelow].Reset();
//                    }
                    advanceLayerOnNextTap = true;
                    break;
//                case LoopLayerState::LOOPING:
                    // std::cout << "TapLoop(): advancing layer; current = " << currentLayer << std::endl;
                    // advance to the next layer and start over
//                    currentLayer;
//                    SetLoopTap();
//                    break;
            }
        }

        void ToggleOverdub() {
            // std::cout << "ToggleOverdub(): layer = " << currentLayer << std::endl;
            if (currentLayer >= 0 && currentLayer < numLayers)
                layer[currentLayer].ToggleWrite();
        }


        void ToggleMute() {
            // std::cout << "ToggleMute(): layer = " << currentLayer << std::endl;
            if (currentLayer >= 0 && currentLayer < numLayers)
                layer[currentLayer].ToggleRead();
        }

        void StopLoop() {
            assert(currentLayer >= 0 && currentLayer < numLayers);

            /// FIXME: actually we probably don't need any of these index-in-bounds checks anymore
            if (currentLayer >= 0) {
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
            }

            // just making sure...
            advanceLayerOnNextTap = false;
        }

        void SetPreserveLevel(float level, int layerIndex = -1) {
            if (layerIndex < 0) {
                layerIndex = currentLayer;
            }
            layer[layerIndex].preserveLevel = level;
        }

        void SetRecordLevel(float level, int layerIndex = -1) {
            if (layerIndex < 0) {
                layerIndex = currentLayer;
            }
            layer[layerIndex].recordLevel = level;
        }


        void SetPlaybackLevel(float level, int layerIndex = -1) {
            if (layerIndex < 0) {
                layerIndex = currentLayer;
            }
            layer[layerIndex].playbackLevel = level;
        }

        void SelectLayer(int layerIndex) {
            currentLayer = layerIndex;
        }

        void SetLoopStartFrame(frame_t start) {
            if (currentLayer >= 0 && currentLayer < numLayers) {
                layer[currentLayer].loopStartFrame = start;
                if (layer[currentLayer].resetFrame < layer[currentLayer].loopStartFrame) {
                    layer[currentLayer].resetFrame = layer[currentLayer].loopStartFrame;
                }
            }
        }

        void SetLoopEndFrame(frame_t end) {
            std::cout << "SetLoopEndFrame(): layer = " << currentLayer << "; frame = " << end << std::endl;
            if (currentLayer >= 0 && currentLayer < numLayers) {
                layer[currentLayer].loopEndFrame = end;
                if (layer[currentLayer].resetFrame > layer[currentLayer].loopEndFrame) {
                    layer[currentLayer].resetFrame = layer[currentLayer].loopEndFrame;
                }
            }
        }

        void SetLoopResetFrame(frame_t offset) {
            if (currentLayer >= 0 && currentLayer < numLayers) {
                layer[currentLayer].resetFrame = offset;
            }
        }

        void SetSyncLastLayer(bool sync) {
            if (currentLayer >= 0 && currentLayer < numLayers) {
                layer[currentLayer].syncLastLayer = sync;
            }
        }

        void SetFadeIncrement(float increment) {
            if (currentLayer >= 0 && currentLayer < numLayers) {
                layer[currentLayer].SetFadeIncrement(increment);
            }
        }

        void SetLoopEnabled(bool enabled) {
            if (currentLayer >= 0 && currentLayer < numLayers) {
                layer[currentLayer].loopEnabled = enabled;
            }
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

        void ResetCurrent() {
            if (currentLayer >= 0 && currentLayer < numLayers) {
                layer[currentLayer].Reset();
            }
        }

        void ResetLayer(int layerIndex) {
            if (layerIndex >= 0 && layerIndex < numLayers) {
                layer[layerIndex].Reset();
            }
        }
    };
}