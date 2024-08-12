#pragma once

#include "Types.hpp"

namespace mlp {

//------------------------------------------------
// simplistic  phasor including crossfade
// for now, just counts integer frames in one direction
    class FadePhasor {
    public:

        enum class AdvanceResult {
            INACTIVE,
            CONTINUING,
            WRAPPED,
            DONE_FADEOUT,
            DONE_FADEIN
        };

        frame_t currentFrame{0};
        frame_t maxFrame{std::numeric_limits<frame_t>::max()};
        bool isFadingIn{false};
        bool isFadingOut{false};
        bool isActive{false};
        float fadePhase{0.f};
        float fadeIncrement{0.01f};
        float fadeValue{0.f};

        // return true if the phasor has wrapped
        AdvanceResult Advance() {
            if (!isActive) {
                return AdvanceResult::INACTIVE;
            }

            AdvanceResult result = AdvanceResult::CONTINUING;

            if (isFadingIn) {
                fadePhase += fadeIncrement;
                if (fadePhase >= 1.f) {
                    fadePhase = 1.f;
                    isFadingIn = false;
                    fadeValue = 1.f;
                    result = AdvanceResult::DONE_FADEIN;
                } else {
                    fadeValue = sinf(fadePhase * static_cast<float>(M_PI_2));
                }
            }
            if (isFadingOut) {
                fadePhase -= fadeIncrement;
                if (fadePhase <= 0.f) {
                    fadePhase = 0.f;
                    fadeValue = 0.f;
                    isFadingOut = false;
                    isActive = false;
                    result = AdvanceResult::DONE_FADEOUT;
                } else {
                    fadeValue = sinf(fadePhase * static_cast<float>(M_PI_2));
                }
            }
            if (++currentFrame >= maxFrame) {
                if (!isFadingOut) {
                    result = AdvanceResult::WRAPPED;
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
            std::cout << "[FadePhasor] reset to position " << position << std::endl;
        }
    };

}