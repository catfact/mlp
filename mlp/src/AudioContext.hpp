#pragma once

#include <array>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>

#include "Constants.hpp"

// singleton class handling final audio output
class AudioContext {
public:
    typedef float sample_t;

    static constexpr int blockFrames = mlp::Constants::FRAMES_PER_BLOCK;
    static constexpr int numOutputChannels = 2;

    static constexpr size_t sampleBytes = sizeof(sample_t);
    static constexpr size_t frameBytes = sampleBytes * numOutputChannels;
    static constexpr size_t blockBytes = blockFrames * frameBytes;

    // typedef std::array<sample_t, blockFrames * 2> StereoInterleavedBuffer;

private:
    size_t framesLeftInBlock;

    sample_t outBuffer[blockFrames * numOutputChannels]{0};
    sample_t *src;

    std::function<void(sample_t *)> blockProcessCallback;

    double sampleRate{};

public:
    AudioContext() : framesLeftInBlock(blockFrames), src(outBuffer) {
        blockProcessCallback = [](sample_t *buf) {};
    }

    void SetBlockProcessCallback(std::function<void(sample_t *)> callback) {
        blockProcessCallback = std::move(callback);
    }

    void ClearOutput() {
        memset(outBuffer, 0, blockBytes);
    }

    // output buffer is assumed to be stereo interleaved floats
    void MainAudioCallback(sample_t *dst, size_t numFrames) {
        while (numFrames >= framesLeftInBlock) {
            const size_t bytesLeftInBlock = framesLeftInBlock * frameBytes;
            memcpy(dst, src, bytesLeftInBlock);
            dst += framesLeftInBlock * numOutputChannels;
            numFrames -= framesLeftInBlock;
            framesLeftInBlock = blockFrames;
            ClearOutput();
            blockProcessCallback(outBuffer);
            src = outBuffer;
        }
        // remainder
        memcpy(dst, src, frameBytes * numFrames);
        framesLeftInBlock -= numFrames;
        src += numOutputChannels * numFrames;
    }

    void SetSampleRate(double aSampleRate) {
        sampleRate = aSampleRate;
    }

    static void ClearAudioBuffer(sample_t *buf,
                                 size_t numFrames = AudioContext::blockFrames,
                                 int numChannels = AudioContext::numOutputChannels) {
        for (size_t frame = 0; frame < numChannels * numFrames; ++frame) {
            *buf++ = static_cast<sample_t>(0);
        }
    }

    static void CopyAudioBufferMixing(sample_t *dst, const sample_t *src,
                                      size_t numFrames = AudioContext::blockFrames,
                                      int numChannels = AudioContext::numOutputChannels) {
        for (size_t frame = 0; frame < numChannels * numFrames; ++frame) {
            *dst++ += *src++;
        }
    }

    static void CopyAudioBufferReplacing(sample_t *dst, const sample_t *src,
                                         size_t numFrames = AudioContext::blockFrames,
                                         int numChannels = AudioContext::numOutputChannels) {
        for (size_t frame = 0; frame < numChannels * numFrames; ++frame) {
            *dst++ = *src++;
        }
    }

    struct AmplitudeRange {
        float min;
        float max;
    };

    static AmplitudeRange ComputeAmplitudeRange(sample_t *buf,
                                                size_t numFrames = AudioContext::blockFrames,
                                                int numChannels = AudioContext::numOutputChannels) {
        sample_t min = std::numeric_limits<sample_t>::max();
        sample_t max = std::numeric_limits<sample_t>::lowest();
        for (size_t idx = 0; idx < (numChannels * numFrames); ++idx) {
            sample_t x = *buf++;
            if (x < min) { min = x; }
            if (x > max) { max = x; }
        }
        return {min, max};
    }

    [[nodiscard]]

    double GetSampleRate() const { return sampleRate; }

    static void ScaleAudioBuffer(float *buf, const float amp,
                                 size_t numFrames = AudioContext::blockFrames,
                                 int numChannels = AudioContext::numOutputChannels) {
        for (size_t frame = 0; frame < numChannels * numFrames; ++frame) {
            *buf++ *= amp;
        }
    }
};