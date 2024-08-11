#pragma once

#include "readerwriterqueue/readerwriterqueue.h"
#include "mlp/Kernel.hpp"

namespace mlp {

    // main API for the audio effect

    class Mlp {
        enum class TapId: int {
            SWITCH1,
            SWITCH2,
            SWITCH3
        };

    private:

        moodycamel::ReaderWriterQueue<TapId> tapQ;

        Kernel kernel;

    public:
        void ProcessAudioBlock(float *buf, unsigned int numFrames) {

            // loop over pending tap events
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
                        kernel.StopClear();
                        break;
                }
            }

            const float* src = buf;
            float* dst = buf;
            for (unsigned int i=0; i<numFrames; ++i) {
                kernel.ProcessFrame(src, dst);
            }
        }

        void Tap(TapId tapId) {
            tapQ.enqueue(tapId);
        }

    };
}