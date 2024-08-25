#pragma once

#include "readerwriterqueue/readerwriterqueue.h"
#include "mlp/Kernel.hpp"

namespace mlp {

    // main API for the audio effect

    class Mlp {
    public:

        enum class TapId : int {
            SetLoop,
            ToggleOverdub,
            ToggleMute,
            StopLoop,
            Reset,
        };

        enum class FloatParamId : int {
            PreserveLevel,
            RecordLevel,
            PlaybackLevel,
            FadeTime,
            SwitchTime
        };

        enum class IndexFloatParamId : int {
            LayerPreserveLevel,
            LayerRecordLevel,
            LayerPlaybackLevel,
            LayerFadeTime,
            LayerSwitchTime
        };

        struct IndexFloatParamValue {
            unsigned int index;
            float value;
        };

        enum class IndexParamId : int {
            SelectLayer,
            ResetLayer,
            LoopStartFrame,
            LoopEndFrame,
            LoopResetFrame,
        };

        enum class IndexIndexParamId : int {
            LayerLoopStartFrame,
            LayerLoopEndFrame,
            LayerLoopResetFrame,
        };

        struct IndexIndexParamValue {
            unsigned int index;
            unsigned long int value;
        };

        enum class BoolParamId : int {
            WriteEnabled,
            ClearEnabled,
            ReadEnabled,
            LoopEnabled,
        };

        enum class IndexBoolParamId : int {
            LayerWriteEnabled,
            LayerClearEnabled,
            LayerReadEnabled,
            LayerLoopEnabled,
        };

        struct IndexBoolParamValue {
            unsigned int index;
            bool value;
        };

        template<typename Id, typename Value>
        struct ParamChangeRequest {
            Id id;
            Value value;
        };

    private:

        struct Q {
            moodycamel::ReaderWriterQueue<TapId> tapQ;
            moodycamel::ReaderWriterQueue<ParamChangeRequest<FloatParamId, float>> floatQ;
            moodycamel::ReaderWriterQueue<ParamChangeRequest<IndexParamId, unsigned long int>> indexQ;
            moodycamel::ReaderWriterQueue<ParamChangeRequest<BoolParamId, bool>> boolQ;
            moodycamel::ReaderWriterQueue<ParamChangeRequest<IndexFloatParamId, IndexFloatParamValue>> indexFloatQ;
            moodycamel::ReaderWriterQueue<ParamChangeRequest<IndexIndexParamId, IndexIndexParamValue>> indexIndexQ;
            moodycamel::ReaderWriterQueue<ParamChangeRequest<IndexBoolParamId, IndexBoolParamValue>> indexBoolQ;
        };
        Q q;
        Kernel kernel;

    public:
        void ProcessAudioBlock(const float *input, float *output, unsigned int numFrames) {

            TapId tapId;
            while (q.tapQ.try_dequeue(tapId)) {
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
                        kernel.ResetLayer();
                        break;
                }
            }

            ParamChangeRequest<FloatParamId, float> floatParamChangeRequest{};
            while (q.floatQ.try_dequeue(floatParamChangeRequest)) {
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
                    case FloatParamId::FadeTime:
                        kernel.SetFadeIncrement(floatParamChangeRequest.value);
                        break;
                    case FloatParamId::SwitchTime:
                        // TODO!
                        break;
                }
            }

            ParamChangeRequest<IndexParamId, unsigned long int> indexParamChangeRequest{};
            while (q.indexQ.try_dequeue(indexParamChangeRequest)) {
                switch (indexParamChangeRequest.id) {
                    case IndexParamId::SelectLayer:
                        kernel.SelectLayer((int) indexParamChangeRequest.value);
                        break;
                    case IndexParamId::ResetLayer:
                        kernel.ResetLayer((int) indexParamChangeRequest.value);
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
            while (q.boolQ.try_dequeue(boolParamChangeRequest)) {
                switch (boolParamChangeRequest.id) {
                    case BoolParamId::WriteEnabled:
                        kernel.SetWrite(boolParamChangeRequest.value);
                        break;
                    case BoolParamId::ClearEnabled:
                        kernel.SetClear(boolParamChangeRequest.value);
                        break;
                    case BoolParamId::ReadEnabled:
                        kernel.SetRead(boolParamChangeRequest.value);
                        break;
                    case BoolParamId::LoopEnabled:
                        kernel.SetLoopEnabled(boolParamChangeRequest.value);
                        break;
                }
            }

            ParamChangeRequest<IndexFloatParamId, IndexFloatParamValue> indexFloatParamChangeRequest{};
            while (q.indexFloatQ.try_dequeue(indexFloatParamChangeRequest)) {
                switch (indexFloatParamChangeRequest.id) {
                    case IndexFloatParamId::LayerPreserveLevel:
                        kernel.SetPreserveLevel(indexFloatParamChangeRequest.value.value,
                                                (int) indexFloatParamChangeRequest.value.index);
                        break;
                    case IndexFloatParamId::LayerRecordLevel:
                        kernel.SetRecordLevel(indexFloatParamChangeRequest.value.value,
                                              (int) indexFloatParamChangeRequest.value.index);
                        break;
                    case IndexFloatParamId::LayerPlaybackLevel:
                        kernel.SetPlaybackLevel(indexFloatParamChangeRequest.value.value,
                                                (int) indexFloatParamChangeRequest.value.index);
                        break;
                    case IndexFloatParamId::LayerFadeTime:
                        kernel.SetFadeIncrement(indexFloatParamChangeRequest.value.value,
                                                (int) indexFloatParamChangeRequest.value.index);

                    case IndexFloatParamId::LayerSwitchTime:
                        /// TODO!
                        break;
                }
            }

            ParamChangeRequest<IndexIndexParamId, IndexIndexParamValue> indexIndexParamChangeRequest{};
            while (q.indexIndexQ.try_dequeue(indexIndexParamChangeRequest)) {
                switch (indexIndexParamChangeRequest.id) {
                    case IndexIndexParamId::LayerLoopStartFrame:
                        kernel.SetLoopStartFrame(indexIndexParamChangeRequest.value.value,
                                                 (int)indexIndexParamChangeRequest.value.index);
                        break;
                    case IndexIndexParamId::LayerLoopEndFrame:
                        kernel.SetLoopEndFrame(indexIndexParamChangeRequest.value.value,
                                               (int)indexIndexParamChangeRequest.value.index);
                        break;
                    case IndexIndexParamId::LayerLoopResetFrame:
                        kernel.SetLoopResetFrame(indexIndexParamChangeRequest.value.value,
                                                 (int)indexIndexParamChangeRequest.value.index);
                        break;
                }
            }
            ParamChangeRequest<IndexBoolParamId, IndexBoolParamValue> indexBoolParamChangeRequest{};
            while (q.indexBoolQ.try_dequeue(indexBoolParamChangeRequest)) {
                switch (indexBoolParamChangeRequest.id) {
                    case IndexBoolParamId::LayerWriteEnabled:
                        kernel.SetLayerWrite(indexBoolParamChangeRequest.value.index,
                                             indexBoolParamChangeRequest.value.value);
                        break;
                    case IndexBoolParamId::LayerClearEnabled:
                        kernel.SetLayerClear(indexBoolParamChangeRequest.value.index,
                                             indexBoolParamChangeRequest.value.value);
                        break;
                    case IndexBoolParamId::LayerReadEnabled:
                        kernel.SetLayerRead(indexBoolParamChangeRequest.value.index,
                                            indexBoolParamChangeRequest.value.value);
                        break;
                    case IndexBoolParamId::LayerLoopEnabled:
                        kernel.SetLoopEnabled(indexBoolParamChangeRequest.value.value);
                        break;
                }
            }

            for (unsigned int i = 0; i < numFrames; ++i) {
                *output = 0.f;
                *(output + 1) = 0.f;
                kernel.ProcessFrame(input, output);
            }
        }

        void Tap(TapId tapId) {
            q.tapQ.enqueue(tapId);
        }

        void FloatParamChange(FloatParamId id, float value) {
            q.floatQ.enqueue({id, value});
        }

        void IndexParamChange(IndexParamId id, unsigned long int value) {
            q.indexQ.enqueue({id, value});
        }

        void BoolParamChange(BoolParamId id, bool value) {
            q.boolQ.enqueue({id, value});
        }

        void IndexFloatParamChange(IndexFloatParamId id, unsigned int index, float value) {
            q.indexFloatQ.enqueue({id, {index, value}});
        }

        void IndexIndexParamChange(IndexIndexParamId id, unsigned int index, unsigned long int value) {
            q.indexIndexQ.enqueue({id, {index, value}});
        }

        void IndexBoolParamChange(IndexBoolParamId id, unsigned int index, bool value) {
            q.indexBoolQ.enqueue({id, {index, value}});
        }


    };
}