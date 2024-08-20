#pragma once

#include "readerwriterqueue/readerwriterqueue.h"
#include "mlp/Kernel.hpp"

namespace mlp {

    // main API for the audio effect

    class Mlp {
    public:
        enum class TapId: int {
            SetLoop,
            ToggleOverdub,
            ToggleMute,
            StopLoop,
            Reset,
        };

        enum class FloatParamId: int {
            PreserveLevel,
            RecordLevel,
            PlaybackLevel,
            FadeIncrement
        };

        enum class IndexParamId: int {
            SelectLayer,
            ResetLayer,
            LoopStartFrame,
            LoopEndFrame,
            LoopResetFrame,
        };

        enum class BoolParamId: int {
            LoopEnabled,
        };

        template <typename Id, typename Value>
        struct ParamChangeRequest {
            Id id;
            Value value;
        };

    private:

        moodycamel::ReaderWriterQueue<TapId> tapQ;
        moodycamel::ReaderWriterQueue<ParamChangeRequest<FloatParamId, float>> floatParamQ;
        moodycamel::ReaderWriterQueue<ParamChangeRequest<IndexParamId, unsigned long int>> indexParamQ;
        moodycamel::ReaderWriterQueue<ParamChangeRequest<BoolParamId, bool>> boolParamQ;

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
                    case TapId::Reset:
                        kernel.ResetCurrent();
                        break;
                }
            }

            ParamChangeRequest<FloatParamId, float> floatParamChangeRequest{};
            while (floatParamQ.try_dequeue(floatParamChangeRequest)) {
                switch (floatParamChangeRequest.id) {
                    case FloatParamId::PreserveLevel:
                        kernel.SetPreserveLevel(floatParamChangeRequest.value);
                        break;
                    case FloatParamId::RecordLevel:
                        kernel.SetRecordLevel(floatParamChangeRequest.value);
                        break;
                    case FloatParamId::PlaybackLevel:
                        kernel.SetPlaybackLevel(floatParamChangeRequest.value);
                        break;
                    case FloatParamId::FadeIncrement:
                        kernel.SetFadeIncrement(floatParamChangeRequest.value);
                        break;
                }
            }

            ParamChangeRequest<IndexParamId, unsigned long int> indexParamChangeRequest{};
            while (indexParamQ.try_dequeue(indexParamChangeRequest)) {
                switch (indexParamChangeRequest.id) {
                    case IndexParamId::SelectLayer:
                        kernel.SelectLayer((int)indexParamChangeRequest.value);
                        break;
                    case IndexParamId::ResetLayer:
                        kernel.ResetLayer((int)indexParamChangeRequest.value);
                        break;
                    case IndexParamId::LoopStartFrame:
                        kernel.SetLoopStartFrame(indexParamChangeRequest.value);
                        break;
                    case IndexParamId::LoopEndFrame:
                        kernel.SetLoopEndFrame(indexParamChangeRequest.value);
                        break;
                    case IndexParamId::LoopResetFrame:
                        kernel.SetLoopResetFrame(indexParamChangeRequest.value);
                        break;
                }
            }

            ParamChangeRequest<BoolParamId, bool> boolParamChangeRequest{};
            while (boolParamQ.try_dequeue(boolParamChangeRequest)) {
                switch (boolParamChangeRequest.id) {
                    case BoolParamId::LoopEnabled:
                        kernel.SetLoopEnabled(boolParamChangeRequest.value);
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

        void FloatParamChange(FloatParamId id, float value) {
            floatParamQ.enqueue({id, value});
        }

        void IndexParamChange(IndexParamId id, unsigned long int value) {
            indexParamQ.enqueue({id, value});
        }

        void BoolParamChange(BoolParamId id, bool value) {
            boolParamQ.enqueue({id, value});
        }

    };
}