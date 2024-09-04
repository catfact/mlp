#include "Weaver.hpp"

using namespace mlp;
Weaver *Weaver::instance = nullptr;

const std::vector<const std::string> Weaver::vmClasses = {
        "MidiIn"
};

const std::map<const std::string, const std::vector<const std::string>> Weaver::vmMethods = {
        {"MidiIn", {"noteOn(_)", "noteOff(_)"}}
};

const std::vector<const std::string> Weaver::ffiClasses{
        "Output"
};

const std::map<const std::string, const std::vector<const std::string>> Weaver::ffiMethods = {
        {"Output", {"bang(_,_)"}}
};

std::map<const std::string, std::map<const std::string, WrenForeignMethodFn>> Weaver::ffiMethodFns{};