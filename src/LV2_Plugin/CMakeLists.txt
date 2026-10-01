#
#   CMakeLists.txt (LV2)
#
#   Copyright 2014, Andrew Deryabin <andrewderyabin@gmail.com>
#   Copyright 2018-2023, Will Godfrey & others
#   Copyright 2024 Kristian Amlie
#
#   This file is part of yoshimi, which is free software: you can
#   redistribute it and/or modify it under the terms of the GNU General
#   Public License as published by the Free Software Foundation, either
#   version 2 of the License, or (at your option) any later version.
#
#   yoshimi is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.

file (GLOB yoshimi_lv2_plugin_files
        YoshimiLV2Plugin.cpp
        YoshimiLV2Plugin.h
        lv2extui.h
        lv2extprg.h)
file (GLOB yoshimi_dsp_files
    ../DSP/AnalogFilter.cpp  ../DSP/Filter.cpp  ../DSP/FormantFilter.cpp
    ../DSP/SVFilter.cpp  ../DSP/Unison.cpp
    ../DSP/FFTwrapper.h  ../DSP/AnalogFilter.h  ../DSP/FormantFilter.h
    ../DSP/SVFilter.h  ../DSP/Filter.h  ../DSP/Unison.h)
file (GLOB yoshimi_effects_files
    ../Effects/Alienwah.cpp  ../Effects/Chorus.cpp  ../Effects/Echo.cpp
    ../Effects/EffectLFO.cpp  ../Effects/EffectMgr.cpp  ../Effects/Effect.cpp
    ../Effects/Phaser.cpp  ../Effects/Reverb.cpp  ../Effects/EQ.cpp
    ../Effects/Distorsion.cpp  ../Effects/DynamicFilter.cpp
    ../Effects/Alienwah.h  ../Effects/Chorus.h  ../Effects/Echo.h
    ../Effects/EffectLFO.h  ../Effects/EffectMgr.h  ../Effects/Effect.h
    ../Effects/Phaser.h  ../Effects/Reverb.h  ../Effects/EQ.h
    ../Effects/Distorsion.h  ../Effects/DynamicFilter.h)
file (GLOB yoshimi_misc_files
    ../Misc/Alloc.h ../Misc/Bank.cpp ../Misc/Bank.h
    ../Misc/BuildScheduler.cpp ../Misc/BuildScheduler.h ../Misc/DataBlockBuff.h
    ../Misc/Config.cpp ../Misc/Config.h ../Misc/ConfBuild.h
    ../Misc/InstanceManager.cpp ../Misc/InstanceManager.h
    ../Misc/Microtonal.cpp ../Misc/Microtonal.h ../Misc/MirrorData.h
    ../Misc/SynthEngine.cpp ../Misc/SynthEngine.h
    ../Misc/Part.cpp ../Misc/Part.h../Misc/TestInvoker.h ../Misc/TestSequence.h
    ../Misc/WavFile.cpp ../Misc/WavFile.h ../Misc/WaveShapeSamples.h ../Misc/WindowsPaths.cpp
    ../Misc/XMLStore.cpp ../Misc/XMLStore.h)
file (GLOB yoshimi_interface_files
    ../Interface/GuiDataExchange.cpp ../Interface/GuiDataExchange.h
    ../Interface/InterfaceAnchor.h
    ../Interface/MidiDecode.cpp ../Interface/MidiDecode.h
    ../Interface/Vectors.cpp ../Interface/Vectors.h
    ../Interface/InterChange.cpp ../Interface/InterChange.h
    ../Interface/Data2Text.cpp ../Interface/Data2text.h
    ../Interface/Text2Data.cpp ../Interface/Text2Data.h
    ../Interface/TextLists.cpp ../Interface/TextLists.h
    ../Interface/RingBuffer.h
    ../Interface/MidiLearn.cpp ../Interface/MidiLearn.h
    ../UI/MiscGui.cpp ../UI/MiscGui.h)
file (GLOB yoshimi_params_files
    ../Params/ADnoteParameters.cpp  ../Params/EnvelopeParams.cpp
    ../Params/FilterParams.cpp  ../Params/LFOParams.cpp
    ../Params/SUBnoteParameters.cpp  ../Params/PADnoteParameters.cpp
    ../Params/Controller.cpp  ../Params/ParamCheck.cpp  ../Params/UnifiedPresets.cpp
    ../Params/ADnoteParameters.h  ../Params/EnvelopeParams.h
    ../Params/FilterParams.h  ../Params/LFOParams.h
    ../Params/SUBnoteParameters.h
    ../Params/PADnoteParameters.h ../Params/PADStatus.h ../Params/RandomWalk.h
    ../Params/OscilParameters.cpp ../Params/OscilParameters.h
    ../Params/Controller.h  ../Params/ParamCheck.h ../Params/UnifiedPresets.h)
file (GLOB yoshimi_synth_files
    ../Synth/ADnote.cpp  ../Synth/Envelope.cpp  ../Synth/LFO.cpp  ../Synth/OscilGen.cpp
    ../Synth/SUBnote.cpp  ../Synth/Resonance.cpp  ../Synth/PADnote.cpp
    ../Synth/ADnote.h  ../Synth/Envelope.h  ../Synth/LFO.h  ../Synth/OscilGen.h
    ../Synth/SUBnote.h  ../Synth/Resonance.h  ../Synth/PADnote.h
    ../Synth/WaveInterpolator.h ../Synth/XFadeManager.h ../Synth/BodyDisposal.h)
file (GLOB yoshimi_musicio_files
    ../MusicIO/MusicClient.cpp ../MusicIO/MusicClient.h
    ../MusicIO/MusicIO.cpp ../MusicIO/MusicIO.h)
set (yoshimi_manifest_ttl "${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl")
set (yoshimi_plugin_ttl "${CMAKE_CURRENT_BINARY_DIR}/yoshimi.ttl")
option(YOSHIMI_LV2_WINDOWS_DIAGNOSTICS "Log Windows LV2 initialization to TEMP" OFF)
option(YOSHIMI_LV2_WINDOWS_CLEAN_TEST "Use a fresh Windows LV2 test identity without logging" OFF)
option(YOSHIMI_LV2_WINDOWS_RESOURCE_TEST "Use a fresh Windows LV2 resource test identity" OFF)
if(WIN32 AND YOSHIMI_LV2_WINDOWS_DIAGNOSTICS AND YOSHIMI_LV2_WINDOWS_CLEAN_TEST)
    message(FATAL_ERROR "Select either Windows diagnostics or the clean test identity")
endif()
if(WIN32 AND YOSHIMI_LV2_WINDOWS_RESOURCE_TEST AND
   (YOSHIMI_LV2_WINDOWS_DIAGNOSTICS OR YOSHIMI_LV2_WINDOWS_CLEAN_TEST))
    message(FATAL_ERROR "Select only one Windows LV2 test identity")
endif()
if(WIN32 AND YOSHIMI_LV2_WINDOWS_RESOURCE_TEST)
    set(windows_lv2_uri "http://yoshimi.sourceforge.net/lv2_plugin_windows_resources_20260930c")
    set(windows_lv2_binary "yoshimi_windows_resources_20260930c")
    set(windows_lv2_name "Yoshimi Windows Resources Test")
elseif(WIN32 AND YOSHIMI_LV2_WINDOWS_DIAGNOSTICS)
    set(windows_lv2_uri "http://yoshimi.sourceforge.net/lv2_plugin_windows_diagnostic_20260930a")
    set(windows_lv2_binary "yoshimi_windows_diagnostic_20260930a")
    set(windows_lv2_name "Yoshimi Windows Diagnostic")
elseif(WIN32 AND YOSHIMI_LV2_WINDOWS_CLEAN_TEST)
    set(windows_lv2_uri "http://yoshimi.sourceforge.net/lv2_plugin_windows_clean_20260930b")
    set(windows_lv2_binary "yoshimi_windows_clean_20260930b")
    set(windows_lv2_name "Yoshimi Windows Clean Test")
endif()
configure_file(manifest.ttl "${yoshimi_manifest_ttl}" COPYONLY)
configure_file(yoshimi.ttl "${yoshimi_plugin_ttl}" COPYONLY)
if(WIN32)
    file(READ "${yoshimi_manifest_ttl}" manifest_contents)
    string(REPLACE "yoshimi_lv2.so" "yoshimi_lv2.dll"
           manifest_contents "${manifest_contents}")
    file(WRITE "${yoshimi_manifest_ttl}" "${manifest_contents}")
    file(READ "${yoshimi_plugin_ttl}" plugin_contents)
    # The LV2 options specification defines supportedOption (singular).
    string(REPLACE "opts:supportedOptions" "opts:supportedOption"
        plugin_contents "${plugin_contents}")
    file(WRITE "${yoshimi_plugin_ttl}" "${plugin_contents}")
    if(YOSHIMI_LV2_WINDOWS_DIAGNOSTICS OR YOSHIMI_LV2_WINDOWS_CLEAN_TEST OR YOSHIMI_LV2_WINDOWS_RESOURCE_TEST)
        # Distinct names and URIs keep this test separate from production caches.
        foreach(metadata ${yoshimi_manifest_ttl} ${yoshimi_plugin_ttl})
            file(READ "${metadata}" diagnostic_contents)
            string(REPLACE "http://yoshimi.sourceforge.net/lv2_plugin"
                "${windows_lv2_uri}"
                diagnostic_contents "${diagnostic_contents}")
            string(REPLACE "yoshimi_lv2.dll" "${windows_lv2_binary}.dll"
                diagnostic_contents "${diagnostic_contents}")
            string(REPLACE "doap:name \"Yoshimi\"" "doap:name \"${windows_lv2_name}\""
                diagnostic_contents "${diagnostic_contents}")
            string(REPLACE "doap:name \"Yoshimi-Multi\"" "doap:name \"${windows_lv2_name} Multi\""
                diagnostic_contents "${diagnostic_contents}")
            file(WRITE "${metadata}" "${diagnostic_contents}")
        endforeach()
    endif()
endif()

set (FltkUI_names
    "PresetsUI"  "EnvelopeUI"
    "LFOUI"  "FilterUI"  "VirKeyboardUI"
    "ConfigUI"  "SUBnoteUI"  "ResonanceUI"
    "OscilGenUI"  "ADnoteUI"  "PADnoteUI"
    "EffUI"  "BankUI"  "PartUI"
    "MicrotonalUI"  "MasterUI" "MasterMiscUI"
    "ParametersUI" "ConsoleUI" "VectorUI"
    "MidiLearnUI"
)

# workaround fltk_wrap_ui breakage
set (FltkUI_sources)
set (FltkUI_headers)
foreach(basename ${FltkUI_names})
    set (fluidfile "${CMAKE_CURRENT_SOURCE_DIR}/../UI/${basename}.fl")
    set (sourcefile "${basename}.cpp")
    set (headerfile "${basename}.h")
    add_custom_command(
      OUTPUT ${sourcefile}
      COMMAND ${FLTK_FLUID_EXECUTABLE} ARGS -c -o .cpp ${fluidfile}
      DEPENDS ${fluidfile}
    )
    set (FltkUI_sources ${FltkUI_sources} "${sourcefile}")
    set (FltkUI_headers ${FltkUI_headers} "${headerfile}")
endforeach(basename ${FltkUI_names})

set (FltkUI_sources
    ${FltkUI_sources}
    ../UI/DynamicTooltip.cpp
    ../UI/WidgetPDial.cpp
    ../UI/WidgetMWSlider.cpp
    ../UI/WidgetCheckButton.cpp
    ../UI/WidgetSpinner.cpp
    ../UI/ScaleTrackedWindow.cpp
)

add_definitions (-DYOSHIMI_LV2_PLUGIN=1)

add_library (yoshimi_lv2 MODULE
            ${yoshimi_lv2_plugin_files}
            ${yoshimi_manifest_ttl}
            ${yoshimi_plugin_ttl}
            ${yoshimi_dsp_files}
            ${yoshimi_effects_files}
            ${yoshimi_misc_files}
            ${yoshimi_interface_files}
            ${yoshimi_params_files}
            ${yoshimi_synth_files}
            ${yoshimi_musicio_files}
            ${FltkUI_sources})

if(WIN32 AND YOSHIMI_LV2_WINDOWS_DIAGNOSTICS)
    target_sources(yoshimi_lv2 PRIVATE WindowsDiagnostics.cpp)
    target_compile_definitions(yoshimi_lv2 PRIVATE YOSHIMI_LV2_WINDOWS_DIAGNOSTICS=1)
endif()
if(WIN32 AND YOSHIMI_LV2_WINDOWS_CLEAN_TEST)
    target_compile_definitions(yoshimi_lv2 PRIVATE YOSHIMI_LV2_WINDOWS_CLEAN_TEST=1)
endif()
if(WIN32 AND YOSHIMI_LV2_WINDOWS_RESOURCE_TEST)
    target_compile_definitions(yoshimi_lv2 PRIVATE YOSHIMI_LV2_WINDOWS_RESOURCE_TEST=1)
endif()
if(WIN32 AND (YOSHIMI_LV2_WINDOWS_DIAGNOSTICS OR YOSHIMI_LV2_WINDOWS_CLEAN_TEST OR YOSHIMI_LV2_WINDOWS_RESOURCE_TEST))
    set_target_properties(yoshimi_lv2 PROPERTIES OUTPUT_NAME "${windows_lv2_binary}")
endif()

add_custom_target(yoshimi_lv2_ttl
                   DEPENDS ${yoshimi_manifest_ttl} ${yoshimi_plugin_ttl})

SET_TARGET_PROPERTIES(yoshimi_lv2 PROPERTIES PREFIX "")
include_directories (AFTER
                            ${LV2_INCLUDE_DIRS}
                            ${FLTK_INCLUDE_DIR}
                            ${MXML_INCLUDE_DIRS}
                            ${FONTCONFIG_INCLUDE_DIRS}
                            ${FFTW3F_INC_DIR}
                            ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_SOURCE_DIR})

if(MINGW)
    # An LV2 host may use plain LoadLibrary, which does not search the bundle
    # for dependencies. Keep MinGW runtime and third-party code in the module.
    pkg_check_modules(WINDOWS_LV2_DEPS REQUIRED cairo fontconfig fftw3f mxml zlib)
    find_library(WINDOWS_LV2_FLTK NAMES libfltk.a)
    if(NOT WINDOWS_LV2_FLTK)
        message(FATAL_ERROR "Windows LV2 requires the static FLTK library (libfltk.a)")
    endif()
    target_compile_definitions(yoshimi_lv2 PRIVATE CAIRO_WIN32_STATIC_BUILD)
    set_property(TARGET yoshimi_lv2 APPEND_STRING PROPERTY LINK_FLAGS
                 " -static -static-libgcc -static-libstdc++")
    foreach(directory ${WINDOWS_LV2_DEPS_STATIC_LIBRARY_DIRS})
        target_link_libraries(yoshimi_lv2 PRIVATE "-L${directory}")
    endforeach()
    target_link_libraries(yoshimi_lv2 PRIVATE
        ${WINDOWS_LV2_FLTK}
        # Static gettext (via GLib/Cairo) also needs iconv.
        -Wl,--start-group ${WINDOWS_LV2_DEPS_STATIC_LIBRARIES} iconv -Wl,--end-group
        ${WINDOWS_LV2_DEPS_STATIC_LDFLAGS_OTHER} comctl32 ws2_32 shell32)
else()
    target_link_libraries(yoshimi_lv2
                          ${LV2_LIBRARIES}
                          ${FLTK_LIBRARIES}
                          ${MXML_LIBRARIES}
                          ${LIBSNDFILE_LIBRARIES}
                          ${FFTW3F_LIBRARIES}
                          ${LIBCAIRO_LIBRARIES}
                          z
                          ${LIBDL_LINUX_LIBS}
    )
endif()

add_dependencies(yoshimi_lv2 yoshimi_lv2_ttl)

set(yoshimi_lv2_bundle "yoshimi.lv2")
if(WIN32 AND DEFINED windows_lv2_binary)
    string(REPLACE "_" "-" yoshimi_lv2_bundle "${windows_lv2_binary}")
    set(yoshimi_lv2_bundle "${yoshimi_lv2_bundle}.lv2")
endif()
install (TARGETS yoshimi_lv2 DESTINATION ${LV2_INSTALL_DIR}/${yoshimi_lv2_bundle} COMPONENT lv2)
install (FILES ${yoshimi_manifest_ttl} ${yoshimi_plugin_ttl}
  DESTINATION ${LV2_INSTALL_DIR}/${yoshimi_lv2_bundle} COMPONENT lv2
)

if(WIN32)
    install(DIRECTORY ../../banks ../../presets ../../doc/examples
        DESTINATION ${LV2_INSTALL_DIR}/${yoshimi_lv2_bundle}/resources COMPONENT lv2)
    install(DIRECTORY ../../doc/ DESTINATION ${LV2_INSTALL_DIR}/${yoshimi_lv2_bundle}/resources/doc COMPONENT lv2)
    install(FILES ../../COPYING DESTINATION ${LV2_INSTALL_DIR}/${yoshimi_lv2_bundle}/resources COMPONENT lv2)
endif()

set_directory_properties(PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${FltkUI_headers}")
