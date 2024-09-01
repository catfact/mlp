#pragma once

#include <cassert>


#include "LayerBehavior.hpp"

#include "Outputs.hpp"
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
        SmoothSwitch clearSwitch;

        ///--- runtime state
        LoopLayerState state{LoopLayerState::STOPPED};
        bool stopPending{false};
        unsigned int currentPhasorIndex{0};
        unsigned int lastPhasorIndex{1};

        ///---- parameters
        // jump to this frame on external reset
        frame_t resetFrame{0};
        // jump to this frame when the loop finishes
        frame_t loopStartFrame{0};
        // frame on which to wrap
        frame_t loopEndFrame{bufferFrames - 1};
        // frame on which to trigger something else...
        frame_t triggerFrame{0};
        // frame at which we paused / will resume
        frame_t pauseFrame{0};

        /// levels
        float playbackLevel{1.f};
        float recordLevel{1.f};
        float preserveLevel{1.f};

        /// behavior flags
        bool loopEnabled{true};

        LayerOutputs *outputs;

        //------------------------------------------------------------------------------------------------------

        void OpenLoop(frame_t startFrame = 0) {
            loopStartFrame = startFrame;
            if (resetFrame < loopStartFrame) {
                resetFrame = loopStartFrame;
            }
            loopEndFrame = bufferFrames - 1;
            state = LoopLayerState::SETTING;
            SetWrite(true);
            assert(phasor[currentPhasorIndex].isActive == false);
            phasor[currentPhasorIndex].Reset();
            std::cout << "[LoopLayer] opened loop" << std::endl;
        }


        void CloseLoop(bool shouldUnmute = true, bool shouldDub = false) {
            state = LoopLayerState::LOOPING;
            SetRead(shouldUnmute);
            SetWrite(shouldDub);
            lastPhasorIndex = currentPhasorIndex;
            currentPhasorIndex ^= 1;

            auto &oldPhasor = phasor[lastPhasorIndex];
            auto &newPhasor = phasor[currentPhasorIndex];

            loopEndFrame = newPhasor.maxFrame = oldPhasor.maxFrame = oldPhasor.currentFrame;
            oldPhasor.isFadingOut = true;
            if (loopEnabled) {
                newPhasor.Reset(loopStartFrame);
            }
            std::cout << "[LoopLayer] closed loop; length = " << newPhasor.maxFrame << std::endl;
        }

//        void SetWriteActive(bool active) {
//            if (active) {
//                writeSwitch.Open();
//            } else {
//                writeSwitch.Close();
//            }
//        }
//
//        void SetReadActive(bool active) {
//            if (active) {
//                readSwitch.Open();
//            } else {
//                readSwitch.Close();
//            }
//        }
//
//        void SetClearActive(bool active) {
//            if (active) {
//                clearSwitch.Open();
//            } else {
//                clearSwitch.Close();
//            }
//        }

        bool ToggleWrite() {
            return writeSwitch.Toggle();
        }

        bool ToggleRead() {
            return readSwitch.Toggle();
        }

        bool ToggleClear() {
            return clearSwitch.Toggle();
        }

        void SetWrite(bool active) {
            if (active) {
                writeSwitch.Open();
            } else {
                writeSwitch.Close();
            }
        }

        void SetRead(bool active) {
            if (active) {
                readSwitch.Open();
            } else {
                readSwitch.Close();
            }
        }

        void SetClear(bool active) {
            if (active) {
                clearSwitch.Open();
            } else {
                clearSwitch.Close();
            }
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
        void ReadPhasor(float *dst, const FadePhasor &aPhasor) {
            //auto bufIdx = (phasor.currentFrame + phasor.frameOffset) % bufferFrames;
            auto bufIdx = aPhasor.currentFrame % bufferFrames;
            for (unsigned int ch = 0; ch < numChannels; ++ch) {
                auto x = buffer[(bufIdx * numChannels) + ch];
                auto a = aPhasor.fadeValue * readSwitch.level * playbackLevel;
                *(dst + ch) += x * a;
            }
        }

        // given a phasor, write to the buffer according to its frame position,
        // scaling record/preserve levels by its fade value
        void WritePhasor(const float *src, const FadePhasor &aPhasor) {
            auto bufIdx = aPhasor.currentFrame % bufferFrames;
            /// FIXME: maybe linear inversion of the switch is not ideal
            float modPreserve = preserveLevel * (1-clearSwitch.level);
            /// we want to modulate the preserve level towards unity as the phasor fades out
            modPreserve += (1.f - modPreserve) * (1.f - aPhasor.fadeValue);
            /// and also as the write switch disengages
            /// (TODO: maybe profile this conditional)
            // if (!writeSwitch.isOpen && writeSwitch.isSwitching) {
                float switchLevel = writeSwitch.level;
                modPreserve += (1.f - modPreserve) * (1 - switchLevel);
            // }
            for (unsigned int ch = 0; ch < numChannels; ++ch) {
                float x = *(src + ch);
                float modRecord = recordLevel * aPhasor.fadeValue;
                modRecord *= writeSwitch.level;
                x *= modRecord;
                float y = buffer[(bufIdx * numChannels) + ch];
                y *= modPreserve;
                x += y;
                buffer[(bufIdx * numChannels) + ch] = x;
            }
        }

        PhasorAdvanceResult ProcessFrame(const float *src, float *dst) {
            if (state == LoopLayerState::STOPPED) {
                PhasorAdvanceResult result;
                result.Set(PhasorAdvanceResultFlag::INACTIVE);
                return result;
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
            clearSwitch.Process();

            // assert(lastPhasorIndex != currentPhasorIndex);
            // this is happening sometimes when the layer is stopped
            ///... i don't think anything too terrible will happen:
            if (lastPhasorIndex == currentPhasorIndex) {
                currentPhasorIndex = lastPhasorIndex ^ 1;
            }

            phasor[lastPhasorIndex].Advance();
            auto result = phasor[currentPhasorIndex].Advance();

            if (result.Test(PhasorAdvanceResultFlag::DONE_FADEOUT)) {
                if (stopPending) {
                    SetRead(false);
                    SetWrite(false);
                    state = LoopLayerState::STOPPED;
                    stopPending = false;
                    outputs->flags.Set(LayerOutputFlagId::Stopped);
                }
            }

            if (result.Test(PhasorAdvanceResultFlag::WRAPPED_LOOP)) {
                if (loopEnabled) {
                    lastPhasorIndex = currentPhasorIndex;
                    currentPhasorIndex ^= 1;
                    phasor[currentPhasorIndex].Reset(loopStartFrame);
                    if (outputs) outputs->flags.Set(LayerOutputFlagId::Looped);
                }
            }

            return result;
        }

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

        void Restart() {
            Reset(0);
        }

        void Pause() {
            pauseFrame = GetCurrentFrame();
            phasor[currentPhasorIndex].isFadingOut = true;
        }

        void Resume() {
            for (unsigned int i=0; i<2; ++i) {
                auto &thePhasor = phasor[i];
                if (!thePhasor.isActive) {
                    currentPhasorIndex = i;
                    thePhasor.Reset(pauseFrame);
                    return;
                }
            }
            // shouldn't really get here
            std::cerr << "[LoopLayer] resume failed - already playing + in a crossfade?" << std::endl;
        }

        void StoreTrigger() {
            SetTriggerFrame(GetCurrentFrame());
        }

        void StoreReset() {
            SetResetFrame(GetCurrentFrame());
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


        void SetTriggerFrame(frame_t frame) {
            triggerFrame = frame;
            if (triggerFrame > loopEndFrame) {
                triggerFrame = loopEndFrame;
            }
            if (triggerFrame < loopStartFrame) {
                triggerFrame = loopStartFrame;
            }
            for (auto &thePhasor: phasor) {
                thePhasor.triggerFrame = triggerFrame;
            }
        }

        void SetFadeIncrement(float increment) {
            for (auto &thePhasor: phasor) {
                thePhasor.fadeIncrement = increment;
            }
        }

        void SetSwitchIncrement(float increment) {
            // FIXME / NB: this will only take effect on the next open/close
            writeSwitch.SetDelta(increment);
            readSwitch.SetDelta(increment);
            clearSwitch.SetDelta(increment);
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