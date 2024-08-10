#include "AudioBackend.hpp"

using namespace lojit;

AudioContext AudioBackend::audioContext;
std::unique_ptr<RtAudio> AudioBackend::dac;