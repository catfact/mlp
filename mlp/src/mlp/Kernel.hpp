#pragma once

#include "Constants.h"

namespace mlp {

    // encapsulates all audio processing and control logic
    class Kernel {

        void ProcessBlock(AudioBuffer& buffer) {
            //--- process audio
            for (int i = 0; i < Constants::VECTORS_PER_BLOCK; i++) {
                //--- process audio
                for (int j = 0; j < Constants::FRAMES_PER_VECTOR; j++) {
                    //--- process audio
                }
            }
        }

    };
}