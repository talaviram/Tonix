cmake_minimum_required(VERSION 3.22)

set(PLUGIN_NAME Tonix)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "macOS deployment target")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")

set(AAX_SIGN_GUID 33007520-63AF-11F0-908A-005056BC33E3 CACHE STRING "AAX Sign GUID")

project(${PLUGIN_NAME} VERSION 1.0.0)

include(cmake/CPM.cmake)
CPMAddPackage(
    GITHUB_REPOSITORY "juce-framework/JUCE"
    GIT_TAG "8.0.12"
    OPTIONS
        "JUCE_ENABLE_MODULE_SOURCE_GROUPS ON"
)

CPMAddPackage(
    GITHUB_REPOSITORY "free-audio/clap-juce-extensions"
    GIT_TAG "645ed2fd0949d36639e3d63333f26136df6df769"
)

CPMAddPackage(
    GITHUB_REPOSITORY "Svalorzen/GitHash"
    GIT_TAG "91d0df1260bdfd69a04cb3d17a49dce0c4d3c24a"
)

include(cmake/FetchLicenses.cmake)
set(INPUT_FILES
    "${CMAKE_SOURCE_DIR}/cmake/BINARY_LICENSE.txt"
    "${CMAKE_SOURCE_DIR}/cmake/JUCE_LICENSE.txt"
    "${JUCE_SOURCE_DIR}/modules/juce_audio_plugin_client/AAX/SDK/LICENSE.txt"
    "${JUCE_SOURCE_DIR}/modules/juce_audio_plugin_client/AU/AudioUnitSDK/LICENSE.txt"
    "${JUCE_SOURCE_DIR}/modules/juce_audio_processors_headless/format_types/VST3_SDK/pluginterfaces/LICENSE.txt"
)
set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/LICENSE.txt")
concat_files("${OUTPUT_FILE}" INPUT_FILES "${INPUT_FILES}")

# On Windows this would most likely fail unless running as admin
if (NOT WIN32)
    set(COPY_DURING_DEV TRUE)
else()
    set(COPY_DURING_DEV FALSE)
endif()

set(BUNDLE_ID "com.talaviram.tonix")

juce_add_plugin(${PLUGIN_NAME}
    BUNDLE_ID ${BUNDLE_ID}
    COMPANY_NAME "Tal Aviram"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_DURING_DEV}
    PLUGIN_MANUFACTURER_CODE Tala
    PLUGIN_CODE TonX
    FORMATS AU AAX VST3 Standalone
    PRODUCT_NAME ${PLUGIN_NAME})

clap_juce_extensions_plugin(TARGET ${PLUGIN_NAME}
        CLAP_ID ${BUNDLE_ID}
        CLAP_FEATURES audio-effect distortion tape)

target_sources(${PLUGIN_NAME}
    PRIVATE
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp)

target_include_directories(${PLUGIN_NAME}
    PRIVATE
        Source
)

target_compile_definitions(${PLUGIN_NAME}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

juce_add_binary_data(BinaryData SOURCES
    Source/Media/KNB_metal_pink_L.png
)

if(NOT LINUX)
    set(CONDITIONAL_FLAGS "juce::juce_recommended_warning_flags")
else()
    set(CONDITIONAL_FLAGS "")
endif()

target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        BinaryData
        githash
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        ${CONDITIONAL_FLAGS})

if(AAX_SIGN_ID)
    set_target_properties(${PLUGIN_NAME}_AAX PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODESIGNING_FOLDER_PATH ""
    )
    include(cmake/AAXSign.cmake)
    get_target_property(AAX_PATH "${PLUGIN_NAME}_AAX" JUCE_PLUGIN_ARTEFACT_FILE)
    sign_aax(${PLUGIN_NAME}_AAX ${AAX_PATH} ${AAX_SIGN_ID})
endif()

# Packaging
include(cmake/Packager.cmake)
