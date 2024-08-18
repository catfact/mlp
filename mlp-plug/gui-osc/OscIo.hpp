#pragma once

#include <juce_osc/juce_osc.h>
#include "MlpGuiIo.hpp"

template<int portNumber = 8000>
class OscInput :
        public MlpGuiInput,
        private juce::OSCReceiver,
        private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback> {
public:
    OscInput() {
        if (!connect(portNumber)) {
            std::cerr << "failed to connect to port " << portNumber << std::endl;
        }

        addListener(this, "/layer/output/flag");
    }

    void oscMessageReceived(const juce::OSCMessage &message) override {
        if (message.size() == 1 && message[0].isInt32() && message[1].isInt32())
            HandleLayerOutputFlag(message[0].getInt32(), message[1].getInt32());
    }
};

template<int portNumber = 8001>
class OscOutput : public MlpGuiOutput {
    juce::OSCSender sender;

    OscOutput() {
        if (!sender.connect("127.0.0.1", portNumber)) {
            std::cerr << "failed to connect to port " << portNumber << std::endl;
        }
    }

    void SendTap(mlp::Mlp::TapId id) override {
        sender.send("/tap", id);
    }

    void SendBool(mlp::Mlp::BoolParamId id, bool value) override {
        sender.send("/param/bool", id, value);
    }

    void SendIndexBool(mlp::Mlp::IndexBoolParamId id, int index, bool value) override {
        sender.send("/param/index/bool", id, index, value);
    }

    void SendIndexFloat(mlp::Mlp::IndexFloatParamId id, int index, float value) override {
        sender.send("/param/index/float", id, index, value);
    }
};