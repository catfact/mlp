#pragma once

#include <cassert>

#include "Phasor.hpp"
#include "Types.hpp"

namespace mlp {

    //---------------------------------------------------
    // a simple multichannel play/record structure
    template<int numChannels, frame_t bufferFrames>
    struct LoopLayer {
        std::array<FadePhasor, 2> phasor;
        int currentPhasor{0};
        int lastPhasor{1};

        frame_t startOffset;
        float *buffer;

        bool writeActive;
        bool readActive;

        float playLevel{1.f};
        float recordLevel{1.f};
        float preserveLevel{1.f};
        bool stopPending;

        enum class State {
            STOPPED,
            SETTING,
            LOOPING,
        };

        State state{State::STOPPED};

        void OpenLoop(frame_t offset = 0) {
            state = State::SETTING;
            writeActive = true;
            phasor[currentPhasor].Reset();
            startOffset = offset;
            std::cout << "[LoopLayer] opened loop" << std::endl;
        }

        void CloseLoop(bool shouldDub = false) {
            state = State::LOOPING;
            readActive = true;
            if (!shouldDub) {
                writeActive = false;
            }
            lastPhasor = currentPhasor;
            currentPhasor ^= 1;

            auto &oldPhasor = phasor[lastPhasor];
            auto &newPhasor = phasor[currentPhasor];

            newPhasor.maxFrame = oldPhasor.maxFrame = oldPhasor.currentFrame;
            oldPhasor.isFadingOut = true;
            newPhasor.Reset();
            std::cout << "[LoopLayer] closed loop; length = " << newPhasor.maxFrame << std::endl;
        }

        bool ToggleWrite() {
            writeActive = !writeActive;
            std::cout << "[LoopLayer] toggled write, new state = " << writeActive << std::endl;
            return writeActive;
        }

        void Stop() {
//            readActive = false;
//            writeActive = false;
//            state = State::STOPPED;
//            std::cout << "[LoopLayer] stopped loop" << std::endl;
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
                *(dst + ch) += buffer[(bufIdx * numChannels) + ch] * phasor.fadeValue;
            }
        }

        // given a phasor, write to the buffer according to its frame position,
        // scaling record/preserve levels by its fade value
        void WritePhasor(const float *src, const FadePhasor &phasor) {
            // auto bufIdx = (phasor.currentFrame + phasor.frameOffset) % bufferFrames;
            auto bufIdx = phasor.currentFrame % bufferFrames;
            for (int ch = 0; ch < numChannels; ++ch) {
                float x = *(src + ch);
                x *= recordLevel * phasor.fadeValue;
                float fb = buffer[bufIdx];
                /// FIXME: the ideal record/preserve balance is tricky to determine,
                /// but probably not this
                fb *= preserveLevel * (1.f - phasor.fadeValue);
                x += fb;
                buffer[(bufIdx * numChannels) + ch] = x;
            }
        }

        // return true if the loop wraps after this frame
        bool ProcessFrame(const float *src, float *dst) {
            if (state == State::STOPPED) {
                return false;
            }
            if (readActive) {
                for (const auto &thePhasor: phasor) {
                    if (thePhasor.isActive) {
                        ReadPhasor(dst, thePhasor);
                    }
                }
            }
            if (writeActive) {
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
                case FadePhasor::AdvanceResult::INACTIVE:
                case FadePhasor::AdvanceResult::CONTINUING:
                case FadePhasor::AdvanceResult::DONE_FADEIN:
                    return false;
                case FadePhasor::AdvanceResult::DONE_FADEOUT:
                    if (stopPending) {
                        readActive = false;
                        writeActive = false;
                        state = State::STOPPED;
                        stopPending = false;
                        std::cout << "[LoopLayer] stopped loop" << std::endl;
                    }
                    return false;
                case FadePhasor::AdvanceResult::WRAPPED:
                    // std::cout << "[LoopLayer] phasor wrapped; reset last phasor and switch" << std::endl;
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