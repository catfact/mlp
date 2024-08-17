#pragma once

#include <bitset>

#include "Constants.hpp"
#include "Types.hpp"

namespace mlp {

    /// FIXME:
    /*
     * ok, i thinkt the design is moving towards something slightly more generalized;
     * instead of a collection of binary modes per "thing that happens",
     * we have the concepts of "conditions" and "actions"
     * each condition triggering a set of actions
     */

    enum class TriggerModeFlag {
        RESET_BELOW,
        PAUSE_SELF
    };
    typedef BitSet<TriggerModeFlag> TriggerMode;

    enum class LoopModeFlag {
        LOOP,
        ONE_SHOT
    };
    typedef BitSet<LoopModeFlag> LoopMode;


}