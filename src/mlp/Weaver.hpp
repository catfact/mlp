#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

#include "wren.hpp"

#include "AudioTaper.hpp"
#include "Mlp.hpp"
#include "StateReflector.hpp"

namespace mlp {

    class Weaver {
        // needs to be singleton
        static Weaver *instance;
        Mlp *mlp;
        mlp::StateReflector* stateReflector{nullptr};

        /// TODO: will probably want an event queue,
        /// so that multiple threads can trigger wren calls without blocking
        /// using a mutex for now
        std::mutex vmLock{};

        WrenConfiguration config{};
        WrenVM *vm{nullptr};

        static const std::vector<const std::string> vmClasses;
        static const std::map<const std::string, const std::vector<const std::string>> vmMethods;
        static const std::vector<const std::string> ffiClasses;
        static const std::map<const std::string, const std::vector<const std::string>> ffiMethods;

        std::map<const std::string, WrenHandle *> vmClassHandles;
        std::map<const std::string, std::map<const std::string, WrenHandle *>> vmMethodHandles;
        std::map<const std::string, WrenHandle *> ffiClassHandles;
        static std::map<const std::string, std::map<const std::string, WrenForeignMethodFn>> ffiMethodFns;

        static void FfiBangSet(WrenVM *vm) {
            (void) vm;
            GetInstance()->mlp->Tap(mlp::Mlp::TapId::Set);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->BangSet();
            }
        }

        static void FfiBangStop(WrenVM *vm) {
            (void) vm;
            GetInstance()->mlp->Tap(mlp::Mlp::TapId::Stop);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->BangStop();
            }
        }

        static void FfiBangReset(WrenVM *vm) {
            (void) vm;
            GetInstance()->mlp->Tap(mlp::Mlp::TapId::Reset);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->BangReset();
            }
        }

        static void FfiParamLayerPreserveLevel(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto level = static_cast<float>(wrenGetSlotDouble(vm, 2));
            GetInstance()->mlp->IndexFloatParamChange(mlp::Mlp::IndexFloatParamId::LayerPreserveLevel, layerIndex, level);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->ParamLayerPreserveLevel(layerIndex, level);
            }
        }

        static void FfiParamLayerRecordLevel(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto level = static_cast<float>(wrenGetSlotDouble(vm, 2));
            GetInstance()->mlp->IndexFloatParamChange(mlp::Mlp::IndexFloatParamId::LayerRecordLevel, layerIndex, level);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->ParamLayerRecordLevel(layerIndex, level);
            }
        }

        static void FfiParamLayerPlaybackLevel(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto level = static_cast<float>(wrenGetSlotDouble(vm, 2));
            GetInstance()->mlp->IndexFloatParamChange(mlp::Mlp::IndexFloatParamId::LayerPlaybackLevel, layerIndex, level);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->ParamLayerPlaybackLevel(layerIndex, level);
            }
        }

        static void FfiParamLayerFadeTime(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto time = static_cast<float>(wrenGetSlotDouble(vm, 2));
            GetInstance()->mlp->IndexFloatParamChange(mlp::Mlp::IndexFloatParamId::LayerFadeTime, layerIndex, time);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->ParamLayerFadeTime(layerIndex, time);
            }
        }

        static void FfiParamLayerSwitchTime(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto time = static_cast<float>(wrenGetSlotDouble(vm, 2));
            GetInstance()->mlp->IndexFloatParamChange(mlp::Mlp::IndexFloatParamId::LayerSwitchTime, layerIndex, time);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->ParamLayerSwitchTime(layerIndex, time);
            }
        }

        static void FfiGlobalMode(WrenVM *vm) {
            auto mode = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            GetInstance()->mlp->IndexParamChange(mlp::Mlp::IndexParamId::Mode, mode);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->GlobalMode(mode);
            }
        }

        static void FfiLayerSelect(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            GetInstance()->mlp->IndexParamChange(mlp::Mlp::IndexParamId::LayerSelect, layerIndex);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->LayerSelect(layerIndex);
            }
        }

        static void FfiLayerReset(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            GetInstance()->mlp->IndexParamChange(mlp::Mlp::IndexParamId::LayerReset, layerIndex);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->LayerReset(layerIndex);
            }
        }

        static void FfiLayerRestart(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            GetInstance()->mlp->IndexParamChange(mlp::Mlp::IndexParamId::LayerRestart, layerIndex);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->LayerRestart(layerIndex);
            }
        }

        static void FfiLayerWrite(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto state = wrenGetSlotBool(vm, 2);
            GetInstance()->mlp->IndexBoolParamChange(mlp::Mlp::IndexBoolParamId::LayerWriteEnabled, layerIndex, state);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->LayerWrite(layerIndex, state);
            }
        }

        static void FfiLayerRead(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto state = wrenGetSlotBool(vm, 2);
            GetInstance()->mlp->IndexBoolParamChange(mlp::Mlp::IndexBoolParamId::LayerReadEnabled, layerIndex, state);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->LayerRead(layerIndex, state);
            }
        }

        static void FfiLayerClear(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto state = wrenGetSlotBool(vm, 2);
            GetInstance()->mlp->IndexBoolParamChange(mlp::Mlp::IndexBoolParamId::LayerClearEnabled, layerIndex, state);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->LayerClear(layerIndex, state);
            }
        }

        static void FfiLayerLoop(WrenVM *vm) {
            auto layerIndex = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
            auto state = wrenGetSlotBool(vm, 2);
            GetInstance()->mlp->IndexBoolParamChange(mlp::Mlp::IndexBoolParamId::LayerLoopEnabled, layerIndex, state);
            if (GetInstance()->stateReflector) {
                GetInstance()->stateReflector->LayerLoop(layerIndex, state);
            }
        }
    public:
        static Weaver *Init(Mlp *aMlp, const char *aScriptPath = nullptr) {
            if (instance == nullptr) {
                instance = new Weaver(aMlp, aScriptPath);
            }
            return instance;
        }

        static void Deinit() { delete instance; }

        static Weaver *GetInstance() {
            if (instance == nullptr) {
                /// if the VM wasn't initialized before referencing,
                /// it means there was no MLP instance or initial script data given,
                /// and nothing will work!
                assert(false);
//                std::cerr << "<WARNING> no main script!" << std::endl;
//                instance = new Weaver();
            }
            return instance;
        }

    private:
        explicit Weaver(Mlp *aMlp = nullptr, const char *scriptData = nullptr) : mlp(aMlp) {
            config.errorFn = &ErrorFn;
            config.writeFn = &WriteFn;
            config.bindForeignMethodFn = &BindForeignMethodFn;

            vm = wrenNewVM(&config);
            const char *module = "main";
            std::string script;
            WrenInterpretResult result;

            // NB: FFI function pointers need to be set before running main!

            ffiMethodFns["Mlp"]["BangSet()"] = &FfiBangSet;
            ffiMethodFns["Mlp"]["BangStop()"] = &FfiBangStop;
            ffiMethodFns["Mlp"]["BangReset()"] = &FfiBangReset;

            ffiMethodFns["Mlp"]["ParamLayerPreserveLevel(_,_)"] = &FfiParamLayerPreserveLevel;
            ffiMethodFns["Mlp"]["ParamLayerRecordLevel(_,_)"] = &FfiParamLayerRecordLevel;
            ffiMethodFns["Mlp"]["ParamLayerPlaybackLevel(_,_)"] = &FfiParamLayerPlaybackLevel;
            ffiMethodFns["Mlp"]["ParamLayerFadeTime(_,_)"] = &FfiParamLayerFadeTime;
            ffiMethodFns["Mlp"]["ParamLayerSwitchTime(_,_)"] = &FfiParamLayerSwitchTime;

            ffiMethodFns["Mlp"]["GlobalMode(_)"] = &FfiGlobalMode;
            ffiMethodFns["Mlp"]["LayerSelect(_)"] = &FfiLayerSelect;
            ffiMethodFns["Mlp"]["LayerReset(_)"] = &FfiLayerReset;
            ffiMethodFns["Mlp"]["LayerRestart(_)"] = &FfiLayerRestart;

            ffiMethodFns["Mlp"]["LayerWrite(_,_)"] = &FfiLayerWrite;
            ffiMethodFns["Mlp"]["LayerRead(_,_)"] = &FfiLayerRead;
            ffiMethodFns["Mlp"]["LayerClear(_,_)"] = &FfiLayerClear;
            ffiMethodFns["Mlp"]["LayerLoop(_,_)"] = &FfiLayerLoop;

            if (scriptData) {
                result = wrenInterpret(vm, module, scriptData);
            } else {
                result = wrenInterpret(vm, module, "System.print(\"wren start\")");
            }

            switch (result) {
                case WREN_RESULT_COMPILE_ERROR:
                    printf("<ERROR> [compile]\n");
                    break;
                case WREN_RESULT_RUNTIME_ERROR:
                    printf("<ERROR> [runtime]\n");
                    break;
                case WREN_RESULT_SUCCESS:
                    printf("<ok>\n");
                    break;
            }

            for (const auto &className: vmClasses) {
                wrenEnsureSlots(vm, 1);
                wrenGetVariable(vm, "main", className.c_str(), 0);
                vmClassHandles[className] = wrenGetSlotHandle(vm, 0);
                vmMethodHandles[className] = std::map<const std::string, WrenHandle *>();
                for (const auto &methodSignature: vmMethods.at(className)) {
                    std::cout << "storing handle for class " << className << "; method " << methodSignature << std::endl;
                    vmMethodHandles[className][methodSignature] = wrenMakeCallHandle(vm, methodSignature.c_str());
                }
            }

            for (const auto &className: ffiClasses) {
                wrenEnsureSlots(vm, 1);
                wrenGetVariable(vm, "main", className.c_str(), 0);
                ffiClassHandles[className] = wrenGetSlotHandle(vm, 0);
                ffiMethodFns[className] = std::map<const std::string, WrenForeignMethodFn>();
                /// need to add method fn pointers here?
            }

            static_assert(sizeof(char) == sizeof(uint8_t));
        }

        ~Weaver() {
            wrenFreeVM(vm);
        }

    public:

        void SetStateReflector(mlp::StateReflector* aStateReflector) {
            stateReflector = aStateReflector;
        }

        void HandleMidiMessage(const unsigned char *aData, int aSize) {
            assert (aSize > 0);
            std::lock_guard<std::mutex> lock(vmLock);
            if (vm != nullptr) {
                wrenEnsureSlots(vm, 2);
                WrenHandle *hClass = vmClassHandles["MidiIn"];
                WrenHandle *hMethod;
                int channel = aData[0] & 0x0F;
                int status  = aData[0] & 0xF0;

                wrenEnsureSlots(vm, 4);
                switch (status) {
                    case 0x90:
                        hMethod = vmMethodHandles["MidiIn"]["noteOn(_,_,_)"];
                        break;
                    case 0x80:
                        hMethod = vmMethodHandles["MidiIn"]["noteOff(_,_,_)"];
                        break;
                    case 0xB0:
                        hMethod = vmMethodHandles["MidiIn"]["controlChange(_,_,_)"];
                        break;
                    case 0xE0:
                        hMethod = vmMethodHandles["MidiIn"]["pitchBend(_,_,_)"];
                        break;
                    default:
                        // std::cout << "unhandled midi message: " << status << std::endl;
                        return;
                }
                //WrenHandle *hMethod = vmMethodHandles["MidiIn"]["data(_)"];
                wrenSetSlotHandle(vm, 0, hClass);
                //wrenSetSlotBytes(vm, 1, reinterpret_cast<const char *>(aData), static_cast<unsigned int>(aSize));

                assert(aSize >= 3);
                wrenSetSlotDouble(vm, 1, channel);
                wrenSetSlotDouble(vm, 2, aData[1]);
                wrenSetSlotDouble(vm, 3, aData[2]);
                wrenCall(vm, hMethod);
            }
        }


        ///--- wren bindings

        /// TODO: should be a callback to the host app
        static void WriteFn(WrenVM *vm, const char *text) {
            (void) vm;
            printf("%s", text);
        }

        /// TODO: should be a callback to the host app
        static void ErrorFn(WrenVM *vm, WrenErrorType errorType,
                            const char *module, const int line,
                            const char *msg) {
            (void) vm;

            switch (errorType) {
                case WREN_ERROR_COMPILE: {
                    printf("[%s line %d] [Error] %s\n", module, line, msg);
                }
                    break;
                case WREN_ERROR_STACK_TRACE: {
                    printf("[%s line %d] in %s\n", module, line, msg);
                }
                    break;
                case WREN_ERROR_RUNTIME: {
                    printf("[Runtime Error] %s\n", msg);
                }
                    break;
            }
        }

        /// TODO: module loading, which definitely needs to be passed off to the host app

        static WrenForeignMethodFn BindForeignMethodFn(WrenVM *vm, const char *module,
                                                       const char *className, bool isStatic,
                                                       const char *signature) {
            (void) vm;
            printf("BindForeignMethodFn: %s %s %s %s\n", module, className, isStatic ? "true" : "false", signature);
            if (!isStatic) {
                std::cerr << "<ERROR> we only know how to bind wren static methods" << std::endl;
            }
            auto fn = ffiMethodFns[className][signature];
            std::cout << fn << std::endl;
            return fn;
        }


        // TODO?
//        static WrenBindForeignClassFn bindForeignClassFn(WrenVM* vm, const char* module, const char* className) {
//            printf("bindForeignClassFn: %s %s\n", module, className);
//        }



    };
}