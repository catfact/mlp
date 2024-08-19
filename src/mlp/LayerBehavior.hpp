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
        //std::array<std::vector<LayerActionFunction>, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionActions;
        std::array<LayerActionFunction, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionAction;
        LayerInterface *thisLayer;
        LayerInterface *layerBelow;
        LayerInterface *layerAbove;

        void Clear() {
            for (auto &action: conditionAction) {
                action = [](LayerInterface *thisLayer, LayerInterface *layerBelow, LayerInterface *layerAbove,
                            bool isInnerMostLayer, bool isOuterMostLayer){};
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
//            auto &actions = conditionActions[static_cast<size_t>(id)];
//            for (auto &action: actions) {
//                action(thisLayer, layerBelow, layerAbove, isInnerMostLayer, isOuterMostLayer);
//            }
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

    static void SetLayerBehaviorMode(LayerBehavior &behavior, LayerBehaviorModeId modeId) {
        behavior.Clear();
        switch (modeId) {
            case LayerBehaviorModeId::ASYNC:
                // nothing to do!
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


//
//    struct LayerConditions {
//        std::array<bool, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> state{};
//
//        static void Set(LayerConditions &conditions, LayerConditionId conditionId, bool value) {
//            conditions.state[static_cast<size_t>(conditionId)] = value;
//        }
//    };
//
//    /// non-specialized action wrapper for layers (which have audio-related specialization)
//    class LayerActionInterface {
//
//        std::array<std::function<void()>, static_cast<size_t>(LayerActionId::NUM_ACTIONS)> actions;
//    public:
//        LayerActionInterface(
//                std::function<void()> reset,
//                std::function<void()> restart,
//                std::function<void()> pause,
//                std::function<void()> resume,
//                std::function<void()> storeTrigger,
//                std::function<void()> storeReset
//        ) : actions{
//                std::move(reset),
//                std::move(restart),
//                std::move(pause),
//                std::move(resume),
//                std::move(storeReset),
//                std::move(storeTrigger)
//        } {}
//
//        void Perform(LayerActionId actionId) {
//            actions[static_cast<size_t>(actionId)]();
//        }
//    };
//
//    struct LayerInterface {
//        LayerActionInterface *actions;
//        LayerConditions &conditions;
//    };
//
//
//    struct LayerBehavior {
//        struct Action {
//            LayerInterface *thisLayer;
//            LayerInterface *layerBelow;
//            LayerInterface *layerAbove;
//            std::function<void(LayerInterface *, LayerInterface *, LayerInterface *, bool)> function;
//
//            void Perform(bool isInnerMostLayer) {
//                function(thisLayer, layerBelow, layerAbove, isInnerMostLayer);
//            }
//        };
//
//        struct Condition {
//            int counter{-1};
//            std::vector<Action> actions{};
//        };
//
//        std::array<Condition, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionDataList;
//
//        typedef BitSet<LayerConditionId, LayerConditionId::NUM_CONDITIONS> ConditionsResult;
//
//        ConditionsResult ProcessConditions(LayerConditions &conditions,
//                                           bool isInnerMostLayer) {
//            ConditionsResult result{0};
//
//            // seems weird that we don't have this in c++...
//            // for (auto id: LayerConditionId)
//            for (size_t i = 0; i < static_cast<size_t>(LayerConditionId::NUM_CONDITIONS); i++) {
//                if (conditions.state[i]) {
//                    if (ProcessCondition(static_cast<LayerConditionId>(i), isInnerMostLayer)) {
//                        result.Set(static_cast<LayerConditionId>(i));
//                    }
//                }
//            }
//            return result;
//        }
//
//        bool ProcessCondition(LayerConditionId conditionId, bool isInnerMostLayer) {
//            auto &conditionData = conditionDataList[static_cast<size_t>(conditionId)];
//            if (conditionData.counter == 0) {
//                return false;
//            }
//            for (auto &action: conditionData.actions) {
//                action.Perform(isInnerMostLayer);
//            }
//            if (conditionData.counter > 0) {
//                conditionData.counter--;
//            }
//            return true;
//        }
//    };

    ///=============================================================================================================
    ///=== modes

    /// for now, defining a number of fixed behavior sets ("modes") with hardcoded condition functions
    /// later, we'd like to make these definable and modifiable at runtime
//
//    enum class LayerModeId {
//        ASYNC,
//        MULTIPLY_UNQUANTIZED,
//        MULTIPLY_QUANTIZED,
//        MULTIPLY_QUANTIZED_START,
//        MULTIPLY_QUANTIZED_END,
//        INSERT_UNQUANTIZED,
//        INSERT_QUANTIZED,
//        INSERT_QUANTIZED_START,
//        INSERT_QUANTIZED_END,
//    };
//
//    static bool
//    InitializeLayerBehavior(LayerInterface *thisLayer,
//                            LayerInterface *layerBelow,
//                            LayerInterface *layerAbove,
//                            LayerBehavior &behavior,
//                            LayerModeId modeId) {
//        for (auto &conditionData: behavior.conditionDataList) {
//            conditionData.counter = -1;
//            conditionData.actions.clear();
//        }
//
//        LayerBehavior::Action a;
//        a.thisLayer = thisLayer;
//        a.layerBelow = layerBelow;
//        a.layerAbove = layerAbove;
//
//        switch (modeId) {
//
////----------------------------------------------------------------------------------------------------------
//            case LayerModeId::ASYNC:
//                // no actions are required!
//                return true;
//
////----------------------------------------------------------------------------------------------------------
//            case LayerModeId::MULTIPLY_UNQUANTIZED:
//
//                //--- loop open: layer below to sets reset frame = current frame
//                a.action.function = [](LayerActionInterface *thisLayer, LayerActionInterface *layerBelow,
//                                LayerActionInterface *layerAbove, bool isInnerMostLayer) {
//                    if (!isInnerMostLayer) {
//                        layerBelow->Perform(LayerActionId::StoreReset);
//                    }
//                };
//                behavior.conditionDataList[static_cast<size_t>(LayerConditionId::OpenLoop)].actions.push_back(a);
//
//                //--- loop wrap: reset the layer below
//                a.function = [](LayerActionInterface *thisLayer, LayerActionInterface *layerBelow,
//                                LayerActionInterface *layerAbove, bool isInnerMostLayer) {
//                    if (!isInnerMostLayer) {
//                        layerBelow->Perform(LayerActionId::Reset);
//                    }
//                };
//                behavior.conditionDataList[static_cast<size_t>(LayerConditionId::Wrap)].actions.push_back(a);
//                return true;
//
////----------------------------------------------------------------------------------------------------------
//            case LayerModeId::INSERT_UNQUANTIZED:
//                // TODO
//                return true;
//            default:
//                // NYI mode
//                return false;
//        }
//    }
}
