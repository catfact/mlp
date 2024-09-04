#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "wren.hpp"

namespace mlp {

    class Weaver {
        // needs to be singleton
        static Weaver *instance;

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

        static void FfiOutputBang(WrenVM *vm) {
            double a = wrenGetSlotDouble(vm, 1);
            double b = wrenGetSlotDouble(vm, 2);
            std::cout << "bang " << a << " " << b << std::endl;
        }

    public:
        static Weaver *Init(const char *scriptPath = nullptr) {
            if (instance == nullptr) {
                instance = new Weaver(scriptPath);
            }
            return instance;
        }

        static void Deinit() { delete instance; }

        static Weaver *GetInstance() {
            if (instance == nullptr) {
                /// if the VM wasn't initialized at construction,
                /// it means there was no initial script data given,
                /// and things will probably not work
                std::cerr << "<WARNING> no main script!" << std::endl;
                instance = new Weaver();
            }
            return instance;
        }

    private:
        explicit Weaver(const char *scriptData = nullptr) {
            config.errorFn = &ErrorFn;
            config.writeFn = &WriteFn;
            config.bindForeignMethodFn = &bindForeignMethodFn;


            vm = wrenNewVM(&config);
            const char *module = "main";
            std::string script;
            WrenInterpretResult result;

            // NB: FFI function pointers need to be set before running main!
            ffiMethodFns["Output"]["bang(_,_)"] = &FfiOutputBang;

            if (scriptData) {
                result = wrenInterpret(vm, module, scriptData);
            } else {
                result = wrenInterpret(vm, module, "System.print(\"wren start\")");
            }

            switch (result) {
                case WREN_RESULT_COMPILE_ERROR: {
                    printf("<ERROR> [compile]\n");
                }
                    break;
                case WREN_RESULT_RUNTIME_ERROR: {
                    printf("<ERROR> [runtime]\n");
                }
                    break;
                case WREN_RESULT_SUCCESS: {
                    printf("<ok>\n");
                }
                    break;
            }

            for (const auto &className: vmClasses) {
                wrenEnsureSlots(vm, 1);
                wrenGetVariable(vm, "main", className.c_str(), 0);
                vmClassHandles[className] = wrenGetSlotHandle(vm, 0);
                vmMethodHandles[className] = std::map<const std::string, WrenHandle *>();
                for (const auto &methodSignature: vmMethods.at(className)) {
                    // std::cout << "storing handle for class " << className << "; method " << methodSignature << std::endl;
                    vmMethodHandles[className][methodSignature] = wrenMakeCallHandle(vm, methodSignature.c_str());
                }
            }

            for (const auto &className: ffiClasses) {
                wrenEnsureSlots(vm, 1);
                wrenGetVariable(vm, "main", className.c_str(), 0);
                ffiClassHandles[className] = wrenGetSlotHandle(vm, 0);
                ffiMethodFns[className] = std::map<const std::string, WrenForeignMethodFn>();
            }

            static_assert(sizeof(char) == sizeof(uint8_t));
        }

        ~Weaver() {
            wrenFreeVM(vm);
        }

    public:

        ///--- API for calling wren from c++
        void HandleNoteOn(const uint8_t *data) {
            std::cout << "note on: " << (int) data[0] << " " << (int) data[1] << std::endl;
            {
                std::lock_guard<std::mutex> lock(vmLock);
                if (vm != nullptr) {
                    wrenEnsureSlots(vm, 2);
                    // TODO: worth profiling these lookups, could be replaced by enums/arrays
                    WrenHandle *hClass = vmClassHandles["MidiIn"];
                    WrenHandle *hMethod = vmMethodHandles["MidiIn"]["noteOn(_)"];
                    wrenSetSlotHandle(vm, 0, hClass);
                    wrenSetSlotBytes(vm, 1, reinterpret_cast<const char *>(data), 2);
                    wrenCall(vm, hMethod);
                }
            }
        }

        void HandleNoteOff(const uint8_t *data) {
            std::cout << "note off: " << (int) data[0] << " " << (int) data[1] << std::endl;
            {
                std::lock_guard<std::mutex> lock(vmLock);
                if (vm != nullptr) {
                    WrenHandle *hClass = vmClassHandles["MidiIn"];
                    wrenEnsureSlots(vm, 2);
                    WrenHandle *hMethod = vmMethodHandles["MidiIn"]["noteOff(_)"];
                    wrenSetSlotHandle(vm, 0, hClass);
                    wrenSetSlotBytes(vm, 1, reinterpret_cast<const char *>(data), 2);
                    wrenCall(vm, hMethod);
                }
            }
        }

        ///--- wren bindings
        static void WriteFn(WrenVM *vm, const char *text) {
            (void)vm;
            printf("%s", text);
        }

        static void ErrorFn(WrenVM *vm, WrenErrorType errorType,
                            const char *module, const int line,
                            const char *msg) {
            (void)vm;
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

        static WrenForeignMethodFn bindForeignMethodFn(WrenVM *vm, const char *module,
                                                       const char *className, bool isStatic,
                                                       const char *signature) {
            (void)vm;
            printf("bindForeignMethodFn: %s %s %s %s\n", module, className, isStatic ? "true" : "false", signature);
            if (!isStatic) {
                std::cerr << "<ERROR> we only know how to bind wren static methods" << std::endl;
            }
            auto fn = ffiMethodFns[className][signature];
            std::cout << fn << std::endl;
            return fn;
        }

//        static WrenBindForeignClassFn bindForeignClassFn(WrenVM* vm, const char* module, const char* className) {
//            printf("bindForeignClassFn: %s %s\n", module, className);
//        }

    };
}