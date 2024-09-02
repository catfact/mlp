#pragma once

#include "readerwriterqueue/readerwriterqueue.h"
#include "mlp/Kernel.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"

/// FIXME nasty
#define EventQueue moodycamel::ReaderWriterQueue

namespace mlp {

    // main API for the audio effect

    class Mlp {
    public:

        enum class TapId : int {
            Set,
            Stop,
            Reset,
            Count
        };

        static constexpr char TapIdLabel[static_cast<int>(TapId::Count)][16] = {
                "SET",
                "STOP",
                "RESET"
        };

        enum class FloatParamId : int {
            PreserveLevel,
            RecordLevel,
            PlaybackLevel,
            FadeTime,
            SwitchTime,
            Count
        };

        static constexpr char FloatParamIdLabel[static_cast<int>(FloatParamId::Count)][16] = {
                "PRESERVE",
                "RECORD",
                "PLAYBACK",
                "FADE",
                "SWITCH"
        };

        enum class IndexFloatParamId : int {
            LayerPreserveLevel,
            LayerRecordLevel,
            LayerPlaybackLevel,
            LayerFadeTime,
            LayerSwitchTime,
            Count
        };

        static constexpr char IndexFloatParamIdLabel[static_cast<int>(IndexFloatParamId::Count)][32] = {
                "PRESERVE",
                "RECORD",
                "PLAYBACK",
                "FADE",
                "SWITCH"
        };

        struct IndexFloatParamValue {
            unsigned int index;
            float value;
        };

        enum class IndexParamId : int {
            Mode,
            SelectLayer,
            ResetLayer,
            LoopStartFrame,
            LoopEndFrame,
            LoopResetFrame,
            Count
        };

        static constexpr char IndexParamIdLabel[static_cast<int>(IndexParamId::Count)][16] = {
                "MODE",
                "SELECT",
                "RESET",
                "STARTPOS",
                "ENDPOS",
                "RESETPOS"
        };

        enum class IndexIndexParamId : int {
            LayerLoopStartFrame,
            LayerLoopEndFrame,
            LayerLoopResetFrame,
            Count
        };

        static constexpr char IndexIndexParamIdLabel[static_cast<int>(IndexIndexParamId::Count)][32] = {
                "STARTPOS",
                "ENDPOS",
                "RESETPOS"
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
            Count
        };

        static constexpr char BoolParamIdLabel[static_cast<int>(BoolParamId::Count)][16] = {
                "WRITE",
                "CLEAR",
                "READ",
                "LOOP"
        };

        enum class IndexBoolParamId : int {
            LayerWriteEnabled,
            LayerClearEnabled,
            LayerReadEnabled,
            LayerLoopEnabled,
            Count
        };

        static constexpr char IndexBoolParamIdLabel[static_cast<int>(IndexBoolParamId::Count)][32] = {
                "WRITE",
                "CLEAR",
                "READ",
                "LOOP"
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

        struct ParamChangeQ {
            EventQueue<TapId> tapQ;
            EventQueue<ParamChangeRequest<FloatParamId, float>> floatQ;
            EventQueue<ParamChangeRequest<IndexParamId, unsigned long int>> indexQ;
            EventQueue<ParamChangeRequest<BoolParamId, bool>> boolQ;
            EventQueue<ParamChangeRequest<IndexFloatParamId, IndexFloatParamValue>> indexFloatQ;
            EventQueue<ParamChangeRequest<IndexIndexParamId, IndexIndexParamValue>> indexIndexQ;
            EventQueue<ParamChangeRequest<IndexBoolParamId, IndexBoolParamValue>> indexBoolQ;
        };
        ParamChangeQ paramChangeQ;

        struct OutputsQ {
            EventQueue<mlp::LayerFlagsMessageData> layerFlagsQ;
            EventQueue<mlp::LayerPositionMessageData> layerPositionQ;
        };
        OutputsQ outputsQ;

        Kernel kernel;
        mlp::OutputsData outputsData;
        unsigned long int framesSinceOutput = 0;

        float sampleRate;

    public:
        void SetSampleRate(float aSampleRate) { sampleRate = aSampleRate; }

        void ProcessAudioBlock(const float *input, float *output, unsigned int numFrames) {

            ProcessParamChanges();

            for (unsigned int i = 0; i < numFrames; ++i) {
                *output = 0.f;
                *(output + 1) = 0.f;
                kernel.ProcessFrame(input, output);
            }

            ProcessOutputs(numFrames);
        }

        EventQueue<mlp::LayerFlagsMessageData> &GetLayerFlagsQ() {
            return outputsQ.layerFlagsQ;
        }

        EventQueue<mlp::LayerPositionMessageData> &GetLayerPositionQ() {
            return outputsQ.layerPositionQ;
        }

    private:

        void ProcessOutputs(unsigned int numFrames) {
            framesSinceOutput += numFrames;
            if (framesSinceOutput >= framesPerOutput) {
                framesSinceOutput = 0;
                kernel.FinalizeOutputs();
                for (unsigned int i = 0; i < numLoopLayers; ++i) {
                    if (outputsData.layers[i].flags.Any()) {
                        // std::cout << "enqueuing layer output flags; layer: " << i << std::endl;
                        outputsQ.layerFlagsQ.enqueue({i, outputsData.layers[i].flags});
                    }
                }
                kernel.InitializeOutputs(&outputsData);
            }
        }

        void ProcessParamChanges() {
            TapId tapId;
            while (paramChangeQ.tapQ.try_dequeue(tapId)) {
                switch (tapId) {
                    case TapId::Set:
                        kernel.SetLoopTap();
                        break;
                    case TapId::Stop:
                        kernel.StopLoop();
                        break;
                    case TapId::Reset:
                        kernel.ResetLayer();
                        break;
                    default:
                        break;
                }
            }

            ParamChangeRequest<FloatParamId, float> floatParamChangeRequest{};
            while (paramChangeQ.floatQ.try_dequeue(floatParamChangeRequest)) {
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
/// FIXME: need a more formal/ explicit way of specifying parameter range mappings
                        kernel.SetFadeTime(floatParamChangeRequest.value * 10.f);
                        break;
                    case FloatParamId::SwitchTime:
/// FIXME: need a more formal/ explicit way of specifying parameter range mappings
                        kernel.SetSwitchTime(floatParamChangeRequest.value * 10.f);
                        break;
                    default:
                        break;
                }
            }

            ParamChangeRequest<IndexParamId, unsigned long int> indexParamChangeRequest{};
            while (paramChangeQ.indexQ.try_dequeue(indexParamChangeRequest)) {
                switch (indexParamChangeRequest.id) {
                    case IndexParamId::Mode:
                        kernel.SetMode(static_cast<LayerBehaviorModeId>(indexParamChangeRequest.value));
                        break;
                    case IndexParamId::SelectLayer:
                        kernel.SetCurrentLayer((unsigned int) indexParamChangeRequest.value);
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
                    default:
                        break;
                }
            }

            ParamChangeRequest<BoolParamId, bool> boolParamChangeRequest{};
            while (paramChangeQ.boolQ.try_dequeue(boolParamChangeRequest)) {
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
                    default:
                        break;
                }
            }

            ParamChangeRequest<IndexFloatParamId, IndexFloatParamValue> indexFloatParamChangeRequest{};
            while (paramChangeQ.indexFloatQ.try_dequeue(indexFloatParamChangeRequest)) {
                float tmpValue;
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
/// FIXME: need a more formal/ explicit way of specifying parameter range mappings
                        tmpValue = indexFloatParamChangeRequest.value.value * 10.f;
                        kernel.SetFadeTime(tmpValue,
                                                (int) indexFloatParamChangeRequest.value.index);

                    case IndexFloatParamId::LayerSwitchTime:
/// FIXME: need a more formal/ explicit way of specifying parameter range mappings
                        tmpValue = indexFloatParamChangeRequest.value.value * 10.f;
                        kernel.SetSwitchTime(tmpValue,
                                           (int) indexFloatParamChangeRequest.value.index);

                        break;
                    default:
                        break;
                }
            }

            ParamChangeRequest<IndexIndexParamId, IndexIndexParamValue> indexIndexParamChangeRequest{};
            while (paramChangeQ.indexIndexQ.try_dequeue(indexIndexParamChangeRequest)) {
                switch (indexIndexParamChangeRequest.id) {
                    case IndexIndexParamId::LayerLoopStartFrame:
                        kernel.SetLoopStartFrame(indexIndexParamChangeRequest.value.value,
                                                 (int) indexIndexParamChangeRequest.value.index);
                        break;
                    case IndexIndexParamId::LayerLoopEndFrame:
                        kernel.SetLoopEndFrame(indexIndexParamChangeRequest.value.value,
                                               (int) indexIndexParamChangeRequest.value.index);
                        break;
                    case IndexIndexParamId::LayerLoopResetFrame:
                        kernel.SetLoopResetFrame(indexIndexParamChangeRequest.value.value,
                                                 (int) indexIndexParamChangeRequest.value.index);
                        break;
                    default:
                        break;
                }
            }
            ParamChangeRequest<IndexBoolParamId, IndexBoolParamValue> indexBoolParamChangeRequest{};
            while (paramChangeQ.indexBoolQ.try_dequeue(indexBoolParamChangeRequest)) {
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
                        kernel.SetLoopEnabled(
                                /// FIXME: weird that the arguments are reversed on this one
                                indexBoolParamChangeRequest.value.value,
                                static_cast<int>(indexBoolParamChangeRequest.value.index));
                        break;
                    default:
                        break;
                }
            }
        }


    public:

        //-----------------------------------------------------------------------------------
        //--- param change API
        void Tap(TapId tapId) {
            paramChangeQ.tapQ.enqueue(tapId);
        }

        void FloatParamChange(FloatParamId id, float value) {
            paramChangeQ.floatQ.enqueue({id, value});
        }

        void IndexParamChange(IndexParamId id, unsigned long int value) {
            paramChangeQ.indexQ.enqueue({id, value});
        }

        void BoolParamChange(BoolParamId id, bool value) {
            paramChangeQ.boolQ.enqueue({id, value});
        }

        void IndexFloatParamChange(IndexFloatParamId id, unsigned int index, float value) {
            paramChangeQ.indexFloatQ.enqueue({id, {index, value}});
        }

        void IndexIndexParamChange(IndexIndexParamId id, unsigned int index, unsigned long int value) {
            paramChangeQ.indexIndexQ.enqueue({id, {index, value}});
        }

        void IndexBoolParamChange(IndexBoolParamId id, unsigned int index, bool value) {
            paramChangeQ.indexBoolQ.enqueue({id, {index, value}});
        }

        frame_t GetLoopEndFrame(unsigned int aLayerIndex) {
            return kernel.GetLoopEndFrame(aLayerIndex);
        }

    };
}


#pragma GCC diagnostic pop