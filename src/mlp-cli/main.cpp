#include <iostream>
#include <memory>
#include <semaphore>
#include <thread>

/// rtaudio
#include "RtAudio.h"

/// oscpack
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "ip/UdpSocket.h"

#include "../Mlp.hpp"

static const int oscRxPort = 9000;
static const int oscTxPort = 9001;

#if 0
static const int blockSize = 64;
#else // better for debug build:
static const int blockSize = 2048;
#endif

using namespace mlp;


Mlp m;
RtAudio adac;

static volatile bool shouldQuit = false;
static volatile bool isMonoInput = false;

static std::vector<float> stereoBuffer;

//-----------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------
int InitAudio() {
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = adac.getDefaultInputDevice();
    iParams.nChannels = 2;
    oParams.deviceId = adac.getDefaultOutputDevice();
    oParams.nChannels = 2;

    auto inputDeviceInfo = adac.getDeviceInfo(iParams.deviceId);
    auto outputDeviceInfo = adac.getDeviceInfo(oParams.deviceId);
    std::cout << "input device: " << inputDeviceInfo.name << std::endl;
    std::cout << "output device: " << outputDeviceInfo.name << std::endl;

    auto inch = inputDeviceInfo.inputChannels;
    if (inch < 2) {
        if (inch < 1) {
            std::cerr << "no input channels available!" << std::endl;
            return 1;
        } else {
            std::cerr << "only mono input available" << std::endl;
            isMonoInput = true;
            iParams.nChannels = 1;
            stereoBuffer.resize(blockSize * 2);
        }
    } else {
        std::cout << "using stereo input" << std::endl;
    }

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_SCHEDULE_REALTIME;
    options.priority = 90;

    unsigned int bufferFrames = blockSize;
    try {
        adac.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, &AudioCallback, &m,
                        &options);
        adac.startStream();
        std::cout << "started audio device stream " << std::endl;
        std::cout << "buffer size = " << bufferFrames << std::endl;
    } catch (std::exception &e) {
        std::cerr << "error starting audio device stream: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}


//-----------------------------------------------------------------------------------
class OscListener : public osc::OscPacketListener {

    std::unique_ptr<UdpListeningReceiveSocket> rxSocket;
    std::unique_ptr<std::thread> rxThread;

public:
    void Init() {
        rxSocket = std::make_unique<UdpListeningReceiveSocket>(
            IpEndpointName(IpEndpointName::ANY_ADDRESS, oscRxPort),
            this);

        std::cout << "listening for input on port " << oscRxPort << "...\n";

        rxThread = std::make_unique<std::thread>([&] {
            rxSocket->RunUntilSigInt();
        });
        rxThread->detach();
    }

protected:
    void ProcessMessage(const osc::ReceivedMessage &msg, const IpEndpointName &remoteEndpoint) override {
        (void) remoteEndpoint; // suppress unused parameter warning

        try {
            if (std::strcmp(msg.AddressPattern(), "/tap") == 0) {
                osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();
                int idx {0};
                args >> idx >> osc::EndMessage;
                std::cout << "tap " << idx << std::endl;
                m.Tap(static_cast<Mlp::TapId>(idx));
            } else if (std::strcmp(msg.AddressPattern(), "/fparam") == 0) {
                osc::ReceivedMessage::const_iterator arg = msg.ArgumentsBegin();
                int idx = arg++->AsInt32();
                float value = arg->AsFloat();
                std::cout << "float param " << idx << " " << value << std::endl;
                m.FloatParamChange(static_cast<Mlp::FloatParamId>(idx), value);
            } else if (std::strcmp(msg.AddressPattern(), "/iparam") == 0) {
                osc::ReceivedMessage::const_iterator arg = msg.ArgumentsBegin();
                int idx = arg++->AsInt32();
                auto value = arg->AsInt32();
                m.IndexParamChange(static_cast<Mlp::IndexParamId>(idx), value);
            } else if (std::strcmp(msg.AddressPattern(), "/bparam") == 0) {
                osc::ReceivedMessage::const_iterator arg = msg.ArgumentsBegin();
                int idx = arg++->AsInt32();
                auto value = arg->AsBool();
                m.BoolParamChange(static_cast<Mlp::BoolParamId>(idx), value);
            } else if (std::strcmp(msg.AddressPattern(), "/quit") == 0) {
                std::cout << "quit" << std::endl;
                shouldQuit = true;
            }
        } catch (osc::Exception &e) {
            std::cout << "error while parsing message: "
                      << msg.AddressPattern() << ": " << e.what() << "\n";
        }
    }
};

class OscSender {
    static constexpr int bufferSize = 1024;
    char buffer[bufferSize];

    static std::unique_ptr<UdpTransmitSocket> txSocket;
    static std::unique_ptr<std::thread> txThread;

    //std::binary_semaphore dataReady{0};

public:
    void Init() {
        txSocket = std::make_unique<UdpTransmitSocket>(
            IpEndpointName("localhost", oscTxPort));

        txThread = std::make_unique<std::thread>([&] {
            while (!shouldQuit) {

                auto &layerFlagsQ = m.GetLayerFlagsQ();
                LayerFlagsMessageData flagsData;
                while (layerFlagsQ.try_dequeue(flagsData)) {
                    /// TODO: send the flags data... (as a blob? bool array? separate messages?)
                }

                auto &layerPositionQ = m.GetLayerPositionQ();
                LayerPositionMessageData posData;
                while (layerPositionQ.try_dequeue(posData)) {
                    // TODO: send the position data...
                }

            }
        });
    }

    void SendInt(const char *address, int value) {
        osc::OutboundPacketStream p(buffer, bufferSize);
        p << osc::BeginMessage(address) << value << osc::EndMessage;
        txSocket->Send(p.Data(), p.Size());
    }

//
//    void SendFloat(const char *address, float value) {
//        osc::OutboundPacketStream p(buffer, bufferSize);
//        p << osc::BeginMessage(address) << value << osc::EndMessage;
//        txSocket->Send(p.Data(), p.Size());
//    }
//

//
//    void SendBool(const char *address, bool value) {
//        osc::OutboundPacketStream p(buffer, bufferSize);
//        p << osc::BeginMessage(address) << value << osc::EndMessage;
//        txSocket->Send(p.Data(), p.Size());
//    }
};

static OscListener listener;
//static OscSender sender;

//-----------------------------------------------------------------------------------
int main() {
    if (InitAudio()) {
        return 1;
    }

//    if (InitOsc()) {
//        return 1;
//    }

    listener.Init();

    while (!shouldQuit) {
        constexpr int waitMs = 100;
        std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
    }

    return 0;
}
