#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ID3D11Device;
struct IDXGISwapChain;
struct DXGI_SWAP_CHAIN_DESC;

namespace fc3
{
// Called by the proxy for every D3D11 device the process creates.
bool OnDeviceCreated(ID3D11Device* device, HMODULE proxyModule);
// Called when a swapchain is created together with its device.
void OnSwapChainCreated(IDXGISwapChain* swapChain, ID3D11Device* device, const DXGI_SWAP_CHAIN_DESC* description);
void Shutdown();
}
