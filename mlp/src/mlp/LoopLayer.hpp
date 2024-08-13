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
        std::array<FadePhasor, 2> phasor;
        int currentPhasor{0};
        int lastPhasor{1};

        frame_t startOffset;
        float *buffer;

//        bool writeActive;
//        bool readActive;
        SmoothSwitch writeSwitch;
        SmoothSwitch readSwitch;

        float playbackLevel{1.f};
        float recordLevel{1.f};
        float preserveLevel{1.f};

        bool stopPending;


        LoopLayerState state{LoopLayerState::STOPPED};

        void OpenLoop(frame_t offset = 0) {
            state = LoopLayerState::SETTING;
            SetWriteActive(true);
            phasor[currentPhasor].Reset();
            startOffset = offset;
            std::cout << "[LoopLayer] opened loop" << std::endl;
        }

        void CloseLoop( bool shouldUnmute = true, bool shouldDub = false) {
            state = LoopLayerState::LOOPING;
            SetReadActive(shouldUnmute);
            SetWriteActive(shouldDub);
            lastPhasor = currentPhasor;
            currentPhasor ^= 1;

            auto &oldPhasor = phasor[lastPhasor];
            auto &newPhasor = phasor[currentPhasor];

            newPhasor.maxFrame = oldPhasor.maxFrame = oldPhasor.currentFrame;
            oldPhasor.isFadingOut = true;
            newPhasor.Reset();
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
                *(dst + ch) += buffer[(bufIdx * numChannels) + ch] * phasor.fadeValue * readSwitch.level;
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

            assert(lastPhasor != currentPhasor);

            phasor[lastPhasor].Advance();
            auto result = phasor[currentPhasor].Advance();
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
                case PhasorAdvanceResult::WRAPPED:
                    phasor[lastPhasor].Reset();
                    lastPhasor = currentPhasor;
                    currentPhasor ^= 1;
                    return true;
            }
        };

        void Reset() {
            lastPhasor = currentPhasor;
            currentPhasor ^= 1;
            phasor[currentPhasor].maxFrame = phasor[lastPhasor].maxFrame;
            phasor[lastPhasor].isFadingOut = true;
            phasor[currentPhasor].Reset(startOffset);
        }

        frame_t GetCurrentFrame() const {
            return phasor[currentPhasor].currentFrame;
        }
    };

}