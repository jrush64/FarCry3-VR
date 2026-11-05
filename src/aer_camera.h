#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace aercamera
{
using LogLineFn = void (*)(const char* line);

bool Install(ID3D11Device* device, ID3D11DeviceContext* context, LogLineFn logLine);
void OnPresent(LONG frameNumber);
void Shutdown();
}
