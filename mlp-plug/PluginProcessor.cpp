
#include <iostream>

#include "ScriptManager.hpp"
#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
        : AudioProcessor(BusesProperties()
                                 .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                 .withOutput("Output", juce::AudioChannelSet::stereo(), true)
),
          inputBuffer(initialBufferSize),
          outputBuffer(initialBufferSize) {

    //const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);
    auto mainScript = ScriptManager::GetMainScript();
    mlp::Weaver::Init(&mlp, mainScript);
    delete mainScript;
    weaver = mlp::Weaver::GetInstance();
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() = default;

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() {
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources() {
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
//  #if JucePlugin_IsMidiEffect
//    juce::ignoreUnused (layouts);
//    return true;
//  #else
//    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
//     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
//        return false;
//
//   #if ! JucePlugin_IsSynth
//    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
//        return false;
//   #endif
//
//    return true;
//  #endif
    auto numInputChannels = layouts.getMainInputChannelSet().size();
    auto numOutputChannels = layouts.getMainOutputChannelSet().size();
    return numInputChannels > 0 && numOutputChannels > 0;
}


void AudioPluginAudioProcessor::InterleaveInput(const float* srcL, const float* srcR, unsigned int numSamples) {
    if (numSamples > inputBuffer.size()) {
        /// NB/FIXME: could speed this up using c++20 bit_ciel() or twiddle hacks
        /// but, we don't want to be hitting this for performance anyway
        size_t sz = inputBuffer.size();
        while (sz < numSamples) sz <<= 1;
        inputBuffer.resize(sz);
    }
    float* dst = inputBuffer.data();
    for (unsigned int i = 0; i < numSamples; ++i) {
        *dst++ = *srcL++;
        *dst++ = *srcR++;
    }
}

void AudioPluginAudioProcessor::InterleaveOutput(float *dstL, float *dstR, unsigned int numSamples) {
    if (numSamples > outputBuffer.size()) {
        /// NB/FIXME: could speed this up using c++20 bit_ciel() or twiddle hacks
        /// but, we don't want to be hitting this for performance anyway
        size_t sz = outputBuffer.size();
        while (sz < numSamples) sz <<= 1;
        outputBuffer.resize(sz);
    }
    float* src = outputBuffer.data();
    for (unsigned int i = 0; i < numSamples; ++i) {
        *dstL++ = *src++;
        *dstR++ = *src++;
    }
}


void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages) {
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

//    for (auto i = 0; i < totalNumOutputChannels; ++i)
//        buffer.clear(i, 0, buffer.getNumSamples());

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    auto numFrames = static_cast<unsigned int>(buffer.getNumSamples());
    const float *srcL = buffer.getReadPointer(0);
    const float *srcR = buffer.getReadPointer(totalNumInputChannels > 1 ? 1 : 0);
    float *dstL = buffer.getWritePointer(0);
    float *dstR = buffer.getWritePointer(totalNumOutputChannels > 1 ? 1 : 0);

    InterleaveInput(srcL, srcR, numFrames);
    mlp.ProcessAudioBlock(inputBuffer.data(), outputBuffer.data(), numFrames);
    InterleaveOutput(dstL, dstR, numFrames);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor() {
    return new AudioPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    juce::ignoreUnused(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new AudioPluginAudioProcessor();
}
