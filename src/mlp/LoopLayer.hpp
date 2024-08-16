#pragma once

#include <cassert>

#include "Phasor.hpp"
#include "SmoothSwitch.hpp"
#include "Types.hpp"

namespace mlp {

    enum class LoopLayerState {
        STOPPED,
        SETTING,
        LOOPING,
    };

    //---------------------------------------------------
    // a simple multichannel play/record structure
    template<int numChannels, frame_t bufferFrames>
    struct LoopLayer {
        ///---- the audio buffer!
        float *buffer;

        //--- sub-processors
        std::array<FadePhasor, 2> phasor;
        SmoothSwitch writeSwitch;
        SmoothSwitch readSwitch;

        ///--- runtime state
        LoopLayerState state{LoopLayerState::STOPPED};
        bool stopPending{false};
        int currentPhasorIndex{0};
        int lastPhasorIndex{1};

        ///---- parameters
        // jump to this frame on external reset
        frame_t resetFrame{0};
        // jump to this frame when the loop finishes
        frame_t loopStartFrame{0};
        // frame on which to wrap
        frame_t loopEndFrame{bufferFrames - 1};
        // frame on which to trigger something else...
        frame_t triggerFrame{0};

        /// levels
        float playbackLevel{1.f};
        float recordLevel{1.f};
        float preserveLevel{1.f};

        /// behavior flags
        bool loopEnabled{true};
        bool syncLastLayer{true};

        void OpenLoop(frame_t startFrame=0) {
            loopStartFrame = startFrame;
            if (resetFrame < loopStartFrame) {
                resetFrame = loopStartFrame;
            }

            state = LoopLayerState::SETTING;
            SetWriteActive(true);
            phasor[currentPhasorIndex].Reset();
            std::cout << "[LoopLayer] opened loop" << std::endl;
        }

        void CloseLoop( bool shouldUnmute = true, bool shouldDub = false) {
            state = LoopLayerState::LOOPING;
            SetReadActive(shouldUnmute);
            SetWriteActive(shouldDub);
            lastPhasorIndex = currentPhasorIndex;
            currentPhasorIndex ^= 1;

            auto &oldPhasor = phasor[lastPhasorIndex];
            auto &newPhasor = phasor[currentPhasorIndex];

            loopEndFrame = newPhasor.maxFrame = oldPhasor.maxFrame = oldPhasor.currentFrame;
            oldPhasor.isFadingOut = true;
            newPhasor.Reset(loopStartFrame);
            std::cout << "[LoopLayer] closed loop; length = " << newPhasor.maxFrame << std::endl;
        }

        void SetWriteActive(bool active)
        {
            if (active) {
                writeSwitch.Open();
            } else {
                writeSwitch.Close();
            }
        }

        void SetReadActive(bool active)
        {
            if (active) {
                readSwitch.Open();
            } else {
                readSwitch.Close();
            }
        }

        void ToggleWrite() {
            writeSwitch.Toggle();
        }

        void ToggleRead() {
            readSwitch.Toggle();
        }

        void Stop() {
            stopPending = true;
            for (auto &thePhasor: phasor) {
                if (thePhasor.isActive) {
                    thePhasor.isFadingOut = true;
                }
            }
        }

        // given a phasor, read from the buffer according to its frame position,
        // and scale the output by its fade value,
        // mixing into the given interleaved audio frame
        void ReadPhasor(float *dst, const FadePhasor &phasor) {
            //auto bufIdx = (phasor.currentFrame + phasor.frameOffset) % bufferFrames;
            auto bufIdx = phasor.currentFrame % bufferFrames;
            for (int ch = 0; ch < numChannels; ++ch) {
                *(dst + ch) += buffer[(bufIdx * numChannels) + ch] * phasor.fadeValue * readSwitch.level * playbackLevel;
            }
        }

        // given a phasor, write to the buffer according to its frame position,
        // scaling record/preserve levels by its fade value
        void WritePhasor(const float *src, const FadePhasor &phasor) {
            // auto bufIdx = (phasor.currentFrame + phasor.frameOffset) % bufferFrames;
            auto bufIdx = phasor.currentFrame % bufferFrames;
            float modPreserve = preserveLevel;
            /// we want to modulate the preserve level towards unity as the phasor fades out
            modPreserve += (1.f - modPreserve) * (1.f - phasor.fadeValue);
            /// and also as the write switch disengages
            if (!writeSwitch.isOpen && writeSwitch.isSwitching) {
                float switchLevel = writeSwitch.level;
                modPreserve += (1.f - modPreserve) * (1 - switchLevel);
            }
            for (int ch = 0; ch < numChannels; ++ch) {
                float x = *(src + ch);
                float modRecord = recordLevel * phasor.fadeValue;
                modRecord *= writeSwitch.level;
                x *= modRecord;
                float y = buffer[(bufIdx * numChannels) + ch];
                y *= modPreserve;
                x += y;
                buffer[(bufIdx * numChannels) + ch] = x;
            }
        }

        // return true if the loop wraps after this frame
        bool ProcessFrame(const float *src, float *dst) {
            if (state == LoopLayerState::STOPPED) {
                return false;
            }
            if (readSwitch.Process()) {
                for (const auto &thePhasor: phasor) {
                    if (thePhasor.isActive) {
                        ReadPhasor(dst, thePhasor);
                    }
                }
            }
            if (writeSwitch.Process()) {
                for (const auto &thePhasor: phasor) {
                    if (thePhasor.isActive) {
                        WritePhasor(src, thePhasor);
                    }
                }
            }

            assert(lastPhasorIndex != currentPhasorIndex);

            phasor[lastPhasorIndex].Advance();
            auto result = phasor[currentPhasorIndex].Advance();
            switch (result)
            {
                case PhasorAdvanceResult::INACTIVE:
                case PhasorAdvanceResult::CONTINUING:
                case PhasorAdvanceResult::DONE_FADEIN:
                    return false;
                case PhasorAdvanceResult::DONE_FADEOUT:
                    if (stopPending) {
                        SetReadActive(false);
                        SetWriteActive(false);
                        state = LoopLayerState::STOPPED;
                        stopPending = false;
                        std::cout << "[LoopLayer] stopped loop" << std::endl;
                    }
                    return false;
                case PhasorAdvanceResult::WRAPPED_LOOP:
                    if (loopEnabled) {
                        lastPhasorIndex = currentPhasorIndex;
                        currentPhasorIndex ^= 1;
                        phasor[currentPhasorIndex].Reset(loopStartFrame);
                    }
                    return true;
                case PhasorAdvanceResult::CROSSED_TRIGGER:
                    /// TODO: perform a trigger action?
                    return false;
            }
        };

        void Reset() {
            lastPhasorIndex = currentPhasorIndex;
            currentPhasorIndex ^= 1;
            phasor[currentPhasorIndex].maxFrame = loopEndFrame;
            phasor[lastPhasorIndex].isFadingOut = true;
            phasor[currentPhasorIndex].Reset(resetFrame);
        }

        void Reset(frame_t frame) {
            SetResetFrame(frame);
            Reset();
        }

        frame_t GetCurrentFrame() const {
            return phasor[currentPhasorIndex].currentFrame;
        }

        void SetResetFrame(frame_t frame) {
            resetFrame = frame;
            if (resetFrame > loopEndFrame) {
                resetFrame = loopEndFrame;
            }
            if (resetFrame < loopStartFrame) {
                resetFrame = loopStartFrame;
            }
        }
        void SetFadeIncrement(float increment) {
            for (auto &thePhasor: phasor) {
                thePhasor.fadeIncrement = increment;
            }
        }

        bool GetIsActive() const {
            if (state == LoopLayerState::STOPPED) {
                return false;
            }
            if (stopPending) {
                return false;
            }
            return true;
        }

    };

}