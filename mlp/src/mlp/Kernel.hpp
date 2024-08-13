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

    public:

        Kernel() {
            // FIXME: not an efficient design
            // we're giving each layer a full audio buffer, sized to maximum loop length
            // we *should* use one long buffer,
            // and dynamically assign pointers into it as loops are set
            for (int i = 0; i < numLayers; ++i) {
                buffer[i].fill(0.f);
                layer[i].buffer = buffer[i].data();
            }
        }

        // process a single *stereo interleaved* audio frame
        void ProcessFrame(const float *&src, float *&dst) {
            float x[2];
            float y[2] {0.f};
            x[0] = *src++;
            x[1] = *src++;
            for (int i = 0; i < numLayers; ++i) {
                if (layer[i].ProcessFrame(x, y)) {
                    // the loop has wrapped around (next frame will fall at loop start)
                    // different behaviors/modes are possible here
                    // for now: each layer after the first resets the layer below it
                    // this means that the most recent loop is effectively the "leader,"
                    // determining the period of all loops
                    if (i > 0) {
                        layer[i - 1].Reset();
                    }
                }
            }
            *dst++ = y[0];
            *dst++ = y[1];
        }

        //------------------------------------------------
        //-- control

        // first action: opens/closes the loop, advances the layer
        void TapLoop() {
            if (currentLayer >= numLayers) {
                // out of layers. for now, don't do anything
                // (TODO: maybe "re-cut" the last layer's loop)
                std::cout << "TapLoop(): out of layers" << std::endl;
                return;
            }

            switch (layer[currentLayer].state) {
                case LoopLayerState::STOPPED:
                    std::cout << "TapLoop(): opening loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].OpenLoop();
                    if (currentLayer > 0)
                    {
                        lastLayerPositionAtLoopStart = layer[currentLayer - 1].GetCurrentFrame();
                    }
                    break;
                case LoopLayerState::SETTING:
                    std::cout << "TapLoop(): closing loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].CloseLoop();
                    if (currentLayer > 0)
                    {
                        layer[currentLayer - 1].startOffset = lastLayerPositionAtLoopStart;
                        layer[currentLayer - 1].Reset();
                    }
                    break;
                case LoopLayerState::LOOPING:
                    std::cout << "TapLoop(): advancing layer; current = " << currentLayer << std::endl;
                    // advance to the next layer and start over
                    ++currentLayer;
                    TapLoop();
                    break;
            }
        }

        void ToggleOverdub() {
            std::cout << "ToggleOverdub(): layer = " << currentLayer << std::endl;
            if (currentLayer >= 0 && currentLayer < numLayers)
                layer[currentLayer].ToggleWrite();
        }


        void ToggleMute() {
            std::cout << "ToggleMute(): layer = " << currentLayer << std::endl;
            if (currentLayer >= 0 && currentLayer < numLayers)
                layer[currentLayer].ToggleRead();
        }

        void StopClear() {
            std::cout << "ToggleOverdub(): layer = " << currentLayer << std::endl;
            if (currentLayer >= numLayers) {
                std::cout << "(selecting last layer)" << std::endl;
                currentLayer = numLayers - 1;
                StopClear();
                return;
            }
            if (currentLayer >= 0) {
                std::cout << "(stopping current layer)" << std::endl;
                layer[currentLayer].Stop();
                currentLayer--;
                if (currentLayer < 0) {
                    // we stopped the first layer, so set up to use it for the next loop definition
                    currentLayer = 0;
                }
                std::cout << "stopped layer and decremented selection; current = " << currentLayer << std::endl;
            }
        }
    };
}