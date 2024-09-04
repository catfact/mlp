cmake_minimum_required(VERSION 3.22)

project(mlp-plug VERSION 0.0.1)

add_subdirectory(JUCE)

#---------------------------------------------------------------------------------------------------------------------
#---- plugin target

# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (AudioPluginExample here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.

juce_add_plugin(mlp-plug
        # VERSION ...                               # Set this if the plugin version is different to the project version
        # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        # ICON_SMALL ...
        COMPANY_NAME "moth-object"
        # IS_SYNTH TRUE/FALSE                       # Is this a synth or an effect?
        # NEEDS_MIDI_INPUT TRUE/FALSE               # Does the plugin need midi input?
        # NEEDS_MIDI_OUTPUT TRUE/FALSE              # Does the plugin need midi output?
        # IS_MIDI_EFFECT TRUE/FALSE                 # Is this plugin a MIDI effect?
        # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?
        # COPY_PLUGIN_AFTER_BUILD TRUE/FALSE        # Should the plugin be installed to a default location after building?
        PLUGIN_MANUFACTURER_CODE Moth               # A four-character manufacturer id with at least one upper-case character
        PLUGIN_CODE Mllp                            # A unique four-character plugin id with exactly one upper-case character
        # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
        FORMATS AU VST3 Standalone                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
        PRODUCT_NAME "mlp")        # The name of the final executable, which can differ from the target name


set(WREN_VM ../extern/wren/src/vm)
set(WREN_OPT ../extern/wren/src/optional)
set(MLP_SRC ../src/mlp)

target_sources(mlp-plug
        PRIVATE
        PluginEditor.cpp
        PluginProcessor.cpp
        MlpGui.cpp
        ${MLP_SRC}/Weaver.cpp
        ${WREN_VM}/wren_compiler.c
        ${WREN_VM}/wren_vm.c
        ${WREN_VM}/wren_core.c
        ${WREN_VM}/wren_debug.c
        ${WREN_VM}/wren_primitive.c
        ${WREN_VM}/wren_utils.c
        ${WREN_VM}/wren_value.c
        ${WREN_VM}/wren_vm.c
        ${WREN_OPT}/wren_opt_meta.c
        ${WREN_OPT}/wren_opt_random.c)

target_compile_definitions(mlp-plug
        PUBLIC
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

# juce_add_binary_data(AudioPluginData SOURCES ...)

target_include_directories(mlp-plug PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../src
        ${CMAKE_CURRENT_LIST_DIR}/../extern
        ${CMAKE_CURRENT_LIST_DIR}/../extern/wren/src/include
        ${CMAKE_CURRENT_LIST_DIR}/../extern/wren/src/optional
        ${CMAKE_CURRENT_LIST_DIR}/../extern/wren/src/vm)

target_link_libraries(mlp-plug
        PRIVATE
        # AudioPluginData
        juce::juce_audio_utils
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)


#-------------------------------------------------------------------------------------------
#--- OSC GUI target
#
#juce_add_gui_app(mlp-plug-gui
#        VERSION 0.0.1
#        COMPANY_NAME "moth-object"
#        PRODUCT_NAME "mlp-gui")
#
#target_sources(mlp-plug-gui
#        PRIVATE
#        gui-osc/Main.cpp
#        gui-osc/MainComponent.cpp)
#
#target_include_directories(mlp-plug-gui PRIVATE ${CMAKE_CURRENT_LIST_DIR})
#target_include_directories(mlp-plug-gui PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src)
#target_include_directories(mlp-plug-gui PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../extern)
#
#target_compile_definitions(mlp-plug-gui
#        PRIVATE
#        JUCE_WEB_BROWSER=0
#        JUCE_USE_CURL=0
#        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:mlp-plug-gui,JUCE_PRODUCT_NAME>"
#        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:mlp-plug-gui,JUCE_VERSION>")
#
#
#target_link_libraries(mlp-plug-gui
#        PRIVATE
#        juce::juce_gui_extra
#        PUBLIC
#        juce::juce_recommended_config_flags
#        juce::juce_recommended_lto_flags
#        juce::juce_recommended_warning_flags)
