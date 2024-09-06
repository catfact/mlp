#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include "mlp/Weaver.hpp"

#include "MidiManager.hpp"


class MidiManager : public juce::MidiInputCallback,
                    public juce::Timer {

private:
    juce::AudioDeviceManager deviceManager;
    juce::Array<juce::MidiDeviceInfo> midiDevices;

public:
    MidiManager() {
        startTimerHz(1);
    }

    void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message) override {
        auto w = mlp::Weaver::GetInstance();
//        auto description = message.getDescription();
//        std::cout << "received midi message (source: " << source->getName() << "): " << description << std::endl;
        (void)source;
        auto *b = message.getRawData();
        auto sz = message.getRawDataSize();
        w->HandleMidiMessage(b, sz);


//        if (message.isNoteOn()) {
//            std::cout << "note on: " << message.getNoteNumber() << " velocity: " << message.getVelocity() << std::endl;
//        } else if (message.isNoteOff()) {
//            std::cout << "note off: " << message.getNoteNumber() << " velocity: " << message.getVelocity() << std::endl;
//        } else if (message.isController()) {
//            std::cout << "controller: " << message.getControllerNumber() << " value: " << message.getControllerValue() << std::endl;
//        } else if (message.isPitchWheel()) {
//            std::cout << "pitch wheel: " << message.getPitchWheelValue() << std::endl;
//        } else if (message.isAftertouch()) {
//            std::cout << "aftertouch: " << message.getAfterTouchValue() << std::endl;
//        } else if (message.isChannelPressure()) {
//            std::cout << "channel pressure: " << message.getChannelPressureValue() << std::endl;
//        } else if (message.isSysEx()) {
//            std::cout << "sysex: " << message.getSysExData() << std::endl;
//        } else {
//            std::cout << "unhandled message type" << std::endl;
//        }
    }

    void timerCallback() override {
        // check for connected MIDI devices
        auto midiInputs = juce::MidiInput::getAvailableDevices();
        for (auto &device: midiInputs) {
            // std::cout << "MIDI input device: " << device.name << std::endl;
            if (!deviceManager.isMidiInputDeviceEnabled(device.identifier)) {
                std::cout << "enabling new midi device:" << device.name << std::endl;
                deviceManager.setMidiInputDeviceEnabled(device.identifier, true);
            }
            if (!midiDevices.contains(device)) {
                std::cout << "new MIDI input device (enabled):" << device.name << std::endl;
                midiDevices.add(device);
                deviceManager.addMidiInputDeviceCallback (device.identifier, this);
            }
        }

        // check for disconnected MIDI devices
        for (auto &device: midiDevices) {
            if (!midiInputs.contains(device)) {
                std::cout << "removing MIDI input device:" << device.name << std::endl;
                deviceManager.removeMidiInputDeviceCallback (device.identifier, this);
                midiDevices.remove(&device);
            }
        }
    }

};