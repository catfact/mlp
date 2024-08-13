#pragma once

#include "readerwriterqueue/readerwriterqueue.h"
#include "mlp/Kernel.hpp"

namespace mlp {

    // main API for the audio effect

    class Mlp {
    public:
        enum class TapId: int {
            SWITCH1,
            SWITCH2,
            SWITCH3,
            SWITCH4
        };

    private:

        moodycamel::ReaderWriterQueue<TapId> tapQ;

        Kernel kernel;

    public:
        void ProcessAudioBlock(const float *input, float* output, unsigned int numFrames) {

            TapId tapId;
            while (tapQ.try_dequeue(tapId)) {
                switch (tapId) {
                    case TapId::SWITCH1:
                        kernel.TapLoop();
                        break;
                    case TapId::SWITCH2:
                        kernel.ToggleOverdub();
                        break;
                    case TapId::SWITCH3:
                        kernel.ToggleMute();
                        break;
                    case TapId::SWITCH4:
                        kernel.StopClear();
                        break;
                }
            }

            for (unsigned int i=0; i<numFrames; ++i) {
                *output = 0.f;
                *(output + 1) = 0.f;
                kernel.ProcessFrame(input, output);
            }
        }

        void Tap(TapId tapId) {
            tapQ.enqueue(tapId);
        }

    };
}