// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <cstdlib>
#include "Misc/WindowsPaths.h"

namespace file {
namespace {
std::string portablePath(std::string path)
{
    for (char& separator : path)
        if (separator == '\\') separator = '/';
    return path;
}
}

std::string windowsUserDirectory(bool configuration)
{
    const char* environment = std::getenv(configuration ? "APPDATA" : "LOCALAPPDATA");
    char folder[MAX_PATH];
    std::string base;
    if (environment && *environment)
        base = environment;
    else if (SUCCEEDED(SHGetFolderPathA(nullptr,
             (configuration ? CSIDL_APPDATA : CSIDL_LOCAL_APPDATA) | CSIDL_FLAG_CREATE,
             nullptr, SHGFP_TYPE_CURRENT, folder)))
        base = folder;
    if (base.empty()) return {};
    return portablePath(base) + "/Yoshimi";
}

std::string windowsFactoryDirectory()
{
    // Resolve our module, not the host executable or its current directory.
    HMODULE module = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&windowsFactoryDirectory), &module))
        return {};
    char filename[32768];
    DWORD length = GetModuleFileNameA(module, filename, sizeof(filename));
    if (!length || length >= sizeof(filename)) return {};
    std::string path = portablePath(std::string(filename, length));
    return path.substr(0, path.rfind('/')) + "/resources";
}

void windowsOpenDocument(const std::string& filename)
{
    ShellExecuteA(nullptr, "open", filename.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
}
#endif
