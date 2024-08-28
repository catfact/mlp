#pragma once

#include "Constants.hpp"
#include "Types.hpp"

namespace mlp {

    // non-audio output values, calculated on each block from the audio thread
    enum class LayerOutputFlagId {
        Selected,
        Inner,
        Wrapped,
        Triggered,
        Paused,
        Resumed,
        Reset,
        Silent,
        Active,
        Clearing,
        Count
    };

    typedef BitSet<LayerOutputFlagId, LayerOutputFlagId::Count> LayerOutputFlags;

    struct LayerOutputs {
        LayerOutputFlags flags;
        // initial and final logical frame positions since last read
        frame_t positionRange[2];
    };

    struct OutputsData {
        LayerOutputs layers[numLoopLayers];
        void SetLayerFlag(unsigned int layer, LayerOutputFlagId flag) {
            layers[layer].flags.Set(flag);
        }
    };

    struct LayerFlagsMessageData {
        unsigned int layer;
        LayerOutputFlags flags;
    };

    struct LayerPositionMessageData {
        unsigned int layer;
        frame_t positionRange[2];
    };


    static const OutputsData defaultOutputsData{};

//    class Outputs {
//        std::atomic<OutputsData> data;
//    public:
//        void Read(OutputsData& dst)  {
//            dst = data.exchange(defaultOutputsData);
//        }
//        void Write(OutputsData& src)  {
//            data.store(src);
//        }
//        static void WriteToQueue(OutputsData& src, moodycamel::ReaderWriterQueue<OutputsData>& queue) {
//            queue.try_enqueue(src);
//        }
//
//    };

}