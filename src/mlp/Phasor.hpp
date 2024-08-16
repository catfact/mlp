#pragma once

#include "Constants.hpp"
#include "Types.hpp"

namespace mlp {

    enum class PhasorAdvanceResult {
        INACTIVE,
        CONTINUING,
        WRAPPED,
        DONE_FADEOUT,
        DONE_FADEIN
    };

    //------------------------------------------------
// simplistic  phasor including crossfade
// for now, just counts integer frames in one direction
    struct FadePhasor {

        frame_t currentFrame{0};
        frame_t maxFrame{std::numeric_limits<frame_t>::max()};
        bool isFadingIn{false};
        bool isFadingOut{false};
        bool isActive{false};
        float fadePhase{0.f};
        float fadeIncrement{0.01f};
        float fadeValue{0.f};

        // return true if the phasor has wrapped
        PhasorAdvanceResult Advance() {
            if (!isActive) {
                return PhasorAdvanceResult::INACTIVE;
            }

            PhasorAdvanceResult result = PhasorAdvanceResult::CONTINUING;

            if (isFadingIn) {
                fadePhase += fadeIncrement;
                if (fadePhase >= 1.f) {
                    fadePhase = 1.f;
                    isFadingIn = false;
                    fadeValue = 1.f;
                    result = PhasorAdvanceResult::DONE_FADEIN;
                } else {
                    fadeValue = sinf(fadePhase * pi_2<float>);
                }
            }
            if (isFadingOut) {
                fadePhase -= fadeIncrement;
                if (fadePhase <= 0.f) {
                    fadePhase = 0.f;
                    fadeValue = 0.f;
                    isFadingOut = false;
                    isActive = false;
                    result = PhasorAdvanceResult::DONE_FADEOUT;
                } else {
                    fadeValue = sinf(fadePhase * pi_2<float>);
                }
            }
            if (++currentFrame >= maxFrame) {
                if (!isFadingOut) {
                    result = PhasorAdvanceResult::WRAPPED;
                    isFadingOut = true;
                }
            }
            return result;
        }

        void Reset(frame_t position = 0) {
            currentFrame = position;
            fadePhase = 0.f;
            fadeValue = 0.f;
            isFadingOut = false;
            isFadingIn = true;
            isActive = true;
            // std::cout << "[FadePhasor] reset to position " << position << std::endl;
        }
    };

}