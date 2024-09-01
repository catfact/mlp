#pragma once

#include <array>
#include <array>
#include <functional>
#include <utility>

#include "Constants.hpp"
#include "LoopLayer.hpp"
#include "Outputs.hpp"
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

    struct LayerInterface {
        std::array<int, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionCounter{-1};

        std::array<std::function<void()>, static_cast<size_t>(LayerActionId::NUM_ACTIONS)> actions;

        bool *loopEnabled{nullptr};
        bool isInner;
        bool isOuter;

        LayerOutputs *outputs{nullptr};

        void SetLayer(LoopLayer<numLoopChannels, bufferFrames> *layer) {
            actions[static_cast<size_t>(LayerActionId::Reset)] = [layer]() { layer->Reset(); };
            actions[static_cast<size_t>(LayerActionId::Restart)] = [layer]() { layer->Restart(); };
            actions[static_cast<size_t>(LayerActionId::Pause)] = [layer]() { layer->Pause(); };
            actions[static_cast<size_t>(LayerActionId::Resume)] = [layer]() { layer->Resume(); };
            actions[static_cast<size_t>(LayerActionId::StoreTrigger)] = [layer]() { layer->StoreTrigger(); };
            actions[static_cast<size_t>(LayerActionId::StoreReset)] = [layer]() { layer->StoreReset(); };
            loopEnabled = &layer->loopEnabled;
        }

        void DoAction(LayerActionId id) {
            actions[static_cast<size_t>(id)]();
            if (outputs) {
                switch (id) {
                    case LayerActionId::Reset:
                        outputs->flags.Set(LayerOutputFlagId::Reset);// ->SetLayerFlag(index, LayerOutputFlagId::Reset);
                        break;
                    case LayerActionId::Pause:
                        outputs->flags.Set(LayerOutputFlagId::Paused);
                        break;
                    case LayerActionId::Resume:
                        outputs->flags.Set(LayerOutputFlagId::Resumed);
                        break;
                    case LayerActionId::StoreTrigger:
                        outputs->flags.Set(LayerOutputFlagId::Triggered);
                        break;
                    case LayerActionId::StoreReset:
                        outputs->flags.Set(LayerOutputFlagId::Reset);
                        break;
                    case LayerActionId::Restart:
                        outputs->flags.Set(LayerOutputFlagId::Restarted);
                        break;
                    case LayerActionId::NUM_ACTIONS:
                    default:
                        break;
                }
            }
        }
    };

    typedef std::function<void(
            LayerInterface *, // this layer
            LayerInterface *, // layer below
            LayerInterface * // layer above
    )> LayerActionFunction;

    // defines the conditions->actions mapping for a given layer
    struct LayerBehavior {
        std::array<LayerActionFunction, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionAction;
        LayerInterface *thisLayer{};
        LayerInterface *layerBelow{};
        LayerInterface *layerAbove{};

        void Clear() {
            for (auto &action: conditionAction) {
                action = [](LayerInterface *aThisLayer, LayerInterface *aLayerBelow, LayerInterface *aLayerAbove) {
                    (void) aThisLayer;
                    (void) aLayerBelow;
                    (void) aLayerAbove;
                };
            }
            for (auto &counter: thisLayer->conditionCounter) {
                counter = -1;
            }
        }

        void ProcessCondition(LayerConditionId id) {
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
            action(thisLayer, layerBelow, layerAbove);
        }

        void SetAction(LayerConditionId id, LayerActionFunction action) {
            conditionAction[static_cast<size_t>(id)] = std::move(action);
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
                behavior.SetAction
                        (LayerConditionId::OpenLoop,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner && !layerBelow->isOuter) {
                                 layerBelow->DoAction(LayerActionId::StoreReset);
                             }
                         });
                behavior.SetAction
                        (LayerConditionId::CloseLoop,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner && !layerBelow->isOuter) {
                                 layerBelow->DoAction(LayerActionId::Reset);
                             }
                         });

                behavior.SetAction
                        (LayerConditionId::Wrap,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner && !layerBelow->isOuter) {
                                 layerBelow->DoAction(LayerActionId::Reset);
                             }
                         });
                break;

            case LayerBehaviorModeId::INSERT_UNQUANTIZED:
                /// totally untested
                /// TODO: this needs to send a client event with the loop disable
                /// and actually we don't want to do this here, only when... opening a loop above?

                behavior.SetAction
                        (LayerConditionId::OpenLoop,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner) {
                                 std::cout << "insert: open loop, pausing layer below" << std::endl;
                                 layerBelow->DoAction(LayerActionId::StoreTrigger);
                                 layerBelow->DoAction(LayerActionId::Pause);
                             }
                         });
                behavior.SetAction
                        (LayerConditionId::CloseLoop,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             std::cout << "insert: close loop, resuming layer below" << std::endl;
                             if (!thisLayer->isInner) {
                                 layerBelow->DoAction(LayerActionId::Resume);
                             }
                         });
                behavior.SetAction
                        (LayerConditionId::Wrap,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner) {
                                 std::cout << "insert: wrapped, resume layer below" << std::endl;
                                 layerBelow->DoAction(LayerActionId::Resume);
                             }
                         });
                behavior.SetAction
                        (LayerConditionId::Trigger,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerBelow;
                             if (!thisLayer->isOuter) {
                                 std::cout << "insert: trigger, reset layer above, pause this layer" << std::endl;
                                 layerAbove->DoAction(LayerActionId::Reset);
                                 thisLayer->DoAction(LayerActionId::Pause);
                             }

                         });
                break;


            case LayerBehaviorModeId::MULTIPLY_QUANTIZED:
            case LayerBehaviorModeId::MULTIPLY_QUANTIZED_START:
            case LayerBehaviorModeId::MULTIPLY_QUANTIZED_END:
            case LayerBehaviorModeId::INSERT_QUANTIZED:
            case LayerBehaviorModeId::INSERT_QUANTIZED_START:
            case LayerBehaviorModeId::INSERT_QUANTIZED_END:
            default:
                // NYI mode
                break;
        }
    }
}
