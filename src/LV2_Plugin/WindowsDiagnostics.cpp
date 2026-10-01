// Temporary, opt-in diagnostics for the native Windows LV2 port.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "WindowsDiagnostics.h"

namespace {
void writeFile(const wchar_t* path, const char* text, DWORD length)
{
    HANDLE file = CreateFileW(path, FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(file, text, length, &written, nullptr);
    CloseHandle(file);
}

void appendBesideDLL(HMODULE module, const char* text, DWORD length)
{
    wchar_t path[32768];
    DWORD count = GetModuleFileNameW(module, path, 32768);
    const wchar_t suffix[] = L".attach.log";
    if (!count || count + sizeof(suffix) / sizeof(wchar_t) >= 32768) return;
    for (unsigned i = 0; i < sizeof(suffix) / sizeof(wchar_t); ++i)
        path[count + i] = suffix[i];
    writeFile(path, text, length);
}

void append(const char* text, DWORD length)
{
    wchar_t path[32768];
    DWORD count = GetEnvironmentVariableW(L"TEMP", path, 32768);
    if (!count || count >= 32768) count = GetTempPathW(32768, path);
    const wchar_t name[] = L"yoshimi-windows-diagnostic-20260930a.log";
    if (!count || count + sizeof(name) / sizeof(wchar_t) >= 32768) return;
    if (path[count - 1] != L'\\' && path[count - 1] != L'/') path[count++] = L'\\';
    for (unsigned i = 0; i < sizeof(name) / sizeof(wchar_t); ++i)
        path[count + i] = name[i];
    writeFile(path, text, length);
}
}

void yoshimiLV2Trace(const char* format, ...)
{
    DWORD savedError = GetLastError();
    char line[8192];
    SYSTEMTIME time;
    GetLocalTime(&time);
    int prefix = snprintf(line, sizeof(line),
        "%04u-%02u-%02u %02u:%02u:%02u.%03u pid=%lu tid=%lu ",
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
        time.wSecond, time.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId());
    va_list args;
    va_start(args, format);
    vsnprintf(line + prefix, sizeof(line) - prefix - 3, format, args);
    va_end(args);
    size_t length = strlen(line);
    line[length++] = '\r';
    line[length++] = '\n';
    append(line, static_cast<DWORD>(length));
    SetLastError(savedError);
}

void yoshimiLV2TraceEnvironment()
{
    char path[32768];
    if (GetModuleFileNameA(nullptr, path, sizeof(path)))
        yoshimiLV2Trace("host executable=%s", path);
    HMODULE module = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<const char*>(&yoshimiLV2TraceEnvironment), &module);
    if (module && GetModuleFileNameA(module, path, sizeof(path)))
        yoshimiLV2Trace("plugin binary=%s; diagnostic build %s %s", path, __DATE__, __TIME__);
    if (GetCurrentDirectoryA(sizeof(path), path))
        yoshimiLV2Trace("cwd=%s", path);
    for (const char* name : {"HOME", "USERPROFILE", "TEMP", "TMP", "APPDATA"})
    {
        DWORD size = GetEnvironmentVariableA(name, path, sizeof(path));
        yoshimiLV2Trace("env %s=%s", name, size && size < sizeof(path) ? path : "<unset/too long>");
    }
    // Record actual loaded modules to reveal host/plugin DLL name collisions.
    for (const char* name : {"libcairo-2.dll", "libfftw3f-3.dll", "libfltk-1.4.dll",
         "libfontconfig-1.dll", "libfreetype-6.dll", "libgcc_s_seh-1.dll",
         "libstdc++-6.dll", "libwinpthread-1.dll", "libglib-2.0-0.dll", "zlib1.dll"})
    {
        module = GetModuleHandleA(name);
        if (module && GetModuleFileNameA(module, path, sizeof(path)))
            yoshimiLV2Trace("dependency %s=%s", name, path);
        else
            yoshimiLV2Trace("dependency %s not loaded as a DLL (may be statically linked)", name);
    }
}

#ifdef __MINGW32__
// The Windows loader invokes TLS callbacks before the CRT DLL entry point,
// and therefore before C++ static constructors and our DllMain.
static void NTAPI diagnosticTLS(PVOID module, DWORD reason, PVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        const char message[] = "EARLY TLS DLL_PROCESS_ATTACH: Yoshimi Windows Diagnostic 20260930a (before CRT initialization)\r\n";
        append(message, sizeof(message) - 1);
        appendBesideDLL(static_cast<HMODULE>(module), message, sizeof(message) - 1);
        OutputDebugStringA(message);
    }
}
extern "C" {
    __attribute__((section(".CRT$XLB"), used))
    PIMAGE_TLS_CALLBACK yoshimiDiagnosticTLSCallback = diagnosticTLS;
}
#endif

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID)
{
    // Only Win32 file operations here: no CRT formatting, threads or loading DLLs.
    if (reason == DLL_PROCESS_ATTACH)
    {
        const char message[] = "DllMain DLL_PROCESS_ATTACH: Yoshimi Windows Diagnostic 20260930a\r\n";
        append(message, sizeof(message) - 1);
        appendBesideDLL(module, message, sizeof(message) - 1);
        OutputDebugStringA(message);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        const char message[] = "DLL_PROCESS_DETACH\r\n";
        append(message, sizeof(message) - 1);
    }
    return TRUE;
}
