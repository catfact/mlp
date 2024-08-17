#pragma once

#include <bitset>

#include "Constants.hpp"
#include "Types.hpp"

namespace mlp {

    enum class TriggerModeFlag {
        RESET_BELOW,
        PAUSE_SELF
    };
    typedef BitSet<TriggerModeFlag> TriggerMode;

    enum class LoopModeFlag {
        LOOP,
        ONE_SHOT
    };


}