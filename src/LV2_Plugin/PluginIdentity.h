#ifndef YOSHIMI_LV2_PLUGIN_IDENTITY_H
#define YOSHIMI_LV2_PLUGIN_IDENTITY_H

#if defined(_WIN32) && defined(YOSHIMI_LV2_WINDOWS_RESOURCE_TEST)
#define YOSHIMI_LV2_PLUGIN_URI "http://yoshimi.sourceforge.net/lv2_plugin_windows_resources_20260930c"
#elif defined(_WIN32) && defined(YOSHIMI_LV2_WINDOWS_DIAGNOSTICS)
// Keep this in sync with the opt-in diagnostic metadata in CMakeLists.txt.
#define YOSHIMI_LV2_PLUGIN_URI "http://yoshimi.sourceforge.net/lv2_plugin_windows_diagnostic_20260930a"
#elif defined(_WIN32) && defined(YOSHIMI_LV2_WINDOWS_CLEAN_TEST)
#define YOSHIMI_LV2_PLUGIN_URI "http://yoshimi.sourceforge.net/lv2_plugin_windows_clean_20260930b"
#else
#define YOSHIMI_LV2_PLUGIN_URI "http://yoshimi.sourceforge.net/lv2_plugin"
#endif

#endif
