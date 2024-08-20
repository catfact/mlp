#pragma once

#include "Constants.hpp"
#include "Types.hpp"

namespace mlp {

    // shoot these need to be bitfields or something actually
    enum class PhasorAdvanceResultFlag {
        INACTIVE,
        CONTINUING,
        WRAPPED_LOOP,
        CROSSED_TRIGGER,
        DONE_FADEOUT,
        DONE_FADEIN,
        NUM_FLAGS
    };

    typedef BitSet<PhasorAdvanceResultFlag, PhasorAdvanceResultFlag::NUM_FLAGS> PhasorAdvanceResult;

    //------------------------------------------------
// simplistic  phasor including crossfade
// for now, just counts integer frames in one direction
    struct FadePhasor {

        frame_t currentFrame{0};
        frame_t maxFrame{std::numeric_limits<frame_t>::max()};
        frame_t triggerFrame{0};
        bool isFadingIn{false};
        bool isFadingOut{false};
        bool isActive{false};
        float fadePhase{0.f};
        float fadeIncrement{0.01f};
        float fadeValue{0.f};

        // return true if the phasor has wrapped
        PhasorAdvanceResult Advance() {
            PhasorAdvanceResult result;
            result.Set(PhasorAdvanceResultFlag::CONTINUING);

            if (!isActive) {
                result.Set(PhasorAdvanceResultFlag::INACTIVE);
                return result;
            }
            result.Set(PhasorAdvanceResultFlag::CONTINUING);

            if (isFadingIn) {
                fadePhase += fadeIncrement;
                if (fadePhase >= 1.f) {
                    fadePhase = 1.f;
                    isFadingIn = false;
                    fadeValue = 1.f;
                    result.Set(PhasorAdvanceResultFlag::DONE_FADEIN);
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
                    result.Set(PhasorAdvanceResultFlag::DONE_FADEOUT);
                } else {
                    fadeValue = sinf(fadePhase * pi_2<float>);
                }
            }
            currentFrame++;
            if (currentFrame == triggerFrame) {
                result.Set(PhasorAdvanceResultFlag::CROSSED_TRIGGER);
            }
            if (currentFrame == maxFrame) {
                if (!isFadingOut) {
                    result.Set(PhasorAdvanceResultFlag::WRAPPED_LOOP);
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