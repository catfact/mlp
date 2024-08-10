#pragma once

namespace mlp { namespace Constants {

    //--- audio rates
    static constexpr int FRAMES_PER_VECTOR = 16;
    static constexpr int VECTORS_PER_BLOCK = 64;
    static constexpr int FRAMES_PER_BLOCK = FRAMES_PER_VECTOR * VECTORS_PER_BLOCK;

    //--- event queue
    static constexpr int EVENT_DATA_SIZE = 4;
}}