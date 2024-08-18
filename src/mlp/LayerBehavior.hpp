#pragma once

#include <array>
#include <array>
#include <functional>
#include <utility>

//#include "LoopLayer.hpp"
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

    struct LayerConditions {
        std::array<bool, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> state{};

        static void Set(LayerConditions &conditions, LayerConditionId conditionId, bool value) {
            conditions.state[static_cast<size_t>(conditionId)] = value;
        }
    };

    /// non-specialized action wrapper for layers (which have audio-related specialization)
    class LayerActionInterface {

        std::array<std::function<void()>, static_cast<size_t>(LayerActionId::NUM_ACTIONS)> actions;
    public:
        LayerActionInterface(
                std::function<void()> reset,
                std::function<void()> restart,
                std::function<void()> pause,
                std::function<void()> resume,
                std::function<void()> storeTrigger,
                std::function<void()> storeReset
        ) : actions{
                std::move(reset),
                std::move(restart),
                std::move(pause),
                std::move(resume),
                std::move(storeReset),
                std::move(storeTrigger)
        } {}

        void Perform(LayerActionId actionId) {
            actions[static_cast<size_t>(actionId)]();
        }
    };


    struct LayerBehavior {
        struct Action {
            LayerActionInterface* thisLayer;
            LayerActionInterface* layerBelow;
            LayerActionInterface* layerAbove;
            std::function<void(LayerActionInterface*, LayerActionInterface*, LayerActionInterface*, bool)> function;

             void Perform(bool isInnerMostLayer) {
                 function(thisLayer, layerBelow, layerAbove, isInnerMostLayer);
            }
        };

        struct Condition {
            int counter{-1};
            std::vector<Action> actions{};
        };

        std::array<Condition, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionDataList;

        typedef BitSet<LayerConditionId, LayerConditionId::NUM_CONDITIONS> ConditionsResult;

        ConditionsResult ProcessConditions(LayerConditions &conditions,
                                           bool isInnerMostLayer) {
            ConditionsResult result{0};

            // seems weird that we don't have this in c++...
            // for (auto id: LayerConditionId)
            for (size_t i = 0; i < static_cast<size_t>(LayerConditionId::NUM_CONDITIONS); i++) {
                if (conditions.state[i]) {
                    if (ProcessCondition(static_cast<LayerConditionId>(i), isInnerMostLayer)) {
                        result.Set(static_cast<LayerConditionId>(i));
                    }
                }
            }
            return result;
        }

        bool ProcessCondition(LayerConditionId conditionId, bool isInnerMostLayer) {
            auto &conditionData = conditionDataList[static_cast<size_t>(conditionId)];
            if (conditionData.counter == 0) {
                return false;
            }
            for (auto &action: conditionData.actions) {
                action.Perform(isInnerMostLayer);
            }
            if (conditionData.counter > 0) {
                conditionData.counter--;
            }
            return true;
        }
    };

    ///=============================================================================================================
    ///=== modes

    /// for now, defining a number of fixed behavior sets ("modes") with hardcoded condition functions
    /// later, we'd like to make these definable and modifiable at runtime

    enum class LayerModeId {
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

    static bool
    InitializeLayerBehavior(std::vector<LayerActionInterface *> actionInterfaceList, LayerBehavior &behavior,
                            LayerModeId modeId) {
        for (auto &conditionData: behavior.conditionDataList) {
            conditionData.counter = -1;
            conditionData.actions.clear();
        }
        switch (modeId) {
            case LayerModeId::ASYNC:
                // nothing to do
                return true;
            case LayerModeId::MULTIPLY_UNQUANTIZED:
//                for (auto &actionInterface: actionInterfaceList) {
//                    behavior.conditionDataList[static_cast<size_t>(LayerConditionId::OpenLoop)].actions.push_back(
//                            {actionInterface,
//                             [](LayerActionInterface *actionInterface) { actionInterface->Perform(LayerActionId::Reset); }
//                            }
//                    );
//                }
//                behavior.conditionDataList[static_cast<size_t>(LayerConditionId::OpenLoop)].actions.push_back(
//                        {nullptr,
//                         [](LayerActionInterface *actionInterface) { actionInterface->Perform(LayerActionId::Reset); }
//                        }
//                );
                return true;
            case LayerModeId::INSERT_UNQUANTIZED:
//                behavior.conditionDataList[static_cast<size_t>(LayerConditionId::OpenLoop)].counter = 1;
//                behavior.conditionDataList[static_cast<size_t>(LayerConditionId::OpenLoop)].actions.push_back(
//                        {nullptr,
//                         [](LayerActionInterface *actionInterface) { actionInterface->Perform(LayerActionId::Reset); }}
//                );
                return true;
            default:
                // NYI mode
                return false;
        }
    }
}
