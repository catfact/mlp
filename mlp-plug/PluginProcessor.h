#pragma once

#include <juce_audio_processors/juce_audio_processors.h>


#include "MidiManager.hpp"
#include "Mlp.hpp"
#include "mlp/Weaver.hpp"

//==============================================================================
class AudioPluginAudioProcessor final :
        public juce::AudioProcessor {
public:
    friend class AudioPluginAudioProcessorEditor;

    //==============================================================================
    AudioPluginAudioProcessor();

    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;

    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;

    bool producesMidi() const override;

    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;

    void changeProgramName(int index, const juce::String &newName) override;


    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;

    void setStateInformation(const void *data, int sizeInBytes) override;

private:
    mlp::Mlp mlp;
    mlp::Weaver* weaver;
    MidiManager midiManager;

    //// FIXME:
    /// rather bad hack here:
    /// the processor assumes stereo interleaved I/O.
    /// i don't know of a way to request JUCE To use interleaved buffers,
    /// and we do want to support up/down mix from mono to stereo.
    /// so, maintaining a stereo interleaved buffer for each direction.
    /// we will allow it to grow on the audio thread, which is not great, but hopefully will not happen often
    static constexpr size_t initialBufferSize = 2048;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;

    void InterleaveInput(const float *srcL, const float *srcR, unsigned int numSamples);

    void InterleaveOutput(float *dstL, float *dstR, unsigned int numSamples);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};

