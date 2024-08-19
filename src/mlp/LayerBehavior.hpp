#pragma once

#include <array>
#include <array>
#include <functional>
#include <utility>

#include "Constants.hpp"
#include "LoopLayer.hpp"
#include "Types.hpp"

namespace mlp {

    enum class LayerActionId {
        Reset,
        Restart,
        Pause,
        Resume,
        StoreTrigger,
        StoreReset,
        NUM_ACTIONS
    };

    enum class LayerConditionId {
        OpenLoop,
        CloseLoop,
        Trigger,
        Wrap,
        NUM_CONDITIONS
    };

//    struct LayerConditionState {
//        bool flag{false};
//        int counter{-1};
//
//        void Clear() {
//            flag = false;
//            counter = -1;
//        }
//    };

    struct LayerInterface {
        //std::array<LayerConditionState, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditions;
        std::array<int, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionCounter{-1};
        std::array<std::function<void()>, static_cast<size_t>(LayerActionId::NUM_ACTIONS)> actions;
        bool *loopEnabled{nullptr};

        void SetLayer(LoopLayer<numLoopChannels, bufferFrames> *layer) {
            actions[static_cast<size_t>(LayerActionId::Reset)] = [layer]() { layer->Reset(); };
            actions[static_cast<size_t>(LayerActionId::Restart)] = [layer]() { layer->Restart(); };
            actions[static_cast<size_t>(LayerActionId::Pause)] = [layer]() { layer->Pause(); };
            actions[static_cast<size_t>(LayerActionId::Resume)] = [layer]() { layer->Resume(); };
            actions[static_cast<size_t>(LayerActionId::StoreTrigger)] = [layer]() { layer->StoreTrigger(); };
            actions[static_cast<size_t>(LayerActionId::StoreReset)] = [layer]() { layer->StoreReset(); };
            loopEnabled = &layer->loopEnabled;
        }
    };

    typedef std::function<void(LayerInterface *, LayerInterface *, LayerInterface *, bool, bool)> LayerActionFunction;

    // defines the conditions->actions mapping for a given layer
    struct LayerBehavior {
        std::array<LayerActionFunction, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionAction;
        LayerInterface *thisLayer;
        LayerInterface *layerBelow;
        LayerInterface *layerAbove;

        void Clear() {
            for (auto &action: conditionAction) {
                action = [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove,
                            bool isInnerMostLayer, bool isOuterMostLayer) {};
            }
            for (auto &counter: thisLayer->conditionCounter) {
                counter = -1;
            }
        }

        void ProcessCondition(LayerConditionId id, bool isInnerMostLayer, bool isOuterMostLayer) {
            int &counter = thisLayer->conditionCounter[static_cast<size_t>(id)];
            if (counter == 0) {
                // no actions taken if the counter has run down
                return;
            }
            if (counter > 0) {
                // decrement if it's positive; negative counter is ignored
                counter--;
            }
            auto &action = conditionAction[static_cast<size_t>(id)];
            action(thisLayer, layerBelow, layerAbove, isInnerMostLayer, isOuterMostLayer);
        }

    };


    enum class LayerBehaviorModeId {
        ASYNC,
        MULTIPLY_UNQUANTIZED,
        MULTIPLY_QUANTIZED,
        MULTIPLY_QUANTIZED_START,
        MULTIPLY_QUANTIZED_END,
        INSERT_UNQUANTIZED,
        INSERT_QUANTIZED,
        INSERT_QUANTIZED_START,
        INSERT_QUANTIZED_END,
    };

    //------------------------------------------------
    static void SetLayerBehaviorMode(LayerBehavior &behavior, LayerBehaviorModeId modeId) {
        behavior.Clear();
        switch (modeId) {
            case LayerBehaviorModeId::ASYNC:
                // nothing to do! (i think...)
                break;
            case LayerBehaviorModeId::MULTIPLY_UNQUANTIZED:
                behavior.conditionAction[static_cast<size_t>(LayerConditionId::OpenLoop)] =
                        [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove,
                           bool isInnerMostLayer, bool isOuterMostLayer) {
                            if (!isInnerMostLayer) {
                                layerBelow->actions[static_cast<size_t>(LayerActionId::StoreReset)]();
                            }
                        };
                behavior.conditionAction[static_cast<size_t>(LayerConditionId::Wrap)] =
                        [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove,
                           bool isInnerMostLayer, bool isOuterMostLayer) {
                            if (!isInnerMostLayer) {
                                layerBelow->actions[static_cast<size_t>(LayerActionId::Reset)]();
                            }
                        };
                break;
            case LayerBehaviorModeId::INSERT_UNQUANTIZED:
                /// totally untested
                *behavior.thisLayer->loopEnabled = false;
                *behavior.layerBelow->loopEnabled = false;
                behavior.conditionAction[static_cast<size_t>(LayerConditionId::OpenLoop)] =
                        [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove,
                           bool isInnerMostLayer, bool isOuterMostLayer) {
                            if (!isInnerMostLayer) {
                                layerBelow->actions[static_cast<size_t>(LayerActionId::StoreTrigger)]();
                            }
                        };
                behavior.conditionAction[static_cast<size_t>(LayerConditionId::Wrap)] =
                        [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove,
                           bool isInnerMostLayer, bool isOuterMostLayer) {
                            if (!isInnerMostLayer) {
                                layerBelow->actions[static_cast<size_t>(LayerActionId::Resume)]();
                            }
                        };
                behavior.conditionAction[static_cast<size_t>(LayerConditionId::Trigger)] =
                        [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove,
                           bool isInnerMostLayer, bool isOuterMostLayer) {
                            if (!isOuterMostLayer) {
                                layerAbove->actions[static_cast<size_t>(LayerActionId::Reset)]();
                                thisLayer->actions[static_cast<size_t>(LayerActionId::Pause)]();
                            }

                        };
                break;
            default:
                // NYI mode
                break;
        }
    }
}
