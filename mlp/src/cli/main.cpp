#include <iostream>
#include <rtaudio/RtAudio.h>
#include <lo/lo_cpp.h>
#include <thread>

#include "../Mlp.hpp"

using namespace mlp;

Mlp m;
RtAudio adac;

static const int oscPort = 9000;
static volatile bool shouldQuit = false;
static volatile bool isMonoInput = false;

static std::vector<float> stereoBuffer;

int AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime,
                   RtAudioStreamStatus status, void *userData) {

#if 0 // dummy check sinewave
    static const float twopi = 2 * 3.14159265358979323846f;
    static const float inc = twopi * 220.f / 48000.f;
    static float phase = 0.f;
    float y;
    auto *out = static_cast<float *>(outputBuffer);
    for (int i = 0; i < nBufferFrames; ++i) {
        y = cos(phase) * 0.1;
        phase += inc;
        out[i * 2] = y;
        out[i * 2 + 1] = y;
    }
    return 0;
#else
    //---------------------------------------------
    // FIXME: nasty hack for mono->stereo input conversion
    // we're using a std vector and allowing it to grow automatically
    // means we may have allocations on audio thread, and possible dropouts, if blocksize fluctuates upwards

    if (isMonoInput) {
        if (stereoBuffer.size() < nBufferFrames * 2) {
            stereoBuffer.resize(nBufferFrames * 2);
        }
        auto *in = static_cast<float *>(inputBuffer);
        for (int i = 0; i < nBufferFrames; ++i) {
            stereoBuffer[i * 2] = in[i];
            stereoBuffer[i * 2 + 1] = in[i];
        }
        inputBuffer = stereoBuffer.data();
        m.ProcessAudioBlock(stereoBuffer.data(), static_cast<float *>(outputBuffer), nBufferFrames);
    } else {
        m.ProcessAudioBlock(static_cast<float *>(inputBuffer), static_cast<float *>(outputBuffer), nBufferFrames);
    }
    return 0;
#endif
}

int InitOsc() {
    lo::ServerThread st(oscPort);
    if (!st.is_valid()) {
        std::cout << "failed to create OSC server on port " << oscPort << std::endl;
        return 1;
    }

//    st.set_callbacks([&st]() { printf("OSC server thread init: %p.\n", &st); },
//                     []() { printf("OSC server thread cleanup.\n"); });

    std::cout << "OSC URL: " << st.url() << std::endl;

    st.add_method("tap", "i", [](lo_arg **argv, int) {
        int idx = argv[0]->i;
        std::cout << "tap " << idx << std::endl;
        m.Tap(static_cast<Mlp::TapId>(idx));
    });

    st.add_method("quit", "i", [](lo_arg **argv, int) {
        std::cout << "quit " << std::endl;
        shouldQuit = true;
    });

    st.start();
    return 0;
}

int InitAudio() {
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = adac.getDefaultInputDevice();
    iParams.nChannels = 2;
    oParams.deviceId = adac.getDefaultOutputDevice();
    oParams.nChannels = 2;

    auto inputDeviceInfo = adac.getDeviceInfo(iParams.deviceId);
    auto inch =inputDeviceInfo.inputChannels;
    if (inch < 2) {
        if (inch < 1) {
            std::cerr << "no input channels available!" << std::endl;
            return 1;
        } else {
            std::cerr << "only mono input available" << std::endl;
            isMonoInput = true;
            iParams.nChannels = 1;
            // make resizing less likely by allocating a good amount upfront
            stereoBuffer.resize(1<<12);
        }
    }

    RtAudio::StreamOptions options;
//    options.flags = RTAUDIO_SCHEDULE_REALTIME;
//    options.priority = 90;

    unsigned int bufferFrames = 512;
    try {
        adac.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, &AudioCallback, &m,
                        &options);
        adac.startStream();
    } catch (RtAudioError &e) {

        e.printMessage();
        return 1;
    }

    return 0;
}


int main() {

    if (InitOsc()) {
        return 1;
    }

    if (InitAudio()) {
        return 1;
    }

    while (!shouldQuit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
