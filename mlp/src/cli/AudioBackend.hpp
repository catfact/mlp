#pragma once

#include <functional>
#include <memory>

#include <rtaudio/RtAudio.h>

#include "../AudioContext.hpp"

/// global audio API manager
/// currently wrapping RtAudio

namespace mlp {
    class AudioBackend {
        static AudioContext audioContext;
        static std::unique_ptr<RtAudio> dac;

        static int BackendAudioCallback(void *outputBuffer,
                                        __attribute__((unused)) void *inputBuffer,
                                        unsigned int numFrames,
                                        __attribute__((unused)) double streamTime,
                                        __attribute__((unused)) RtAudioStreamStatus status,
                                        __attribute__((unused)) void *data) {

            //callback(static_cast<float *>(outputBuffer), numFrames);
            audioContext.MainAudioCallback(static_cast<float *>(outputBuffer), numFrames);
            return 0;
        }

    public:

        static AudioContext &GetAudioContext() { return audioContext; }

        static int Initialize() {
            dac = std::make_unique<RtAudio>(RtAudio::UNSPECIFIED);

            ////////////
            //// TODO: parameterize / check
            const int channels = 2;
            const int fs = 48000;
            unsigned int bufferFrames = 512;
            ////////////

            RtAudio::StreamParameters oParams;
            oParams.nChannels = channels;
            oParams.firstChannel = 0;
            auto outputDeviceId =dac->getDefaultOutputDevice();

//=============================
//== FIXME: wrong rtaudio API version, or something
//=============================
//            ///----------
//            /// TODO: parameterize audio device name
//            auto deviceNames = dac->getDeviceNames();
//            auto deviceIds = dac->getDeviceIds();
//            for (int i=0; i<deviceNames.size(); ++i) {
//                auto id = deviceIds[i];
//                if (outputDeviceId == id) {
//                    std::cout << "selected output device: " << deviceNames[i] << std::endl;
//                }
//            }
//            ///////
//
//            oParams.deviceId = outputDeviceId;
//
//            RtAudio::StreamOptions options;
//                options.flags = RTAUDIO_HOG_DEVICE;
//            options.flags |= RTAUDIO_SCHEDULE_REALTIME;
//            // options.flags |= RTAUDIO_NONINTERLEAVED;
//
//
//            if (dac->openStream(&oParams,
//                                nullptr,
//                                RTAUDIO_FLOAT32,
//                                fs,
//                                &bufferFrames,
//                                &BackendAudioCallback,
//                                nullptr,
//                                &options)) {
//                std::cout << dac->getErrorText() << std::endl;
//            }

            audioContext.SetSampleRate(dac->getStreamSampleRate());

            if (!dac->isStreamOpen()) {
                std::cerr << "[AudioContext] failed to open DAC" << std::endl;
                Cleanup();
                return -1;
            }

//=============================
//== FIXME: wrong rtaudio API version, or something
//=============================
//            if (dac->startStream()) {
//                std::cerr << "[AudioContext] failed to start stream" << std::endl;
//                Cleanup();
//                return -1;
//            }

            return 0;
        }

        static void Cleanup() {
            if (dac->isStreamRunning())
                dac->stopStream();  // or could call dac->abortStream();


            if (dac->isStreamOpen()) dac->closeStream();
        }

    };
}