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
                std::function<void()> resume
        ) : actions{std::move(reset), std::move(restart), std::move(pause), std::move(resume)} {}

        void Perform(LayerActionId actionId) {
            actions[static_cast<size_t>(actionId)]();
        }
    };

    struct LayerBehavior {
        struct Action{
            LayerActionInterface *actionInterface{nullptr};
            std::function<void(LayerActionInterface *)> function;
            bool Perform() {
                if (actionInterface) {
                    function(actionInterface);
                    return true;
                }
                return false;
            }
        };

        struct Condition {
            int counter{-1};
            std::vector<Action> actions{};
        };
        std::array<Condition, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionDataList;

        //typedef std::array<bool, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> ConditionsResult;
        typedef BitSet<LayerConditionId, LayerConditionId::NUM_CONDITIONS> ConditionsResult;

        ConditionsResult ProcessConditions(LayerActionInterface &actionInterface, LayerConditions &conditions) {
            ConditionsResult result{0};

            // seems weird that we don't have this in c++...
            // for (auto id: LayerConditionId)
            for (size_t i = 0; i < static_cast<size_t>(LayerConditionId::NUM_CONDITIONS); i++) {
                if (conditions.state[i]) {
                    if (ProcessCondition(static_cast<LayerConditionId>(i))) {
                        result.Set(static_cast<LayerConditionId>(i));
                    }
                }
            }
            return result;
        }

        bool ProcessCondition(LayerConditionId conditionId) {
            auto &conditionData = conditionDataList[static_cast<size_t>(conditionId)];
            if (conditionData.counter == 0) {
                return false;
            }
            for (auto &action : conditionData.actions) {
                action.Perform();
            }
            if (conditionData.counter > 0) {
                conditionData.counter--;
            }
            return true;
        }
    };


}
