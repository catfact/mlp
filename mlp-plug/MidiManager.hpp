#pragma once

#include <juce_audio_devices/juce_audio_devices.h>


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

        auto description = message.getDescription();
        std::cout << "received midi message (source: " << source->getName() << "): " << description << std::endl;
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