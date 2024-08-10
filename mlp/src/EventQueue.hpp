#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

#include "readerwriterqueue/readerwriterqueue.h"

#include "Constants.hpp"

namespace mlp {
    enum class EventType : uint8_t {
        NOTE_ON,
        NOTE_OFF
    };

    struct EventPayload {
        EventType type;
        std::array<uint8_t, Constants::EVENT_DATA_SIZE> data;
    };

    // input event with system timestamp
    struct InputEvent {
        EventPayload payload;
        std::chrono::high_resolution_clock::time_point timestamp;
    };

    // an event that has been added to the pending queue for audio-thread processing
    // initial timestamp has been converted to a count of vectors to wait before processing
    struct PendingEvent {
        EventPayload payload;
        long int vectorCount;
    };

    class EventQueue {
        moodycamel::ReaderWriterQueue<InputEvent> inputEvents;
        std::vector<PendingEvent> pendingEvents;
        std::vector<PendingEvent> pendingEventsTemp;
        unsigned int latencyFloorVectors;
        std::function<void(const EventPayload&)> callback;

        float sampleRate;
        float latencyFloorSeconds;

    public:
        EventQueue() : sampleRate(48000.0f), latencyFloorSeconds(0.01f) {}

        void SetEventCallback(std::function<void(const EventPayload&)> aCallback) {
            callback = aCallback;
        }

        void SetLatencyFloorSeconds(double aLatencyFloorSeconds) {
            latencyFloorSeconds = aLatencyFloorSeconds;
        }

        void HandleInput(EventPayload &payload) {
            InputEvent event;
            event.payload = payload;
            event.timestamp = std::chrono::high_resolution_clock::now();
            inputEvents.enqueue(event);
        }

        // called at the top of each audio block from the audio thread
        void ProcessBlock() {
            // for each input event, convert timestamp to a vector offset and add to pending queue
            InputEvent event;
            int n = 0;
            while (inputEvents.try_dequeue(event)) {
                auto now = std::chrono::high_resolution_clock::now();
                auto targetTime = event.timestamp + std::chrono::duration<double>(latencyFloorSeconds);
                unsigned int vectorsToWait;
                if (now > targetTime) {
                    // we've already blown the latency budget, process this event ASAP
                    vectorsToWait = 0;
                } else {
                    double secondsToWait = std::chrono::duration<double>(targetTime - now).count();
                    vectorsToWait = static_cast<unsigned int>(secondsToWait * (double)sampleRate / (double)Constants::FRAMES_PER_VECTOR);
                }
                PendingEvent pendingEvent{
                        .payload = event.payload,
                        .vectorCount = vectorsToWait
                };
                pendingEvents.push_back(pendingEvent);
                n++;
            }
        }

        // called for each vector in a block from the audio thread
        void ProcessVector() {
            for (auto &event: pendingEvents) {
                if (event.vectorCount == 0) {
                    callback(event.payload);
                }
                event.vectorCount -= 1;

            }
        }

        void PostProcessBlock() {
            pendingEventsTemp.clear();
            for (auto &event: pendingEvents) {
                if (event.vectorCount >= 0) {
                    pendingEventsTemp.push_back(event);
                }
            }
            pendingEvents = pendingEventsTemp;
            // std::cerr << "pending events post-block : " << pendingEvents.size() << std::endl;
        }

    };
}