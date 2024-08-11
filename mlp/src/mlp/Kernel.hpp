#pragma once

#include <array>
#include <limits>



namespace mlp {
    typedef unsigned long int frame_t;

    //------------------------------------------------
    // simplistic phasor that counts frames
    struct FramePhasor {
        frame_t currentFrame{0};
        // exclusive maximum frame count
        frame_t maxFrame{std::numeric_limits<frame_t>::max()};

        // return true if the phasor has wrapped
        bool Advance() {
            if (++currentFrame >= maxFrame) {
                currentFrame = 0;
                return true;
            }
            return false;
        }
    };

    //---------------------------------------------------
    // a simple multichannel play/record structure
    template<int numChannels, frame_t bufferFrames>
    struct LoopLayer {
        FramePhasor phasor;
        float *buffer;
        frame_t frameOffset;

        bool writeActive;
        bool readActive;

        float playLevel {1.f};
        float recordLevel {1.f};
        float preserveLevel {1.f};

        enum class State {
            STOPPED,
            SETTING,
            LOOPING,
        };

        State state{State::STOPPED};

        void OpenLoop(frame_t offset = 0) {
            state = State::SETTING;
            writeActive = true;
            std::cout << "opened loop" << std::endl;
            /// FIXME: should store the current position of the next-lowest layer,
            /// so it can be set as the new offset for that layer when we close this loop
        }

        void CloseLoop(bool shouldDub = false) {
            state = State::LOOPING;
            readActive = true;
            if (!shouldDub) {
                writeActive = false;
            }
            phasor.maxFrame = phasor.currentFrame;
            phasor.currentFrame = 0;
            std::cout << "closed loop; length = " << phasor.maxFrame << std::endl;
        }

        bool ToggleWrite() {
            writeActive = !writeActive;
            std::cout << "toggled write, new state = " << writeActive << std::endl;
            return writeActive;
        }

        void Stop() {
            readActive = false;
            writeActive = false;
            state = State::STOPPED;
            std::cout << "stopped loop" << std::endl;
        }

        // return true if the loop wraps after this frame
        bool ProcessFrame(const float *src, float *dst) {
            if (state == State::STOPPED) {
                return false;
            }
            // auto bufIdx = (phasor.currentFrame + frameOffset) % bufferFrames;
            auto bufIdx = (phasor.currentFrame + frameOffset) % phasor.maxFrame;
            if (readActive) {
                for (int ch = 0; ch < numChannels; ++ch) {
                    *(dst + ch) += buffer[bufIdx] * playLevel;
                }
            }

            if (writeActive) {
                for (int ch = 0; ch < numChannels; ++ch) {
                    float x = *(src + ch);
                    x *= recordLevel;
                    float fb = buffer[bufIdx];
                    fb *= preserveLevel;
                    x += fb;
                    buffer[bufIdx] = x;
                }
            }
            dst += numChannels;
            src += numChannels;
            return phasor.Advance();
        };

        void Reset() {
            phasor.currentFrame = 0;
        }
    };

    //==============================================
    //==== core processor
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
            // we *should* use one long buffer, and dynamically assign pointers into it as loops are set
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
                    // for now, each layer after the first, resets the layer below it
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
                case Layer::State::STOPPED:
                    std::cout << "TapLoop(): opening loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].OpenLoop();
                    if (currentLayer > 0)
                    {
                        lastLayerPositionAtLoopStart = layer[currentLayer - 1].phasor.currentFrame;
                    }
                    break;
                case Layer::State::SETTING:
                    std::cout << "TapLoop(): closing loop; layer = " << currentLayer << std::endl;
                    layer[currentLayer].CloseLoop();
                    if (currentLayer > 0)
                    {
                        layer[currentLayer - 1].frameOffset = lastLayerPositionAtLoopStart;
                        // this _should_ result in gapless playback on the target layer...
                        // but worth checking
                        layer[currentLayer - 1].Reset();
                    }
                    break;
                case Layer::State::LOOPING:
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