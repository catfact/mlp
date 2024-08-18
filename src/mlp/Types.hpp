#pragma once

#include <bitset>

namespace mlp {
    typedef unsigned long int frame_t;

    // silly syntax sweetener for indexing bitfields by enum class values
    // ** enum class must have a `NUM_FLAGS` member!! **
    template<typename IdClass, IdClass Count>
    struct BitSet {
        std::bitset<static_cast<size_t>(Count)> flags;

        void Set(IdClass flag) {
            flags.set(static_cast<size_t>(flag));
        }

        void Clear(IdClass flag) {
            flags.reset(static_cast<size_t>(flag));
        }

        bool Test(IdClass flag) const {
            return flags.test(static_cast<size_t>(flag));
        }
    };

}