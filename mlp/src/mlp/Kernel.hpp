#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "LoopLayer.hpp"
#include "Types.hpp"

namespace mlp {

    //==============================================
    //==== core processor class

    class Kernel {

    public:
        static constexpr frame_t bufferFrames = 1 << 25;
        static constexpr unsigned int numChannels = 2;
        static constexpr unsigned int numLayers = 4;

    private:
        typedef LoopLayer<numChannels, bufferFrames> Layer;
        std::array<Layer, numLayers> layer;

        int currentLayer{0};

        // non-interleaved stereo buffers
        typedef std::array<float, bufferFrames * numChannels> LayerBuffer;
        std::array<LayerBuffer, numLayers> buffer{};

        frame_t lastLayerPositionAtLoopStart;
        bool advanceLayerOnNextTap{false};

    public:

        Kernel() {
            /// FIXME: not an efficient design
            /// we're giving each layer a full audio buffer, sized to maximum loop length
            /// we *should* use one long buffer,
            /// and dynamically assign pointers into it as loops are set
            for (int i = 0; i < numLayers; ++i) {
                buffer[i].fill(0.f);
                layer[i].buffer = buffer[i].data();
            }
        }

        // process a single *stereo interleaved* audio frame
        void ProcessFrame(const float *&src, float *&dst) {
            float x[2];
            float y[2]{0.f};
            x[0] = *src++;
            x[1] = *src++;
            for (int i = 0; i < numLayers; ++i) {
                if (layer[i].ProcessFrame(x, y)) {
                    // the loop has wrapped around (next frame will fall at loop start)
                    /// TODO: different behaviors/modes are possible here
                    /// for now: each layer after the first optionally resets the layer below it
                    /// this means that the most recent loop is effectively the "leader,"
                    /// determining the period of all loops
                    if (i > 0) {
                        if (layer[i].syncLastLayer) {
                            layer[i - 1].Reset();
                        }
                    }
                }
            }
            *dst++ = y[0];
            *dst++ = y[1];
        }

        //------------------------------------------------
        //-- control

        // first action: opens/closes the loop, advances the layer
        void SetLoopTap() {
            if (advanceLayerOnNextTap) {
                std::cout << "SetLoopTap(): advancing layer" << std::endl;
                if (++currentLayer >= numLayers) {
                    std::cout << "SetLoopTap(): reached end of layers; wrapping to first" << std::endl;
                    currentLayer = 0;
                }
                advanceLayerOnNextTap = false;
            }
            if (currentLayer >= numLayers) {
                return;
            }

            switch (layer[currentLayer].state) {
                case LoopLayerState::STOPPED:
                case LoopLayerState::LOOPING:
                    // std::cout << "TapLoop(): opening loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].OpenLoop();
                    if (currentLayer > 0) {
                        lastLayerPositionAtLoopStart = layer[currentLayer - 1].GetCurrentFrame();
                    }
                    break;
                case LoopLayerState::SETTING:
                    // std::cout << "TapLoop(): closing loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].CloseLoop();
                    if (currentLayer > 0) {
                        layer[currentLayer - 1].resetFrame = lastLayerPositionAtLoopStart;
                        layer[currentLayer - 1].Reset();
                    }
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
            // std::cout << "ToggleOverdub(): layer = " << currentLayer << std::endl;
            if (currentLayer >= numLayers) {
                // std::cout << "(selecting last layer)" << std::endl;
                currentLayer = numLayers - 1;
                StopLoop();
                return;
            }
            if (currentLayer >= 0) {
                // std::cout << "(stopping current layer)" << std::endl;
                layer[currentLayer].Stop();
                currentLayer--;
                if (currentLayer < 0) {
                    // we stopped the first layer, so set up to use it for the next loop definition
                    currentLayer = 0;
                }
                // std::cout << "stopped layer and decremented selection; current = " << currentLayer << std::endl;
            }
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

//        void SetLoopPoints(frame_t start, frame_t end) {
//            if (currentLayer >= 0 && currentLayer < numLayers) {
//                layer[currentLayer].SetLoopPoints(start, end);
//            }
//        }

        void SetLoopStartFrame(frame_t start) {
            if (currentLayer >= 0 && currentLayer < numLayers) {
                layer[currentLayer].loopStartFrame = start;
                if (layer[currentLayer].resetFrame < layer[currentLayer].loopStartFrame) {
                    layer[currentLayer].resetFrame = layer[currentLayer].loopStartFrame;
                }
            }
        }

        void SetLoopEndFrame(frame_t end) {
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

        void Reset() {
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