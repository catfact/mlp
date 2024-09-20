#pragma once
namespace mlp {
    class StateReflector {
    public:
        virtual void BangSet() = 0;

        virtual void BangStop() = 0;

        virtual void BangReset() = 0;

        virtual void ParamLayerPreserveLevel(unsigned int layerIndex, float level) = 0;

        virtual void ParamLayerRecordLevel(unsigned int layerIndex, float level) = 0;

        virtual void ParamLayerPlaybackLevel(unsigned int layerIndex, float level) = 0;

        virtual void ParamLayerFadeTime(unsigned int layerIndex, float time) = 0;

        virtual void ParamLayerSwitchTime(unsigned int layerIndex, float time) = 0;

        virtual void GlobalMode(unsigned int mode) = 0;

        virtual void LayerSelect(unsigned int layerIndex) = 0;

        virtual void LayerReset(unsigned int layerIndex) = 0;

        virtual void LayerRestart(unsigned int layerIndex) = 0;

        virtual void LayerWrite(unsigned int layerIndex, bool state) = 0;

        virtual void LayerRead(unsigned int layerIndex, bool state) = 0;

        virtual void LayerClear(unsigned int layerIndex, bool state) = 0;

        virtual void LayerLoop(unsigned int layerIndex, bool state) = 0;
    };
}