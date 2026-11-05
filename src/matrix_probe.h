#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ID3D11Device;

namespace matrixprobe
{
using LogLineFn = void (*)(const char* line);

bool ArmDevice(ID3D11Device* device);
bool Install(ID3D11Device* device, LogLineFn logLine);
void OnPresent(LONG frameNumber);
void Shutdown();
}
