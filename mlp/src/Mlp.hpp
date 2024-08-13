#pragma once

#include "readerwriterqueue/readerwriterqueue.h"
#include "mlp/Kernel.hpp"

namespace mlp {

    // main API for the audio effect

    class Mlp {
    public:
        /// pretty silly...
        /// give these meaningful names when API is more stable
        /// (same goes for OSC paths)
        enum class TapId: int {
            SetLoop,
            ToggleOverdub,
            ToggleMute,
            StopLoop
        };

        enum class ParamId: int {
            PreserveLevel,
            RecordLevel,
            PlaybackLevel,
        };

        struct ParamChangeRequest {
            ParamId id;
            float value;
        };

    private:

        moodycamel::ReaderWriterQueue<TapId> tapQ;
        moodycamel::ReaderWriterQueue<ParamChangeRequest> paramQ;

        Kernel kernel;

    public:
        void ProcessAudioBlock(const float *input, float* output, unsigned int numFrames) {

            TapId tapId;
            while (tapQ.try_dequeue(tapId)) {
                switch (tapId) {
                    case TapId::SetLoop:
                        kernel.SetLoopTap();
                        break;
                    case TapId::ToggleOverdub:
                        kernel.ToggleOverdub();
                        break;
                    case TapId::ToggleMute:
                        kernel.ToggleMute();
                        break;
                    case TapId::StopLoop:
                        kernel.StopLoop();
                        break;
                }
            }

            ParamChangeRequest paramChangeRequest;
            while (paramQ.try_dequeue(paramChangeRequest)) {
                switch (paramChangeRequest.id) {
                    case ParamId::PreserveLevel:
                        kernel.SetPreserveLevel(paramChangeRequest.value);
                        break;
                    case ParamId::RecordLevel:
                        kernel.SetRecordLevel(paramChangeRequest.value);
                        break;
                    case ParamId::PlaybackLevel:
                        kernel.SetPlaybackLevel(paramChangeRequest.value);
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

        void ParamChange(ParamId id, float value) {
            paramQ.enqueue({id, value});
        }

    };
}