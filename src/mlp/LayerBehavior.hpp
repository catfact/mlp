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
        EnableLoop,
        DisableLoop,
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
            actions[static_cast<size_t>(LayerActionId::EnableLoop)] = [layer]() { layer->SetLoopEnabled(true); };
            actions[static_cast<size_t>(LayerActionId::DisableLoop)] = [layer]() { layer->SetLoopEnabled(false); };
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
                    case LayerActionId::EnableLoop:
                        outputs->flags.Set(LayerOutputFlagId::Opened);
                        break;
                    case LayerActionId::DisableLoop:
                        outputs->flags.Set(LayerOutputFlagId::Closed);
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
//        MULTIPLY_QUANTIZED,
//        MULTIPLY_QUANTIZED_START,
//        MULTIPLY_QUANTIZED_END,
        INSERT_UNQUANTIZED,
//        INSERT_QUANTIZED,
//        INSERT_QUANTIZED_START,
//        INSERT_QUANTIZED_END,
        COUNT,
    };

    static constexpr char LayerBehaviorModeIdLabel[static_cast<int>(LayerBehaviorModeId::COUNT)][16] = {
        "ASYNC",
        "MULT",
        "INSERT",
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
                             if (!thisLayer->isInner) {
                                 layerBelow->DoAction(LayerActionId::StoreReset);
                             }
                         });
                behavior.SetAction
                        (LayerConditionId::CloseLoop,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner) {
                                 layerBelow->DoAction(LayerActionId::Reset);
                             }
                         });

                behavior.SetAction
                        (LayerConditionId::Wrap,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner) {
                                 layerBelow->DoAction(LayerActionId::Reset);
                             }
                         });
                break;

            case LayerBehaviorModeId::INSERT_UNQUANTIZED:
                behavior.SetAction
                        (LayerConditionId::OpenLoop,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner) {
                                 // std::cout << "insert: open loop, not inner: pause layer below" << std::endl;
                                 layerBelow->DoAction(LayerActionId::StoreTrigger);
                                 layerBelow->DoAction(LayerActionId::Pause);
                                 // std::cout << "insert: open loop, not inner: disable loop on this layer" << std::endl;
                                 thisLayer->DoAction(LayerActionId::DisableLoop);
                             } else {
                                    // std::cout << "insert: open loop, inner: no special action" << std::endl;
                             }
                         });
                behavior.SetAction
                        (LayerConditionId::CloseLoop,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner) {
                                 // std::cout << "insert: close loop, not inner: resuming layer below" << std::endl;
                                 layerBelow->DoAction(LayerActionId::Resume);
                             } else {
                                 // std::cout << "insert: close loop, inner; no special action" << std::endl;
                             }
                         });
                behavior.SetAction
                        (LayerConditionId::Wrap,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerAbove;
                             if (!thisLayer->isInner) {
                                 // std::cout << "insert: wrapped, resume layer below" << std::endl;
                                 layerBelow->DoAction(LayerActionId::Resume);
                             } else {
                                 // std::cout << "insert: wrapped, inner: no special action" << std::endl;
                             }
                         });
                behavior.SetAction
                        (LayerConditionId::Trigger,
                         [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove) {
                             (void) layerBelow;
                             if (!thisLayer->isOuter) {
                                 // std::cout << "insert: trigger, reset layer above, pause this layer" << std::endl;
                                 layerAbove->DoAction(LayerActionId::Restart);
                                 thisLayer->DoAction(LayerActionId::Pause);
                             } else {
                                 // std::cout << "insert: trigger, outer: no special action" << std::endl;
                             }
                         });
                break;


//            case LayerBehaviorModeId::MULTIPLY_QUANTIZED:
//            case LayerBehaviorModeId::MULTIPLY_QUANTIZED_START:
//            case LayerBehaviorModeId::MULTIPLY_QUANTIZED_END:
//            case LayerBehaviorModeId::INSERT_QUANTIZED:
//            case LayerBehaviorModeId::INSERT_QUANTIZED_START:
//            case LayerBehaviorModeId::INSERT_QUANTIZED_END:
            case LayerBehaviorModeId::COUNT:
            default:
                // NYI mode
                break;
        }
    }
}
