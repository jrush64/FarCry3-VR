#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ID3D11Device;

namespace fc3aer
{
enum class Eye
{
    Left,
    Right,
};

bool InstallFrameHooks(ID3D11Device* device, HMODULE proxyModule);
Eye CurrentEye();
void Shutdown();
}
