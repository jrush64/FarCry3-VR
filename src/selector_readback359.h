#pragma once
#include <map>
// Render-thread staging snapshots. Only an already-completed GPU fence may be
// serialized; no gameplay Map is allowed to wait for a texture copy.
namespace fc3readback359 {
class Batch {
 std::map<ID3D11Resource*,ID3D11Resource*> copies;
 ID3D11Query* fence=nullptr;size_t bytes=0;
public:
 Batch()=default;Batch(const Batch&)=delete;Batch& operator=(const Batch&)=delete;
 ~Batch(){Clear();}
 void Clear(){for(auto& p:copies){p.first->Release();p.second->Release();}copies.clear();if(fence)fence->Release();fence=nullptr;bytes=0;}
 bool Empty()const{return copies.empty();}
 ID3D11Resource* Find(ID3D11Resource* source){auto it=copies.find(source);return it==copies.end()?nullptr:it->second;}
 bool Add(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Resource* source){
  if(!source||Find(source))return true;if(copies.size()>=512)return false;
  ID3D11Resource* target=nullptr;D3D11_RESOURCE_DIMENSION kind;source->GetType(&kind);size_t size=0;HRESULT hr=E_FAIL;
  if(kind==D3D11_RESOURCE_DIMENSION_BUFFER){
   D3D11_BUFFER_DESC desc={};static_cast<ID3D11Buffer*>(source)->GetDesc(&desc);size=desc.ByteWidth;
   if(!size||size>32u*1024*1024)return false;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=desc.StructureByteStride=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
   ID3D11Buffer* buffer=nullptr;hr=device->CreateBuffer(&desc,nullptr,&buffer);target=buffer;
  }else if(kind==D3D11_RESOURCE_DIMENSION_TEXTURE2D){
   D3D11_TEXTURE2D_DESC desc={};static_cast<ID3D11Texture2D*>(source)->GetDesc(&desc);
   if(desc.SampleDesc.Count!=1||!desc.Width||!desc.Height||!desc.MipLevels||desc.MipLevels>15||!desc.ArraySize||desc.ArraySize>12)return false;
   const UINT n=UINT(desc.Format);const bool bc=(n>=70&&n<=84)||(n>=94&&n<=99);
   const UINT pixel=n>=1&&n<=4?16:n>=5&&n<=8?12:n>=9&&n<=22?8:n>=23&&n<=47?4:n>=48&&n<=59?2:n>=60&&n<=65?1:n==67?4:n==85||n==86?2:n>=87&&n<=93?4:0;
   if(!bc&&!pixel)return false;
   for(UINT mip=0;mip<desc.MipLevels;++mip){const auto w=(std::max)(1u,desc.Width>>mip),h=(std::max)(1u,desc.Height>>mip);size+=size_t(bc?((w+3)/4)*((n<=72||(n>=79&&n<=81))?8:16):w*pixel)*(bc?(h+3)/4:h)*desc.ArraySize;}
   if(size>192u*1024*1024||bytes>192u*1024*1024-size)return false;
   desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* texture=nullptr;hr=device->CreateTexture2D(&desc,nullptr,&texture);target=texture;
  }
  if(FAILED(hr)||!target)return false;
  if(size>192u*1024*1024||bytes>192u*1024*1024-size){target->Release();return false;}
  context->CopyResource(target,source);source->AddRef();copies.emplace(source,target);bytes+=size;return true;
 }
 bool End(ID3D11Device* device,ID3D11DeviceContext* context){D3D11_QUERY_DESC d={D3D11_QUERY_EVENT,0};if(fence||FAILED(device->CreateQuery(&d,&fence)))return false;context->End(fence);return true;}
 int Ready(ID3D11DeviceContext* context){if(!fence)return -1;BOOL complete=FALSE;const HRESULT hr=context->GetData(fence,&complete,sizeof(complete),D3D11_ASYNC_GETDATA_DONOTFLUSH);return hr==S_OK&&complete?1:hr==S_FALSE||hr==S_OK?0:-1;}
};
inline Batch*& Current(){static thread_local Batch* batch=nullptr;return batch;}
struct Scope {Batch* previous;explicit Scope(Batch& batch):previous(Current()){Current()=&batch;}~Scope(){Current()=previous;}};
inline UINT MapFlags(){return Current()?D3D11_MAP_FLAG_DO_NOT_WAIT:0;}
}
