// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef YOSHIMI_WINDOWS_PATHS_H
#define YOSHIMI_WINDOWS_PATHS_H

#ifdef _WIN32
#include <string>
namespace file {
std::string windowsUserDirectory(bool configuration);
std::string windowsFactoryDirectory();
void windowsOpenDocument(const std::string& filename);
}
#endif
#endif
