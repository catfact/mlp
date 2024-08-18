#pragma once

#include <math.h>

#include "Constants.hpp"

namespace mlp {

//----------------------------------------
// simple smoother/fader class with a binary logical value
    struct SmoothSwitch {
        // true when the switch is logically open
        bool isOpen{false};
        // true when an up/down fade is in progress
        bool isSwitching{false};
        // linear phase in [0, 1]
        float phase{0.f};
        // actual output value (likely nonlinear)
        float level{0.f};
        // per-sample phase delta
        float delta{0.01f};
        // signed delta, depends on fade direction
        float sdelta{};

        void UpdateLevel()
        {
            if (isSwitching) {
                //--------------------------------
                /// could use different curves here, or LUT, or...

                // quarter-cycle sine:
                // return sinf(phase * pi_2<float>);

                // half-cycle raised cosine:
                level = cosf((phase + 1.f) * pi<float>) * 0.5f + 0.5f;
                //--------------------------------
            } else {
                level = isOpen ? 1.f : 0.f;
            }

        }

        void Open() {
            if (isOpen) return;
            isOpen = true;
            sdelta = delta;
            isSwitching = true;
        }

        void Close() {
            if (!isOpen) return;
            isOpen = false;
            sdelta = -delta;
            isSwitching = true;
        }

        bool Toggle() {
            if (isOpen) {
                Close();
            } else {
                Open();
            }
            return isOpen;
        }

        // advances phase, updates level, returns active status
        bool Process() {
            if (!isSwitching) {
                UpdateLevel();
                return IsActive();
            }
            phase += sdelta;
            if (phase >= 1.f) {
                phase = 1.f;
                isSwitching = false;
            } else if (phase <= 0.f) {
                phase = 0.f;
                isSwitching = false;
            }
            UpdateLevel();
            return IsActive();
        }

        bool IsActive() const {
            return isOpen || isSwitching;
        }

        void SetDelta(float d) {
            delta = d;
            sdelta = isOpen ? delta : -delta;
        }
    };
}