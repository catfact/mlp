#pragma once

#include "Mlp.hpp"
#include "mlp/Outputs.hpp"

struct MlpGuiInput {
    typedef std::function<void(unsigned int, mlp::LayerOutputFlagId)> LayerOutputFlagCallback;

    LayerOutputFlagCallback layerOutputFlagCallback = [](unsigned int, mlp::LayerOutputFlagId) {};

    void HandleLayerOutputFlag(unsigned int layerIndex, mlp::LayerOutputFlagId id)
    {
        layerOutputFlagCallback(layerIndex, id);
    }
};

class MlpGuiOutput {
public:
    virtual void SendTap(mlp::Mlp::TapId id) = 0;
    virtual void SendBool(mlp::Mlp::BoolParamId id, bool value) = 0;
    virtual void SendIndexBool(mlp::Mlp::IndexBoolParamId id, unsigned int index, bool value) = 0;
    virtual void SendIndexFloat(mlp::Mlp::IndexFloatParamId id, unsigned int index, float value) = 0;
};