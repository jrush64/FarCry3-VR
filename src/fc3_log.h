#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace fc3
{
// printf-style line logger. Thread-safe. Adds the line ending itself.
void Log(const char* format, ...);
// Folder that holds the log, the settings file and eye dumps (with trailing slash).
const wchar_t* DataFolder();
}
