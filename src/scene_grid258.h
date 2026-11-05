#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <cstring>

// Bounded diagnostic only. Read the actual colour target at sample zero;
// preserve all compute/output bindings and never modify the scene resource.
namespace fc3grid258 {
template<class T> inline void Release(T*& p){if(p)p->Release();p=nullptr;}
struct Cache {
 ID3D11Device* device=nullptr;ID3D11ComputeShader* shader[2]={};
 void Reset(){for(auto& s:shader)Release(s);device=nullptr;}
};
inline HRESULT Read(Cache& cache,ID3D11DeviceContext* ctx,ID3D11RenderTargetView* view,
                    std::vector<float>& pixels,D3D11_TEXTURE2D_DESC& sourceDesc){
 pixels.clear();sourceDesc={};
 if(!ctx||!view||ctx->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)return E_INVALIDARG;
 ID3D11Resource* resource=nullptr;view->GetResource(&resource);ID3D11Texture2D* texture=nullptr;
 HRESULT hr=resource->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&texture);Release(resource);if(FAILED(hr))return hr;
 texture->GetDesc(&sourceDesc);D3D11_RENDER_TARGET_VIEW_DESC vd={};view->GetDesc(&vd);
 const bool ms=sourceDesc.SampleDesc.Count>1;
 if(sourceDesc.ArraySize!=1||(ms?vd.ViewDimension!=D3D11_RTV_DIMENSION_TEXTURE2DMS:vd.ViewDimension!=D3D11_RTV_DIMENSION_TEXTURE2D)||!(sourceDesc.BindFlags&D3D11_BIND_SHADER_RESOURCE)){Release(texture);return E_INVALIDARG;}
 ID3D11Device* dev=nullptr;ctx->GetDevice(&dev);if(cache.device!=dev){cache.Reset();cache.device=dev;}
 if(!cache.shader[ms]){
  const char* code=ms?
   "Texture2DMS<float4> S:register(t0);RWTexture2D<float4> O:register(u0);[numthreads(8,8,1)]void main(uint3 p:SV_DispatchThreadID){uint w,h,n;S.GetDimensions(w,h,n);if(p.x<64&&p.y<36)O[p.xy]=S.Load(min(uint2(w-1,h-1),uint2((p.x+.5)*w/64,(p.y+.5)*h/36)),0);}":
   "Texture2D<float4> S:register(t0);RWTexture2D<float4> O:register(u0);[numthreads(8,8,1)]void main(uint3 p:SV_DispatchThreadID){uint w,h;S.GetDimensions(w,h);if(p.x<64&&p.y<36)O[p.xy]=S.Load(int3(min(uint2(w-1,h-1),uint2((p.x+.5)*w/64,(p.y+.5)*h/36)),0));}";
  ID3DBlob *blob=nullptr,*errors=nullptr;hr=D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"main","cs_5_0",0,0,&blob,&errors);Release(errors);
  if(SUCCEEDED(hr))hr=dev->CreateComputeShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&cache.shader[ms]);Release(blob);
 }
 ID3D11ShaderResourceView* srv=nullptr;D3D11_SHADER_RESOURCE_VIEW_DESC sd={};sd.Format=vd.Format;sd.ViewDimension=ms?D3D11_SRV_DIMENSION_TEXTURE2DMS:D3D11_SRV_DIMENSION_TEXTURE2D;
 if(!ms){sd.Texture2D.MostDetailedMip=vd.Texture2D.MipSlice;sd.Texture2D.MipLevels=1;}
 if(SUCCEEDED(hr))hr=dev->CreateShaderResourceView(texture,&sd,&srv);Release(texture);
 D3D11_TEXTURE2D_DESC td={64,36,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_UNORDERED_ACCESS,0,0};
 ID3D11Texture2D *output=nullptr,*staging=nullptr;ID3D11UnorderedAccessView* uav=nullptr;
 if(SUCCEEDED(hr))hr=dev->CreateTexture2D(&td,nullptr,&output);
 if(SUCCEEDED(hr))hr=dev->CreateUnorderedAccessView(output,nullptr,&uav);
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 if(SUCCEEDED(hr))hr=dev->CreateTexture2D(&td,nullptr,&staging);
 if(SUCCEEDED(hr)){
  ID3D11ComputeShader* old=nullptr;ID3D11ClassInstance* instances[256]={};UINT count=256;
  ID3D11ShaderResourceView* oldSrv=nullptr;ID3D11UnorderedAccessView* oldUav=nullptr;
  ID3D11RenderTargetView* targets[8]={};ID3D11DepthStencilView* depth=nullptr;
  ctx->CSGetShader(&old,instances,&count);ctx->CSGetShaderResources(0,1,&oldSrv);ctx->CSGetUnorderedAccessViews(0,1,&oldUav);ctx->OMGetRenderTargets(8,targets,&depth);
  ctx->OMSetRenderTargets(0,nullptr,nullptr);ctx->CSSetShader(cache.shader[ms],nullptr,0);ctx->CSSetShaderResources(0,1,&srv);ctx->CSSetUnorderedAccessViews(0,1,&uav,nullptr);ctx->Dispatch(8,5,1);
  ID3D11ShaderResourceView* noSrv=nullptr;ID3D11UnorderedAccessView* noUav=nullptr;ctx->CSSetShaderResources(0,1,&noSrv);ctx->CSSetUnorderedAccessViews(0,1,&noUav,nullptr);
  ctx->CSSetShader(old,instances,count);ctx->CSSetShaderResources(0,1,&oldSrv);ctx->CSSetUnorderedAccessViews(0,1,&oldUav,nullptr);ctx->OMSetRenderTargets(8,targets,depth);
  Release(old);Release(oldSrv);Release(oldUav);Release(depth);for(auto& target:targets)Release(target);for(UINT i=0;i<count;++i)Release(instances[i]);
  ctx->CopyResource(staging,output);D3D11_MAPPED_SUBRESOURCE map={};hr=ctx->Map(staging,0,D3D11_MAP_READ,0,&map);
  if(SUCCEEDED(hr)){pixels.resize(64*36*4);for(UINT y=0;y<36;++y)memcpy(pixels.data()+y*64*4,(const char*)map.pData+y*map.RowPitch,64*16);ctx->Unmap(staging,0);}
 }
 Release(staging);Release(output);Release(uav);Release(srv);Release(dev);return hr;
}
}
