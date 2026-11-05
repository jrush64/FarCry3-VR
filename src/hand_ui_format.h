#pragma once
#include <dxgi.h>
#include <cstdint>
namespace fc3handui {
inline DXGI_FORMAT ChooseFormat(DXGI_FORMAT source,const int64_t* offered,uint32_t count){const bool bgra=source==DXGI_FORMAT_B8G8R8A8_UNORM||source==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB||source==DXGI_FORMAT_B8G8R8A8_TYPELESS;const DXGI_FORMAT preferred[2]={bgra?DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,bgra?DXGI_FORMAT_B8G8R8A8_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM};for(int p=0;p<2;++p)for(uint32_t i=0;i<count;++i)if(offered[i]==preferred[p])return preferred[p];return DXGI_FORMAT_UNKNOWN;}
}
