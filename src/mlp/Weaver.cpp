#include "Weaver.hpp"

using namespace mlp;
Weaver *Weaver::instance = nullptr;

const std::vector<const std::string> Weaver::vmClasses = {
        "MidiIn"
};

const std::map<const std::string, const std::vector<const std::string>> Weaver::vmMethods = {
        {"MidiIn", {"noteOn(_,_,_)", "noteOff(_,_,_)", "controlChange(_,_,_)", "pitchBend(_,_,_)"}}
};

const std::vector<const std::string> Weaver::ffiClasses{
        "Mlp"
};

const std::map<const std::string, const std::vector<const std::string>> Weaver::ffiMethods = {
        {"Mlp", {
                "BangSet()",
                "BangStop()",
                "BangReset()",

                "ParamLayerPreserveLevel(_,_)",
                "ParamLayerRecordLevel(_,_)",
                "ParamLayerPlayLevel(_,_)",
                "ParamLayerFadeTime(_,_)",
                "ParamLayerSwitchTime(_,_)",

                "GlobalMode(_)",
                "LayerSelect(_)",
                "LayerReset(_)",
                "LayerRestart(_)",

                "LayerWrite(_,_)",
                "LayerClear(_,_)",
                "LayerRead(_,_)",
                "LayerLoop(_,_)"
        }}
};

std::map<const std::string, std::map<const std::string, WrenForeignMethodFn>> Weaver::ffiMethodFns{};