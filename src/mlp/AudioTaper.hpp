#pragma once

namespace mlp {
    class AudioTaper {
        static constexpr int tableSize = 128;
        static const double ampData[tableSize];
        static const char* dbData[tableSize];

    };
}