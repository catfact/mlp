#pragma once

#include <iostream>
#include <juce_core/juce_core.h>

#include "BinaryData.h"

class ScriptManager {
private:
    static const char* GetEmbeddedMainScript() {
        int wrenMainSize;
        auto wrenMain = BinaryData::getNamedResource("main_wren", wrenMainSize);
        auto sz = static_cast<size_t>(wrenMainSize);
        char* data = new char[sz]();
        memcpy(data, wrenMain, sz);
        return data;
    }

    static const char* GetUserMainScript() {
        auto f = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        std::cout << "user documents dir: " << f.getFullPathName() << std::endl;
        f = f.getChildFile("mlp");
        f = f.getChildFile("main.wren");
        std::cout << "user script file: " << f.getFullPathName() << std::endl;
        if (f.existsAsFile()) {
            juce::MemoryBlock mb;
            f.loadFileAsData(mb);
            auto sz = mb.getSize();
            char* data = new char[sz]();
            memcpy(data, mb.getData(), sz);
            return data;
        } else {
            return nullptr;
        }
    }

public:
    static const char*  GetMainScript() {
        auto userMain = GetUserMainScript();
        if (userMain != nullptr) {
            return userMain;
        }
        return GetEmbeddedMainScript();
    }
};