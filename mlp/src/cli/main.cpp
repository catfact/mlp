#include <iostream>
#include <rtaudio/RtAudio.h>
#include <lo/lo_cpp.h>
#include <thread>

#include "../Mlp.hpp"

using namespace mlp;

Mlp m;
RtAudio adac;
static volatile bool quit;

int AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime,
                   RtAudioStreamStatus status, void *userData) {
    m.ProcessAudioBlock(static_cast<float *>(inputBuffer), static_cast<float *>(outputBuffer), nBufferFrames);
    return 0;
}

int InitOsc() {
    int oscPort = 9000;
    lo::ServerThread st(oscPort);
    if (!st.is_valid()) {
        std::cout << "failed to create OSC server on port " << oscPort << std::endl;
        return 1;
    }

    st.set_callbacks([&st]() { printf("OSC server thread init: %p.\n", &st); },
                     []() { printf("OSC server thread cleanup.\n"); });

    std::cout << "OSC URL: " << st.url() << std::endl;

    st.add_method("tap", "i", [](lo_arg **argv, int) {
        int idx = argv[0]->i;
        std::cout << "tap " << idx << std::endl;
        m.Tap(static_cast<Mlp::TapId>(idx));
    });

    st.add_method("quit", "i", [](lo_arg **argv, int) {
        quit = true;
    });

    st.start();
    return 0;
}

int InitAudio() {
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = adac.getDefaultInputDevice();
    iParams.nChannels = 2;
    iParams.firstChannel = 0;
    oParams.deviceId = adac.getDefaultOutputDevice();
    oParams.nChannels = 2;
    oParams.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_SCHEDULE_REALTIME;
    options.priority = 90;

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

    while (!quit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
