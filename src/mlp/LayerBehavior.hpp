#pragma once

#include <array>
#include <array>
#include <functional>
#include <utility>

#include "LoopLayer.hpp"
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

        struct ConditionData {
            int counter{-1};
            std::vector<Action> actions{};
        };
        std::array<ConditionData, static_cast<size_t>(LayerConditionId::NUM_CONDITIONS)> conditionDataList;

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
