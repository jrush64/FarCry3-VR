// Bounded real-draw diagnostic, no color modification. Include after D3D11 headers.
namespace fc3night {
struct Patch { UINT x,y,w,h; };
inline Patch PatchAt(UINT width,UINT height,UINT index) {
 const UINT w=width<64?width:64,h=height<64?height:64;
 const UINT x=index==0?(width-w)/2:(index==2||index==4?width-w:0);
 const UINT y=index==0?(height-h)/2:(index>=3?height-h:0);
 return {x,y,w,h};
}
struct Record {
 ID3D11Resource* staging=nullptr; ID3D11Query* ready=nullptr;
 unsigned long long shader=0,draw=0,resource=0,context=0;
 LONG frame=0;UINT eye=0,slot=0,width=0,height=0,depth=0,format=0,viewFormat=0,subresource=0,bytes=0;
 D3D11_DEVICE_CONTEXT_TYPE contextType=D3D11_DEVICE_CONTEXT_IMMEDIATE;
 bool post=false,buffer=false,volume=false;unsigned attempts=0;
};
static Record records[24];static UINT count=0;static bool claimed[2][2]={};
inline UINT PixelBytes(DXGI_FORMAT f){switch(f){case DXGI_FORMAT_R16G16B16A16_TYPELESS:case DXGI_FORMAT_R16G16B16A16_FLOAT:case DXGI_FORMAT_R16G16B16A16_UNORM:return 8;case DXGI_FORMAT_R32G32B32A32_FLOAT:return 16;case DXGI_FORMAT_B8G8R8A8_TYPELESS:case DXGI_FORMAT_B8G8R8A8_UNORM:case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:case DXGI_FORMAT_R8G8B8A8_TYPELESS:case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:case DXGI_FORMAT_R32_FLOAT:case DXGI_FORMAT_R32_TYPELESS:case DXGI_FORMAT_R24G8_TYPELESS:case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:return 4;default:return 0;}}
inline void Capture(ID3D11DeviceContext* c,ID3D11Resource* resource,UINT viewFormat,UINT mip,Record r){
 if(!resource||count>=24)return;ID3D11Device* d=nullptr;c->GetDevice(&d);if(!d)return;
 D3D11_RESOURCE_DIMENSION dim;resource->GetType(&dim);HRESULT hr=E_FAIL;
 if(dim==D3D11_RESOURCE_DIMENSION_TEXTURE2D){
  D3D11_TEXTURE2D_DESC desc={};static_cast<ID3D11Texture2D*>(resource)->GetDesc(&desc);
  if(desc.SampleDesc.Count!=1||desc.ArraySize!=1||mip>=desc.MipLevels){d->Release();return;}
  r.width=desc.Width>>mip;r.height=desc.Height>>mip;if(!r.width)r.width=1;if(!r.height)r.height=1;
  r.format=desc.Format;r.subresource=mip;r.bytes=PixelBytes(desc.Format);if(!r.bytes){d->Release();return;}
  desc.Width=320;desc.Height=64;desc.MipLevels=1;desc.ArraySize=1;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  ID3D11Texture2D* t=nullptr;hr=d->CreateTexture2D(&desc,nullptr,&t);r.staging=t;
  if(SUCCEEDED(hr)){for(UINT i=0;i<5;++i){Patch p=PatchAt(r.width,r.height,i);D3D11_BOX box={p.x,p.y,0,p.x+p.w,p.y+p.h,1};c->CopySubresourceRegion(t,0,i*64,0,0,resource,mip,&box);}}
 }else if(dim==D3D11_RESOURCE_DIMENSION_TEXTURE3D){
  D3D11_TEXTURE3D_DESC desc={};static_cast<ID3D11Texture3D*>(resource)->GetDesc(&desc);
  if(mip>=desc.MipLevels){d->Release();return;}
  r.volume=true;r.width=desc.Width>>mip;r.height=desc.Height>>mip;r.depth=desc.Depth>>mip;
  if(!r.width)r.width=1;if(!r.height)r.height=1;if(!r.depth)r.depth=1;
  r.bytes=PixelBytes(desc.Format);r.format=desc.Format;r.subresource=mip;
  if(!r.bytes||r.width>64||r.height>64||r.depth>64||r.width*r.height*r.depth*r.bytes>1024*1024){d->Release();return;}
  desc.Width=r.width;desc.Height=r.height;desc.Depth=r.depth;desc.MipLevels=1;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  ID3D11Texture3D* t=nullptr;hr=d->CreateTexture3D(&desc,nullptr,&t);r.staging=t;
  if(SUCCEEDED(hr)){D3D11_BOX box={0,0,0,r.width,r.height,r.depth};c->CopySubresourceRegion(t,0,0,0,0,resource,mip,&box);}
 }else if(dim==D3D11_RESOURCE_DIMENSION_BUFFER){
  D3D11_BUFFER_DESC bd={};static_cast<ID3D11Buffer*>(resource)->GetDesc(&bd);if(bd.ByteWidth>4096){d->Release();return;}
  r.buffer=true;r.bytes=bd.ByteWidth;bd.Usage=D3D11_USAGE_STAGING;bd.BindFlags=0;bd.MiscFlags=0;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  ID3D11Buffer* b=nullptr;hr=d->CreateBuffer(&bd,nullptr,&b);r.staging=b;if(SUCCEEDED(hr))c->CopyResource(b,resource);
 }
 if(SUCCEEDED(hr)&&r.staging){D3D11_QUERY_DESC q={D3D11_QUERY_EVENT,0};if(SUCCEEDED(d->CreateQuery(&q,&r.ready))){c->End(r.ready);r.resource=reinterpret_cast<unsigned long long>(resource);r.context=reinterpret_cast<unsigned long long>(c);r.contextType=c->GetType();r.viewFormat=viewFormat;records[count++]=r;}else r.staging->Release();}
 d->Release();
}
inline bool BeginActualDraw(ID3D11DeviceContext* c,unsigned long long shader,LONG frame,unsigned long long draw,UINT eye,bool stable){
 int which=shader==0x6A620A60024D1B32ull?0:shader==0xC5D7AC9B41B47C00ull?1:-1;
 if(which<0||eye>1||!stable||frame<600||claimed[which][eye])return false;claimed[which][eye]=true;
 Record r;r.shader=shader;r.frame=frame;r.draw=draw;r.eye=eye;
 ID3D11Buffer* cb=nullptr;c->PSGetConstantBuffers(0,1,&cb);r.slot=100;if(cb){Capture(c,cb,0,0,r);cb->Release();}
 for(UINT s=0;s<3;++s){ID3D11ShaderResourceView* v=nullptr;c->PSGetShaderResources(s,1,&v);if(!v)continue;
  D3D11_SHADER_RESOURCE_VIEW_DESC vd={};v->GetDesc(&vd);ID3D11Resource* resource=nullptr;v->GetResource(&resource);
  r.slot=s;if(vd.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D)Capture(c,resource,vd.Format,vd.Texture2D.MostDetailedMip,r);
  else if(vd.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE3D)Capture(c,resource,vd.Format,vd.Texture3D.MostDetailedMip,r);
  if(resource)resource->Release();v->Release();}
 return true;
}
inline void EndActualDraw(ID3D11DeviceContext* c,unsigned long long shader,LONG frame,unsigned long long draw,UINT eye,bool armed){
 if(!armed)return;ID3D11RenderTargetView* v=nullptr;c->OMGetRenderTargets(1,&v,nullptr);if(!v)return;
 D3D11_RENDER_TARGET_VIEW_DESC vd={};v->GetDesc(&vd);ID3D11Resource* resource=nullptr;v->GetResource(&resource);
 Record r;r.shader=shader;r.frame=frame;r.draw=draw;r.eye=eye;r.post=true;
 if(vd.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2D)Capture(c,resource,vd.Format,vd.Texture2D.MipSlice,r);
 if(resource)resource->Release();v->Release();
}
inline void FlushOnPresent(ID3D11DeviceContext* immediate,const wchar_t* folder){
 if(!immediate||immediate->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||!folder)return;
 for(UINT i=0;i<count;++i){Record& r=records[i];if(!r.staging)continue;BOOL ready=FALSE;HRESULT hr=immediate->GetData(r.ready,&ready,sizeof(ready),D3D11_ASYNC_GETDATA_DONOTFLUSH);
  if(hr!=S_OK||!ready){if(++r.attempts<180)continue;r.staging->Release();r.ready->Release();r.staging=nullptr;continue;}
  D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(immediate->Map(r.staging,0,D3D11_MAP_READ,0,&map))){if(++r.attempts>=180){r.staging->Release();r.ready->Release();r.staging=nullptr;}continue;}
  wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\fc3_night_%016llX_f%ld_d%llu_e%u_%s_s%u.bin",folder,r.shader,r.frame,r.draw,r.eye,r.post?L"post":L"pre",r.slot);
  FILE* f=nullptr;_wfopen_s(&f,path,L"wb");if(f){if(r.buffer)fwrite(map.pData,1,r.bytes,f);else if(r.volume){for(UINT z=0;z<r.depth;++z)for(UINT y=0;y<r.height;++y)fwrite(static_cast<const unsigned char*>(map.pData)+z*map.DepthPitch+y*map.RowPitch,1,r.width*r.bytes,f);}else for(UINT p=0;p<5;++p){Patch box=PatchAt(r.width,r.height,p);for(UINT y=0;y<box.h;++y)fwrite(static_cast<const unsigned char*>(map.pData)+y*map.RowPitch+p*64*r.bytes,1,box.w*r.bytes,f);}fclose(f);}
  immediate->Unmap(r.staging,0);
  _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\fc3_night_%016llX_f%ld_d%llu_e%u_%s_s%u.txt",folder,r.shader,r.frame,r.draw,r.eye,r.post?L"post":L"pre",r.slot);
  _wfopen_s(&f,path,L"wt");if(f){fprintf(f,"shader=%016llX frame=%ld draw=%llu eye=%u phase=%s slot=%u resource=%llX draw_context=%llX context_type=%u\nresource_format=%u view_format=%u subresource=%u width=%u height=%u depth=%u bytes=%u volume=%u\nGPU event completed before immediate-context map; deferred submission execution established by completion, not CPU frame alone.\n",r.shader,r.frame,r.draw,r.eye,r.post?"post":"pre",r.slot,r.resource,r.context,r.contextType,r.format,r.viewFormat,r.subresource,r.width,r.height,r.depth,r.bytes,r.volume?1:0);if(!r.buffer&&!r.volume)for(UINT p=0;p<5;++p){Patch box=PatchAt(r.width,r.height,p);fprintf(f,"patch%u x=%u y=%u width=%u height=%u\n",p,box.x,box.y,box.w,box.h);}fclose(f);}
  r.staging->Release();r.ready->Release();r.staging=nullptr;
 }
}
}
