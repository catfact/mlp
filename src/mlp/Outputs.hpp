#pragma once

#include "Constants.hpp"
#include "Types.hpp"

namespace mlp {

    // non-audio output values, calculated on each block from the audio thread
    enum class LayerOutputFlagId {
        Selected=0,
        Inner,
        Outer,
        Wrapped,
        Looped,
        Triggered,
        Paused,
        Resumed,
        Reset,
        Restarted,
        Silent,
        Active,
        Stopped,
        Writing,
        NotWriting,
        Clearing,
        NotClearing,
        Reading,
        NotReading,
        Opened,
        Closed,
        LoopEnabled,
        LoopDisabled,
        Setting,
        Playing,
        Count
    };

    static const char LayerOutputFlagLabel[static_cast<unsigned long>(LayerOutputFlagId::Count)][16] = {
        "Selected",
        "Inner",
        "Outer",
        "Wrapped",
        "Looped",
        "Triggered",
        "Paused",
        "Resumed",
        "Reset",
        "Restarted",
        "Silent",
        "Active",
        "Stopped",
        "Writing",
        "NotWriting",
        "Clearing",
        "NotClearing",
        "Reading",
        "NotReading",
        "Opened",
        "Closed",
        "LoopEnabled",
        "LoopDisabled",
        "Setting",
        "Playing"
    };



    typedef BitSet<LayerOutputFlagId, LayerOutputFlagId::Count> LayerOutputFlags;

    struct LayerOutputs {
        LayerOutputFlags flags;
        // initial and final logical frame positions since last read
        frame_t positionRange[2]{};
    };

    struct OutputsData {
        LayerOutputs layers[numLoopLayers];
        void SetLayerFlag(unsigned int layer, LayerOutputFlagId flag) {
            layers[layer].flags.Set(flag);
        }
    };

    struct LayerFlagsMessageData {
        unsigned int layer{};
        LayerOutputFlags flags;
    };

    struct LayerPositionMessageData {
        unsigned int layer;
        frame_t positionRange[2];
    };

    struct LayerLoopEndMessageData {
        unsigned int layer;
        frame_t loopEnd;
    };


    static const OutputsData defaultOutputsData{};

}
