#pragma once

#include "AudioContext.hpp"
#include "mlp/Kernel.hpp"

namespace mlp {
    class Mlp {

        Kernel kernel;
        AudioContext audioContext;

    public:

        Mlp()
        {
            audioContext.SetBlockProcessCallback([this](AudioContext::sample_t *buf) {
                kernel.ProcessBlock(buf, buf);
            });
        }

        void ProcessAudioBlock(float *buf, unsigned int numFrames) {
            audioContext.MainAudioCallback(buf, numFrames);
        }

    };
}