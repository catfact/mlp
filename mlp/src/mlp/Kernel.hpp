#pragma once

#include <array>

// #include "../Constants.hpp"

namespace mlp {
    typedef unsigned long int frame_t;

    //------------------------------------------------
    // simplistic phasor that counts frames
    struct FramePhasor {
        frame_t currentFrame{0};
        // exclusive maximum frame count
        frame_t maxFrame{1000};

        // return true if the phasor has wrapped
        bool Tick() {
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

        float playLevel;
        float recordLevel;
        float preserveLevel;

        enum class State {
            STOPPED,
            SETTING,
            LOOPING,
        };

        State state{State::STOPPED};

        void StartRecord(frame_t offset = 0) {
            state = State::SETTING;
            writeActive = true;
        }

        void FinishRecord(bool shouldDub = false)
        {
            state = State::LOOPING;
            readActive = true;
            if (!shouldDub)
            {
                writeActive = false;
            }
            phasor.maxFrame = phasor.currentFrame;
            phasor.currentFrame = 0;
        }

        bool ToggleWrite()
        {
            writeActive = !writeActive;
            return writeActive;
        }

        void Stop()
        {
            readActive = false;
            writeActive = false;
            state = State::STOPPED;
        }

        // return true if the loop wraps after this frame
        bool ProcessFrame(float *src, float *dst) {
            auto bufIdx = (phasor.currentFrame + frameOffset) % bufferFrames;
            if (readActive) {
                for (int ch = 0; ch < numChannels; ++ch) {
                    *(dst + ch) += buffer[bufIdx] * playLevel;
                }
                if (writeActive) {
                    for (int ch = 0; ch < numChannels; ++ch) {
                        buffer[bufIdx] = *(src + ch) * recordLevel + buffer[bufIdx] * preserveLevel;
                    }
                }
            }
            dst += numChannels;
            src += numChannels;
            return phasor.Tick();
        };

        void Reset() {
            phasor.currentFrame = 0;
        }

        void SetMaxFrame(frame_t aFrameIndex) {
            phasor.maxFrame = aFrameIndex;
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
        std::array<LayerBuffer, numLayers> buffers{};

    public:

        Kernel() {
            for (int i = 0; i < numLayers; ++i) {
                layer[i].buffer = buffers[i].data();
            }
        }

        // process a single *stereo interleaved* audio frame
        void ProcessFrame(const float *&src, float *&dst) {
            float x[2];
            float y[2];
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
                return;
            }

            switch (layer[currentLayer].state) {
                case Layer::State::STOPPED:
                    layer[currentLayer].StartRecord();
                    break;
                case Layer::State::SETTING:
                    layer[currentLayer].FinishRecord();
                    break;
                case Layer::State::LOOPING:
                    // advance to the next layer and start over
                    ++currentLayer;
                    TapLoop();
                    break;
            }
        }

        void ToggleOverdub() {
            if (currentLayer >= 0 && currentLayer < numLayers)
                layer[currentLayer].ToggleWrite();
        }

        void StopClear()
        {
            if (currentLayer > )
            if (currentLayer >= 0 && currentLayer < numLayers) {
                if (currentLayer < 0 || currentLayer >= numLayers)
                    return;
                layer[currentLayer].Stop();
                currentLayer--;
            }
        }
    };
}