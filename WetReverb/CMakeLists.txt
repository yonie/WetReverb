cmake_minimum_required(VERSION 3.14.0)

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

set(vst3sdk_SOURCE_DIR "../vst3sdk")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(WetReverb
    VERSION 1.0.1.0
    DESCRIPTION "WetReverb VST 3 Plug-in"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(WetReverb
    source/version.h
    source/wetreverbcids.h
    source/wetreverbprocessor.h
    source/wetreverbprocessor.cpp
    source/wetreverbcontroller.h
    source/wetreverbcontroller.cpp
    source/wetreverbentry.cpp
    source/reverbbuffer.h
    source/reverbbuffer.cpp
    source/ledmeterview.h
    source/ledmeterview.cpp
    source/buttonledindicator.h
    source/buttonledindicator.cpp
    source/buttonselectionframe.h
    source/buttonselectionframe.cpp
    source/customviewcreator.h
    source/customviewcreator.cpp
)

#- VSTGUI Wanted ----
if(SMTG_ENABLE_VSTGUI_SUPPORT)
    target_sources(WetReverb
        PRIVATE
            resource/wetreverbeditor.uidesc
    )
    target_link_libraries(WetReverb
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(WetReverb
        RESOURCES
            "resource/wetreverbeditor.uidesc"
            "resource/backplate.png"
    )
endif(SMTG_ENABLE_VSTGUI_SUPPORT)
# -------------------

smtg_target_add_plugin_snapshots (WetReverb
    RESOURCES
        resource/C8D4E21F8A72B45A9F18C23D5B6E4F01_snapshot.png
        resource/C8D4E21F8A72B45A9F18C23D5B6E4F01_snapshot_2.0x.png
)

target_link_libraries(WetReverb
    PRIVATE
        sdk
)

smtg_target_configure_version_file(WetReverb)

if(SMTG_MAC)
    smtg_target_set_bundle(WetReverb
        BUNDLE_IDENTIFIER org.yonie.vst3.wetreverb
        COMPANY_NAME "Yonie"
    )
    smtg_target_set_debug_executable(WetReverb
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(WetReverb PRIVATE 
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT WetReverb)

        smtg_target_set_debug_executable(WetReverb
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
elseif(SMTG_LINUX)
    # Linux uses default settings
    # Plugin will be built to: build/VST3/WetReverb.vst3/Contents/x86_64-linux/WetReverb.so
    # Install location: ~/.vst3/ or /usr/lib/vst3/
endif()