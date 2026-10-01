#ifndef YOSHIMI_LV2_WINDOWS_DIAGNOSTICS_H
#define YOSHIMI_LV2_WINDOWS_DIAGNOSTICS_H

#if defined(_WIN32) && defined(YOSHIMI_LV2_WINDOWS_DIAGNOSTICS)
void yoshimiLV2Trace(const char* format, ...);
void yoshimiLV2TraceEnvironment();
#else
#define yoshimiLV2Trace(...) ((void)0)
#define yoshimiLV2TraceEnvironment() ((void)0)
#endif

#endif
