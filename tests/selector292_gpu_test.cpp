#define WIN32_LEAN_AND_MEAN
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include "../src/vr_pad.h"
#include "../src/xr_hand_tracking.h"
#include "../src/test_notes_ui.h"
#include "../src/selector290.h"
#include "../src/selector_policy290.h"
#include "../src/selector_scene292.h"
#include "../src/selector_passive295.h"
#include "../src/xr_body_pose.h"
void Check(bool b,const char* s){if(!b){fprintf(stderr,"FAIL: %s\n",s);exit(1);}printf("PASS: %s\n",s);}
void Log(const char*,...){}
namespace fc3 {const wchar_t* DataFolder(){static wchar_t p[1024]={};if(!p[0]){DWORD n=GetEnvironmentVariableW(L"FC3VR_TEST_DATA",p,1000);if(n==0||n>=1000)lstrcpyW(p,L".\\");else if(p[n-1]!=L'\\')lstrcatW(p,L"\\");}return p;}} // set FC3VR_TEST_DATA to the folder holding the captured test assets
namespace fc3stereo {float WorldScale(){return 1;}bool RunOverlay291(bool(*render)(void*),void* data){return render(data);}void CopyTexture(ID3D11Texture2D* a,ID3D11Texture2D* b){ID3D11Device* d=nullptr;a->GetDevice(&d);ID3D11DeviceContext* c=nullptr;d->GetImmediateContext(&c);c->CopyResource(a,b);c->Release();d->Release();}}
namespace fc3pad {bool fixtureMenu=false,active290=false;bool MenuOpen(){return fixtureMenu;}bool GameplayInputBlocked(){return fixtureMenu;}void SetSelector290(bool on){active290=on;}}
namespace fc3xr {void InteractionPulse285(int){} void SetTestNote(const wchar_t*){} }
namespace fc3select290 {bool fixtureDelay=false;Inventory fixtureInventory;int fixtureResult=0,fixtureGrab=-1,fixtureCalls=0;bool lease=false;void KeepAlive(bool on){lease=on;}Inventory Read(){auto i=fixtureInventory;i.at=GetTickCount64();return i;}bool Request(const Inventory& shown,int slot){if(!shown.valid||!shown.weapons[slot].entity)return false;fixtureGrab=slot;++fixtureCalls;fixtureResult=fixtureDelay?0:1;if(!fixtureDelay)fixtureInventory.equipped=shown.weapons[slot].entity;return true;}int Result(){return fixtureResult;}}
XrView g_views[2]={};bool g_viewsTracked=true,g_referenceValid=true;
XrSession g_session=static_cast<XrSession>(1);XrSpace g_space=static_cast<XrSpace>(2),g_handSpaces[2]={static_cast<XrSpace>(3),static_cast<XrSpace>(4)};XrTime g_displayTime=1;
UINT g_width=640,g_height=640;DXGI_FORMAT g_format=DXGI_FORMAT_R8G8B8A8_UNORM;
ID3D11Device* device=nullptr;ID3D11Texture2D* images[2]={};XrPosef hand={{0,0,0,1},{0,0,0}};bool tracked=true;UINT acquired=0,released=0;
XrResult XRAPI_CALL Create(XrSession,const XrSwapchainCreateInfo* info,XrSwapchain* out){Check(info->width==640&&info->height==640&&info->arraySize==2,"actual3D projection has two independent eye slices");D3D11_TEXTURE2D_DESC d={};d.Width=640;d.Height=640;d.ArraySize=2;d.MipLevels=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_RENDER_TARGET;for(auto& image:images)Check(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&image)),"WARP stereo compositor images");*out=static_cast<XrSwapchain>(5);return XR_SUCCESS;}
XrResult XRAPI_CALL Destroy(XrSwapchain){for(auto& i:images){if(i)i->Release();i=nullptr;}return XR_SUCCESS;}
XrResult XRAPI_CALL Enumerate(XrSwapchain,uint32_t capacity,uint32_t* count,XrSwapchainImageBaseHeader* out){*count=2;if(capacity){auto* d=reinterpret_cast<XrSwapchainImageD3D11KHR*>(out);for(UINT i=0;i<2;++i)d[i].texture=images[i];}return XR_SUCCESS;}
XrResult XRAPI_CALL Acquire(XrSwapchain,const XrSwapchainImageAcquireInfo*,uint32_t* index){*index=acquired++%2;return XR_SUCCESS;}
bool fail358=false;XrResult XRAPI_CALL Wait(XrSwapchain,const XrSwapchainImageWaitInfo*){return fail358?XR_ERROR_RUNTIME_FAILURE:XR_SUCCESS;}
XrResult XRAPI_CALL Release(XrSwapchain,const XrSwapchainImageReleaseInfo*){++released;return XR_SUCCESS;}
XrResult XRAPI_CALL Locate(XrSpace,XrSpace,XrTime,XrSpaceLocation* out){out->pose=hand;out->locationFlags=tracked?(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT):0;return XR_SUCCESS;}
auto p_xrCreateSwapchain=Create;auto p_xrDestroySwapchain=Destroy;auto p_xrEnumerateSwapchainImages=Enumerate;auto p_xrAcquireSwapchainImage=Acquire;auto p_xrWaitSwapchainImage=Wait;auto p_xrReleaseSwapchainImage=Release;auto p_xrLocateSpace=Locate;

float g_referenceYaw=0;XrVector3f g_referencePosition={0,0,0};
namespace fc3visual292 {
bool preparing330=false;bool Preparing330(){return preparing330;}void SetPreparing330(bool on){preparing330=on;}
WeaponVisual fixtureVisual[4];bool contact=false;int contactSlot=-1;float contactBody[16]={};
void Target(const fc3select290::Inventory&,int){}
const WeaponVisual* Read(const fc3select290::Inventory& inv,int slot){if(slot<0||slot>3)return nullptr;const auto& v=fixtureVisual[slot];return v.ready&&v.owner==inv.owner&&v.id==inv.weapons[slot].id&&v.entity==inv.weapons[slot].entity?&v:nullptr;}
void Contact(const float* body,int slot,float){contact=true;contactSlot=slot;memcpy(contactBody,body,64);}
void ClearContact(){contact=false;contactSlot=-1;}
bool diskPreload296=false;
void Preload296(const fc3select290::Inventory& inv){if(!diskPreload296)return;for(int i=0;i<4;++i){if(fixtureVisual[i].ready)continue;wchar_t path[100];swprintf_s(path,L"evidence/cache296-test-%d.fcv296",i);std::vector<BYTE> bytes;Check(fc3cache296::ReadFile(path,bytes)&&fc3cache296::Decode(device,bytes,inv.weapons[i].id,fixtureVisual[i]),"private disk cache restores actual native resource fixture");fixtureVisual[i].owner=inv.owner;fixtureVisual[i].entity=inv.weapons[i].entity;}}
int passiveSlot295=-1;void Passive295(const fc3select290::Inventory& inv){bool ready[4];for(int i=0;i<4;++i)ready[i]=Read(inv,i)!=nullptr;passiveSlot295=fc3passive295::EquippedMissing(inv,ready);}

}
#include "../src/weapon_hands303.h"
#include "../src/support_continuation356.h"
bool g_ammoPouch334=false,g_supportTracked342=false,g_handTriggerWait303=false,g_handGripWait303[2]={};
XrResult g_lastSync345=XR_SUCCESS;fc3hands303::Snapshot g_handsSnapshot303;
fc3hands303::Settings g_handSettings303={false,true};fc3hands303::State g_hands303;bool g_handsLoaded303=true,g_roleContext303=false,g_changeContext303=false;ULONGLONG g_roleAt303=0;SRWLOCK g_handsLock303=SRWLOCK_INIT;unsigned g_roleEpoch303=0;int g_nativeHoldingSide303=0;float g_handPalm303[2][3]={};unsigned g_handPalmEpoch303[2]={~0u,~0u};
void LoadHands303(){}void PublishHands303(bool){}
namespace fc3pad{bool NativeMenuOpen(){return fixtureMenu;}}
namespace fc3xr{unsigned ReferenceGeneration286(){return 0;}}
namespace fc3hands303{Transfer307 TransferContact307(){return {};}Snapshot fixtureSnapshot;Snapshot Read(){return fixtureSnapshot;}}
#include "../src/xr_selector290.inl"
void Controls(fc3pad::Controllers c){g_selectAllowed290=true;g_selectAllowedAt290=GetTickCount64();SelectorControls290(c);}
std::vector<unsigned> Pixels291(ID3D11Texture2D* texture,int eye){D3D11_TEXTURE2D_DESC td={};texture->GetDesc(&td);td.ArraySize=1;td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* staging=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&staging)),"pixel readback staging");ID3D11DeviceContext* c=nullptr;device->GetImmediateContext(&c);c->CopySubresourceRegion(staging,0,0,0,0,texture,eye,nullptr);D3D11_MAPPED_SUBRESOURCE map={};Check(SUCCEEDED(c->Map(staging,0,D3D11_MAP_READ,0,&map)),"read actual rendered eye pixels");std::vector<unsigned> p(640*640);for(int y=0;y<640;++y)memcpy(p.data()+y*640,static_cast<char*>(map.pData)+y*map.RowPitch,640*4);c->Unmap(staging,0);c->Release();staging->Release();return p;}
void Save291(const std::vector<unsigned>& p){BITMAPFILEHEADER h={};h.bfType=0x4d42;h.bfOffBits=54;h.bfSize=54+DWORD(p.size()*4);BITMAPINFOHEADER b={};b.biSize=40;b.biWidth=640;b.biHeight=-640;b.biPlanes=1;b.biBitCount=32;std::vector<unsigned> bgra=p;for(auto& v:bgra)v=(v&0xff00ff00)|((v&0xff)<<16)|((v>>16)&0xff);FILE* f=nullptr;fopen_s(&f,"evidence/native-materials292-fixture-left.bmp","wb");if(f){fwrite(&h,14,1,f);fwrite(&b,40,1,f);fwrite(bgra.data(),4,bgra.size(),f);fclose(f);}}

static const GUID codeGuid292={0x292929,0,0,{0,0,0,0,0,0,0,1}};
void MakeOwnedVisual292(ID3D11DeviceContext* c,int slot){
 const char* code="struct I{float3 p:POSITION;float2 uv:TEXCOORD0;};struct O{float4 p:SV_POSITION;float2 uv:TEXCOORD0;};O vs(I i){O o;o.p=float4(i.p,1);o.uv=i.uv;return o;}Texture2D Skin:register(t3);SamplerState SS:register(s2);cbuffer Material:register(b0){float4 Tint;};struct P{float4 colour:SV_TARGET0;float4 normal:SV_TARGET1;};P ps(O i){P o;o.colour=Skin.Sample(SS,i.uv)*Tint;o.colour.a=0;o.normal=float4(0,0,1,1);return o;}";
 ID3DBlob *vc=nullptr,*pc=nullptr,*error=nullptr;Check(SUCCEEDED(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"vs","vs_4_0",0,0,&vc,&error)),"compile native-style vertex fixture");fc3visual292::Drop(error);Check(SUCCEEDED(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"ps","ps_4_0",0,0,&pc,&error)),"compile actual textured multi-target material fixture");fc3visual292::Drop(error);
 ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;device->CreateVertexShader(vc->GetBufferPointer(),vc->GetBufferSize(),nullptr,&vs);device->CreatePixelShader(pc->GetBufferPointer(),pc->GetBufferSize(),nullptr,&ps);vs->SetPrivateData(codeGuid292,UINT(vc->GetBufferSize()),vc->GetBufferPointer());ps->SetPrivateData(codeGuid292,UINT(pc->GetBufferSize()),pc->GetBufferPointer());
 D3D11_INPUT_ELEMENT_DESC e[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};ID3D11InputLayout* layout=nullptr;device->CreateInputLayout(e,2,vc->GetBufferPointer(),vc->GetBufferSize(),&layout);
 const float vertices[]={-.08f,-.1f,0,0,1, -.08f,.16f,0,0,0, .08f,.16f,0,1,0, .08f,-.1f,0,1,1, .08f,.06f,0,0,1, .08f,.14f,0,0,0, .18f,.14f,0,1,0, .18f,.06f,0,1,1};const USHORT ids[]={0,1,2,0,2,3,4,5,6,4,6,7};
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(vertices);bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA init={vertices,0,0};ID3D11Buffer *vb=nullptr,*ib=nullptr,*cb=nullptr;device->CreateBuffer(&bd,&init,&vb);bd.ByteWidth=sizeof(ids);bd.BindFlags=D3D11_BIND_INDEX_BUFFER;init.pSysMem=ids;device->CreateBuffer(&bd,&init,&ib);bd.ByteWidth=16;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;float tint[]={1,1,1,1};init.pSysMem=tint;device->CreateBuffer(&bd,&init,&cb);
 const UINT colours[]={0xff2020e0,0xff20e020,0xffe02020,0xff20e0e0};const UINT bright=colours[slot];UINT pixels[]={bright,bright&0xff7f7f7f,bright&0xff7f7f7f,bright};D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=2;td.ArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;init={pixels,8,0};ID3D11Texture2D* texture=nullptr;ID3D11ShaderResourceView* srv=nullptr;device->CreateTexture2D(&td,&init,&texture);device->CreateShaderResourceView(texture,nullptr,&srv);
 D3D11_SAMPLER_DESC sd={};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;ID3D11SamplerState* sampler=nullptr;device->CreateSamplerState(&sd,&sampler);
 UINT stride=20,zero=0;c->IASetInputLayout(layout);c->IASetVertexBuffers(0,1,&vb,&stride,&zero);c->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);c->VSSetShader(vs,nullptr,0);c->PSSetShader(ps,nullptr,0);c->PSSetConstantBuffers(0,1,&cb);c->PSSetShaderResources(3,1,&srv);c->PSSetSamplers(2,1,&sampler);
 auto& v=fc3visual292::fixtureVisual[slot];const auto& inv=fc3select290::fixtureInventory;v.owner=inv.owner;v.id=inv.weapons[slot].id;v.entity=inv.weapons[slot].entity;
 for(int k=0;k<3;++k){
  if(k==2){D3D11_TEXTURE2D_DESC handDesc=td;handDesc.Width=handDesc.Height=1;UINT skin=0xffa080f0;D3D11_SUBRESOURCE_DATA handInit={&skin,4,0};ID3D11Texture2D* handTexture=nullptr;ID3D11ShaderResourceView* handView=nullptr;device->CreateTexture2D(&handDesc,&handInit,&handTexture);device->CreateShaderResourceView(handTexture,nullptr,&handView);c->PSSetShaderResources(3,1,&handView);fc3visual292::Drop(handView);fc3visual292::Drop(handTexture);}
  auto part=std::make_shared<fc3visual292::Part>();std::string why;const bool ok=fc3visual292::Capture(device,c,6,(k==1?6:0),0,codeGuid292,*part,why);if(!ok)printf("%s\n",why.c_str());Check(ok,k?"native attachment captured independently under the same owned identity":"native body/material captured under exact owned identity");for(int i=0;i<16;++i)part->clipToGrip[i]=i%5==0?1.f:0.f;if(k==2){part->hand=true;part->clipToGrip[11]=.02f;Check(fc3visual292::CropHand(device,c,*part),"native grip-hand crop retains actual hand triangles");}else Check(fc3visual292::Geometry(device,c,*part,v.collider.triangles),"collision triangles come from the very same captured GPU geometry");v.parts.push_back(part);}
 v.collider.Build();v.ready=true;float changed[]={0,0,0,0};c->UpdateSubresource(cb,0,nullptr,changed,0,0);
 fc3visual292::Drop(sampler);fc3visual292::Drop(srv);fc3visual292::Drop(texture);fc3visual292::Drop(cb);fc3visual292::Drop(ib);fc3visual292::Drop(vb);fc3visual292::Drop(layout);fc3visual292::Drop(ps);fc3visual292::Drop(vs);fc3visual292::Drop(vc);fc3visual292::Drop(pc);
}
void Wrist(XrVector3f p){float native[16]={1,0,0,p.x,0,1,0,p.y,0,0,-1,-p.z,0,0,0,1};PublishSelectorWrist293(native);hand.position=p;}
#include "live_wrist293_fixture.h"
int main(){
 fc3hands303::fixtureSnapshot.enabled=true;fc3hands303::fixtureSnapshot.empty=true;
 g_holstersLoaded297=true;g_holsters297.mode=fc3holster297::Front;
 {float endpoints[24]={0,-.04f,0,0,.04f,0, .5f,-.04f,0,.5f,.04f,0, 0,-.35f,0,0,-.27f,0, .5f,-.35f,0,.5f,-.27f,0};bool occupied[4]={true,true,true,true};float p[3]={.09f,0,0};
 Check(fc3grab295::Handle(endpoints,p,occupied)==0,"10cm handle capsule includes palms outside old5.5cm sphere");p[0]=0;p[1]=.13f;Check(fc3grab295::Handle(endpoints,p,occupied)==0,"capsule extends along the handle with rounded endpoints");p[0]=.115f;p[1]=0;Check(fc3grab295::Handle(endpoints,p,occupied)==-1&&fc3grab295::Handle(endpoints,p,occupied,0)==0,"12cm exit hysteresis prevents edge flicker without broadening initial selection");p[0]=NAN;Check(fc3grab295::Handle(endpoints,p,occupied)==-1,"invalid palm cannot select");}

 for(const auto& sample:liveWrist293){
  float native[16]={};native[15]=1;for(int r=0;r<3;++r)for(int c=0;c<3;++c)native[r*4+c]=sample[r*3+c];native[3]=.25f;native[7]=1.2f;native[11]=.4f;
  XrPosef rejected={},pose={};Check(!fc3xr::BodyToReferencePose(native,1,0,{0,0,0},rejected),"292 rejects the actual mirrored native wrist basis from failed PID151792");
  PublishSelectorWrist293(native);Check(SelectorWrist292(pose),"293 production wrist publisher accepts the recorded native basis");
  const XrVector3f local={.0871f,.0409f,-.1103f};const auto palm=testnotes::Add(pose.position,testnotes::Rotate(pose.orientation,local));float expected[3]={};for(int r=0;r<3;++r)expected[r]=native[r*4]*local.x+native[r*4+1]*local.y+native[r*4+2]*local.z+native[r*4+3];
  Check(std::fabs(palm.x-expected[0])<1e-4f&&std::fabs(palm.y-expected[1])<1e-4f&&std::fabs(palm.z+expected[2])<1e-4f,"native asymmetric palm keeps the same physical point without a second reflection");
  float round[16];Check(fc3wrist293::ChangeLocalFrame(g_selectorWristBody292,round)&&std::equal(round,round+16,native),"contact target returns exactly to native skin frame");
 }
 fc3contact292::Collider plane;for(int y=0;y<10;++y)for(int x=0;x<10;++x){const float a=float(x),b=float(y);plane.triangles.push_back({{a,b,0},{a+1,b,0},{a,b+1,0}});plane.triangles.push_back({{a+1,b,0},{a+1,b+1,0},{a,b+1,0}});}plane.Build();fc3contact292::P resolved,nearest;float squared=0;
 Check(plane.Sweep({3.2f,4.2f,.5f},{3.2f,4.2f,-.5f},.025f,resolved)&&resolved.z>=.025f,"BVH sweep blocks a fast palm crossing through actual triangle surfaces");
 Check(!plane.Nearest({30,30,0},.025f,nearest,squared)&&!plane.Nearest({NAN,0,0},.025f,nearest,squared),"BVH rejects distant and invalid contacts");
 g_referenceYaw=.7f;g_referencePosition={2,3,4};fc3visual292::fixtureVisual[0].ready=true;XrPosef target={{0,std::sin(.2f),0,std::cos(.2f)},{2.2f,3.4f,3.5f}};BodyContact292(target,0,1);XrPosef round={};
 Check(fc3xr::BodyToReferencePose(fc3visual292::contactBody,1,g_referenceYaw,g_referencePosition,round)&&std::fabs(round.position.x-target.position.x)<1e-5f&&std::fabs(round.position.y-target.position.y)<1e-5f&&std::fabs(round.position.z-target.position.z)<1e-5f&&std::fabs(round.orientation.y-target.orientation.y)<1e-5f,"contact pose round-trips body/reference axes at nonzero yaw without changing tracking");g_referenceYaw=0;g_referencePosition={0,0,0};fc3visual292::fixtureVisual[0].ready=false;
 auto& inv=fc3select290::fixtureInventory;inv.valid=true;inv.owner=37;inv.equipped=202;for(int i=0;i<4;++i){inv.weapons[i].id=100+i;inv.weapons[i].entity=200+i;}
 for(int i=0;i<2;++i){g_views[i].pose.orientation.w=1;g_views[i].pose.position={i?.032f:-.032f,1.6f,0};g_views[i].fov={-.8f,.8f,.8f,-.8f};}
 ID3D11DeviceContext* immediate=nullptr;Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&immediate)),"WARP; no native engine or XR runtime");for(int i=0;i<4;++i)MakeOwnedVisual292(immediate,i);
 // Include a compressed, mipmapped cube and a texture array in the archive.
 for(int kind=0;kind<2;++kind){D3D11_TEXTURE2D_DESC extra={};extra.Width=extra.Height=4;extra.MipLevels=3;extra.ArraySize=kind?2:6;extra.Format=kind?DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:DXGI_FORMAT_BC1_UNORM;extra.SampleDesc.Count=1;extra.BindFlags=D3D11_BIND_SHADER_RESOURCE;extra.MiscFlags=kind?0:D3D11_RESOURCE_MISC_TEXTURECUBE;UINT pixels[16];for(auto& pixel:pixels)pixel=0xff204080;std::vector<D3D11_SUBRESOURCE_DATA> init;for(UINT a=0;a<extra.ArraySize;++a)for(UINT m=0;m<3;++m)init.push_back({pixels,kind?(4u>>m)*4u:8u,0});ID3D11Texture2D* tex=nullptr;ID3D11ShaderResourceView* srv=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&extra,init.data(),&tex))&&SUCCEEDED(device->CreateShaderResourceView(tex,nullptr,&srv)),"compressed cube/mip and sRGB array cache fixtures created");fc3visual292::fixtureVisual[0].parts[0]->textures[14+kind]=srv;tex->Release();}
 std::vector<BYTE> diskArchives296[4];for(int i=0;i<4;++i){fc3cache296::Writer archive;Check(fc3cache296::Encode(device,immediate,fc3visual292::fixtureVisual[i],archive),"native geometry, shaders, UVs, material constants and textures encode without substitution");diskArchives296[i]=archive.bytes;wchar_t path[100];swprintf_s(path,L"evidence/cache296-test-%d.fcv296",i);fc3cache296::DiskWriter writer;Check(writer.Start(path,std::move(archive.bytes)),"cache file writer started");writer.Wait();Check(writer.TakeResult()==1,"private cache file committed atomically");}
 std::vector<unsigned> referencePixels296;
 D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=640;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;ID3D11Texture2D* source=nullptr;ID3D11RenderTargetView* original=nullptr;device->CreateTexture2D(&td,nullptr,&source);device->CreateRenderTargetView(source,nullptr,&original);immediate->OMSetRenderTargets(1,&original,nullptr);
 for(int slot=0;slot<4;++slot){fc3pad::Controllers c={};c.active=true;Wrist({0,1.3f,0});Controls(c);c.a=true;c.rightGrip=1;Controls(c);Check(g_selectGesture290.open&&!g_pick292.requested,"right grip+A opens without equipping");c.a=false;c.rightGrip=0;Wrist({0,1.3f,0});Controls(c);Check(g_warm292.done&&g_selectGesture290.open,"already cached owned visuals need no weapon cycling; A released");
  XrCompositionLayerProjection layer={};XrCompositionLayerProjectionView views[2]={};Check(SelectorLayer291(layer,views,source),"shipping native material renderer produces stereo layer");
  if(GetEnvironmentVariableW(L"FC3_CACHE359_ONLY",nullptr,0)){
   fc3readback359::Batch snapshot;const auto& visual=fc3visual292::fixtureVisual[0];
   for(const auto& part:visual.parts){Check(snapshot.Add(device,immediate,part->vertices),"359 vertex snapshot");for(auto* cb:part->constants)if(cb)Check(snapshot.Add(device,immediate,cb),"359 constant snapshot");for(auto* view:part->textures)if(view){ID3D11Resource* resource=nullptr;view->GetResource(&resource);Check(snapshot.Add(device,immediate,resource),"359 texture/mip/array snapshot");resource->Release();}}
   Check(snapshot.End(device,immediate),"359 asynchronous staging fence");immediate->Flush();int ready=0;const auto deadline=GetTickCount64()+2000;while(!(ready=snapshot.Ready(immediate))&&GetTickCount64()<deadline)Sleep(1);Check(ready==1,"359 GPU fence completes without a production wait or flush");
   D3D11_BUFFER_DESC mutated359={};visual.parts[0]->vertices->GetDesc(&mutated359);std::vector<BYTE> zeros359(mutated359.ByteWidth);immediate->UpdateSubresource(visual.parts[0]->vertices,0,nullptr,zeros359.data(),0,0);
   fc3cache296::Writer encoded;
   {fc3readback359::Scope scope(snapshot);Check(fc3readback359::MapFlags()==D3D11_MAP_FLAG_DO_NOT_WAIT,"359 gameplay serializer maps are explicitly nonblocking");Check(fc3cache296::Encode(device,immediate,visual,encoded),"359 completed snapshot serializes actual native resources");}
   Check(encoded.bytes==diskArchives296[0],"359 asynchronous snapshot preserves exact original textures/material/vertex archive bytes");
   fc3cache296::Writer changed359;Check(fc3cache296::Encode(device,immediate,visual,changed359)&&changed359.bytes!=encoded.bytes,"359 frozen cache snapshot survives subsequent live source-buffer changes");
   fc3visual292::WeaponVisual decoded;Check(fc3cache296::Decode(device,encoded.bytes,inv.weapons[0].id,decoded)&&decoded.ready,"359 serialized snapshot reloads through unmodified private cache reader");
   fc3readback359::Batch missing;{fc3readback359::Scope scope(missing);fc3cache296::Writer refused;Check(!fc3cache296::Encode(device,immediate,visual,refused),"359 missing snapshot cannot fall back to a synchronous gameplay copy");}
   snapshot.Clear();Check(snapshot.Empty()&&fc3readback359::Current()==nullptr,"359 snapshot resources and scoped reader release cleanly");
   puts("PASS:359 real GPU fence, nonwaiting texture/buffer maps, byte-identical material archives, existing cache decoder and missing-resource refusal");return 0;
  }
  if(GetEnvironmentVariableW(L"FC3_RECOVERY358_ONLY",nullptr,0)){
   g_holsters297.mode=static_cast<fc3holster297::Mode>(1);g_bodyVisible297=true;fail358=true;
   Check(!SelectorLayer291(layer,views,source)&&g_bodyRenderFailed297,"358 actual XR wait failure latches draw denial");
   fail358=false;g_bodyVisible297=true;
   Check(SelectorLayer291(layer,views,source),"358 replacement native-material overlay successfully renders");
   Check(!g_bodyRenderFailed297,"358 successful rendered holsters must clear the previous draw-denial latch");
   puts("PASS:358 real renderer failure/recreation/success recovery, native material resources and two-eye layer");return 0;
  }

  ID3D11RenderTargetView* restored=nullptr;immediate->OMGetRenderTargets(1,&restored,nullptr);Check(restored==original,"game render targets restored");fc3visual292::Drop(restored);
  if(slot==0){auto left=Pixels291(images[(acquired-1)%2],0),right=Pixels291(images[(acquired-1)%2],1);size_t opaque=0,different=0,red=0,green=0,blue=0,yellow=0,dark=0;for(size_t i=0;i<left.size();++i){UINT p=left[i];if(p>>24){++opaque;UINT r=p&255,g=(p>>8)&255,b=(p>>16)&255;if(r>180&&g<60&&b<60)++red;if(g>180&&r<60&&b<60)++green;if(b>180&&r<60&&g<60)++blue;if(r>180&&g>180&&b<60)++yellow;if((r>80&&r<150)||(g>80&&g<150)||(b>80&&b<150))++dark;}if(p!=right[i])++different;}
   Check(opaque>1000&&opaque<left.size()/2&&different>500,"native materials have opaque silhouettes, transparent surroundings and stereo disparity despite native alpha0");Check(red>100&&green>100&&blue>100&&yellow>100&&dark>100,"all four exact material instances retain different skins and textured UV detail");Save291(left);referencePixels296=left;
   fc3hands303::fixtureSnapshot.empty=false;fc3hands303::fixtureSnapshot.entity=g_selectShown290.weapons[2].entity;
   Check(SelectorLayer291(layer,views,source),"303 held-weapon visibility filter renders");auto heldPixels=Pixels291(images[(acquired-1)%2],0);int heldBlue=0,remainingRed=0;for(UINT px:heldPixels){const UINT rr=px&255,gg=(px>>8)&255,bb=(px>>16)&255;if(bb>180&&rr<60&&gg<60)++heldBlue;if(rr>180&&gg<60&&bb<60)++remainingRed;}
   Check(heldBlue==0&&remainingRed>100,"303 actual GPU pixels hide only the held weapon from the rack");fc3hands303::fixtureSnapshot.empty=true;
   Check(SelectorLayer291(layer,views,source),"303 stowed weapon returns to visible holster");auto stowedPixels=Pixels291(images[(acquired-1)%2],0);int stowedBlue=0;for(UINT px:stowedPixels)if(((px>>16)&255)>180&&(px&255)<60&&((px>>8)&255)<60)++stowedBlue;Check(stowedBlue>100,"303 empty-hands GPU pixels restore the stowed exact weapon appearance");

   // Protruding attachment occupies pixels beyond the body, from its own native draw.
   int attachments=0;for(int i=0;i<4;++i){auto pose=RackHandle292(i);const float wx=pose.position.x+.14f,wy=pose.position.y+.10f,wz=pose.position.z;const int x=int((wx+.032f)/(-wz*std::tan(.8f))*320+320),y=int(320-(wy-1.6f)/(-wz*std::tan(.8f))*320);for(int yy=y-3;yy<=y+3;++yy)for(int xx=x-3;xx<=x+3;++xx)if(xx>=0&&xx<640&&yy>=0&&yy<640&&(left[yy*640+xx]>>24))++attachments;}Check(attachments>100,"separately captured native attachments appear in the rendered output");
  }
  const auto h=RackHandle292(slot);
  if(slot==0){const XrVector3f wanted={h.position.x+.15f-.0871f,h.position.y+.10f-.0409f,h.position.z+.005f+.1103f};Wrist(wanted);Controls(c);Check(!fc3visual292::contact,"preview gun geometry cannot push back or pin the hand while approaching the handle");}
  Wrist(testnotes::Add(h.position,{.20f,0,0}));Controls(c);Check(g_selectGesture290.hover==-1,"distant barrel/body contact cannot select a weapon");
  Wrist(testnotes::Add(h.position,{.085f,0,0}));Controls(c);Check(g_selectGesture290.hover==slot&&!fc3visual292::contact,"enlarged capsule accepts an off-centre palm without pinning it");
  if(slot==0){Check(SelectorLayer291(layer,views,source),"hover highlight renders");auto pixels=Pixels291(images[(acquired-1)%2],0);int cyan=0;for(UINT px:pixels)if((px&255)<10&&((px>>8)&255)>170&&((px>>8)&255)<210&&((px>>16)&255)>245)++cyan;Check(cyan>50,"cyan silhouette identifies the handle-ready weapon");}
  const int before=fc3select290::fixtureCalls;const auto stationary=hand.position;fc3select290::fixtureDelay=true;c.rightGrip=1;Controls(c);
  Check(g_pick292.slot==slot&&g_pick292.requested&&fc3select290::fixtureCalls==before+1&&fc3select290::fixtureGrab==slot,"grip queues exact weapon immediately, with zero controller motion or pull delay");
  if(slot==0){Check(SelectorLayer291(layer,views,source),"accepted grip renders green while native readback is pending");auto pixels=Pixels291(images[(acquired-1)%2],0);int green=0;for(UINT px:pixels)if((px&255)>15&&(px&255)<40&&((px>>8)&255)>245&&((px>>16)&255)>50&&((px>>16)&255)<80)++green;Check(green>50,"green silhouette confirms immediate equip request");}
  if(slot==0||slot==2){c.leftGrip=1;Controls(c);Check(g_selectGesture290.open&&g_pick292.slot==slot,"left support grip cannot dismiss an accepted long-weapon selection");}
  c.rightGrip=0;Controls(c);Check(g_pick292.requested&&fc3select290::fixtureCalls==before+1,"release after accepted grip cannot cancel or duplicate the queued equip");
  inv.equipped=inv.weapons[slot].entity;fc3select290::fixtureResult=1;fc3select290::fixtureDelay=false;Controls(c);
  Check(!g_selectGesture290.open&&hand.position.x==stationary.x&&hand.position.y==stationary.y&&hand.position.z==stationary.z,"exact native readback closes rack without any pull motion");
 }
 // Replay the ready-rack transition with a real rejected-292 basis; never an identity wrist.
 {fc3pad::Controllers c={};c.active=true;Controls(c);c.a=true;c.rightGrip=1;Controls(c);c.a=false;c.rightGrip=0;
  for(const auto& sample:liveWrist293){float native[16]={};native[15]=1;for(int r=0;r<3;++r)for(int col=0;col<3;++col)native[r*4+col]=sample[r*3+col];native[7]=1.3f;PublishSelectorWrist293(native);Controls(c);Controls(c);Check(g_warm292.done&&g_selectGesture290.open&&g_rackBasisReady292,"ready rack remains open across recorded native wrist samples after A/grip release");}
  g_selectorWristAt292=GetTickCount64()-121;Controls(c);Check(g_selectGesture290.open&&!g_selectGesture290.ready&&!fc3visual292::contact,"stale arm sample retains visible rack while preventing a grab");
  XrCompositionLayerProjection layer={};XrCompositionLayerProjectionView views[2]={};Check(SelectorLayer291(layer,views,source),"rack remains renderable through a missed native arm draw");
  PublishSelectorWrist293(nullptr);Controls(c);Check(g_selectGesture290.open&&g_selectGesture290.hover<0,"absent rendered wrist cannot close or select the visible rack");
  tracked=false;Controls(c);Check(!g_selectGesture290.open,"actual XR hand tracking loss still cancels");tracked=true;}
 // A cold appearance prepares the missing owned instance and restores the original.
 fc3pad::Controllers c={};c.active=true;inv.equipped=202;fc3visual292::fixtureVisual[0].ready=false;Wrist({0,1.3f,0});Controls(c);const int before=fc3select290::fixtureCalls;c.a=true;c.rightGrip=1;Controls(c);Check(inv.equipped==200&&!g_warm292.done,"cold visual preparation requests only a missing owned weapon");c.a=false;c.rightGrip=0;Controls(c);fc3visual292::fixtureVisual[0].ready=true;Controls(c);Controls(c);Controls(c);Check(g_warm292.done&&inv.equipped==202&&fc3select290::fixtureCalls==before+2,"visual preparation restores the exact original weapon before grabbing");tracked=false;Controls(c);Check(!g_selectGesture290.open&&!fc3visual292::contact,"tracking loss cancels selector and virtual hand constraint");tracked=true;inv.equipped=202;fc3visual292::fixtureVisual[0].ready=false;Wrist({0,1.3f,0});c={};c.active=true;Controls(c);c.a=true;c.rightGrip=1;Controls(c);Check(inv.equipped==200,"cancellation fixture is preparing a different owned weapon");c.a=false;c.rightGrip=0;Controls(c);c.a=true;Controls(c);c.a=false;Controls(c);Check(inv.equipped==202&&!g_selectGesture290.open,"A during preparation restores the original equipped identity before closing");
 inv.equipped=202;fc3visual292::fixtureVisual[0].ready=false;fc3select290::fixtureDelay=true;c={};c.active=true;Wrist({0,1.3f,0});Controls(c);const int delayedBefore=fc3select290::fixtureCalls;c.a=true;c.rightGrip=1;Controls(c);Check(inv.equipped==202&&g_warm292.inflight,"native appearance switch can remain queued before equipped readback");c.a=false;c.rightGrip=0;Controls(c);c.a=true;Controls(c);Check(g_selectGesture290.open&&g_warm292.cancel&&fc3select290::fixtureCalls==delayedBefore+1,"cancel waits for in-flight switch even while original is still equipped");inv.equipped=200;fc3select290::fixtureResult=1;fc3select290::fixtureDelay=false;c.a=false;Controls(c);Controls(c);Check(inv.equipped==202&&!g_selectGesture290.open&&fc3select290::fixtureCalls==delayedBefore+2,"late switch is drained and original restored before cancellation closes");
 // Passive cache chooses only the already equipped owned instance; never queues equip.
 fc3select290::fixtureDelay=false;const int passiveBefore=fc3select290::fixtureCalls;inv.equipped=202;bool ready[4]={true,true,false,true};
 Check(fc3passive295::EquippedMissing(inv,ready)==2,"background capture selects only the current missing owned appearance");ready[2]=true;Check(fc3passive295::EquippedMissing(inv,ready)==-1,"fully cached equipment skips passive capture");inv.equipped=999;Check(fc3passive295::EquippedMissing(inv,ready)==-1,"unowned equipped entity cannot be passively captured");inv.equipped=202;
 g_selectGesture290.Close();c={};c.active=true;Wrist({0,1.3f,0});Controls(c);Check(fc3select290::fixtureCalls==passiveBefore&&!fc3pad::active290&&fc3select290::lease,"passive observation refreshes read-only inventory without switching weapons or owning game input");
 Check(acquired==released,"all acquired stereo images released");
 for(auto& visual:fc3visual292::fixtureVisual)visual.ready=true;
 // Four body positions must select the corresponding current inventory entities.
 for(int slot=0;slot<4;++slot){g_holsters297={};g_selectGesture290.Close();g_pick292.Cancel();fc3pad::Controllers body={};body.active=true;Wrist({0,1.3f,0});Controls(body);Check(!g_selectGesture290.open&&!fc3pad::active290&&g_bodyVisible297,"idle body holsters render without owning firing or reload input and without A");const auto pose=RackHandle292(slot);const auto palm=Palm292(pose);const auto offset=fc3holster297::Centre(g_holsters297,slot);Check(std::fabs(palm.x-offset.x)<1e-5f&&std::fabs(palm.y-(1.6f+offset.y))<1e-5f&&std::fabs(palm.z-offset.z)<1e-5f,"four native handle centres lie at left/right hip and lower-back slots");
  // An arbitrary actual wrist orientation must still hit the generous body target.
  const auto wristPosition=testnotes::Sub(palm,{.0871f,.0409f,-.1103f});Wrist(wristPosition);Controls(body);Check(g_selectGesture290.hover==slot,"body holster trigger reaches the intended exact slot");const int before=fc3select290::fixtureCalls;body.rightGrip=1;Controls(body);Check(fc3select290::fixtureCalls==before+1&&fc3select290::fixtureGrab==slot&&!g_selectGesture290.open,"four-body grip equips exact current inventory slot with no pull");
 }
 // A normal grip elsewhere stays available to existing reload/support behavior.
 {fc3pad::Controllers input={};input.active=true;Wrist({0,1.3f,0});Controls(input);const int before=fc3select290::fixtureCalls;input.rightGrip=1;Controls(input);Check(!g_selectGesture290.open&&!fc3pad::active290&&fc3select290::fixtureCalls==before,"grip outside body handles does not steal weapon or reload controls");auto p=Palm292(RackHandle292(0));Wrist(testnotes::Sub(p,{.0871f,.0409f,-.1103f}));Controls(input);Check(fc3select290::fixtureCalls==before,"an already-held grip entering a holster cannot accidentally equip");input.rightGrip=0;Controls(input);}
 // Missing appearances do not start the disliked native preparation cycle.
 {for(auto& visual:fc3visual292::fixtureVisual)visual.ready=false;fc3pad::Controllers input={};input.active=true;Wrist({0,1.3f,0});const int before=fc3select290::fixtureCalls;Controls(input);Check(!g_bodyVisible297&&!g_selectGesture290.open&&!fc3pad::active290&&fc3select290::fixtureCalls==before&&fc3visual292::passiveSlot295>=0,"uncached body loadout captures only normally equipped weapon without forced setup");for(auto& visual:fc3visual292::fixtureVisual)visual.ready=true;}
 // A lowered head translates body slots without pitching them into the floor.
 {g_holsters297={};g_selectGesture290.Close();fc3pad::Controllers body={};body.active=true;Wrist({0,1.3f,0});Controls(body);const auto before=Palm292(RackHandle292(0));for(auto& view:g_views){view.pose.position.x+=.2f;view.pose.position.y-=.3f;view.pose.orientation={std::sin(-.65f),0,0,std::cos(-.65f)};}Controls(body);const auto after=Palm292(RackHandle292(0));Check(std::fabs(after.x-before.x-.2f)<1e-5f&&std::fabs(after.y-before.y+.3f)<1e-5f&&std::fabs(after.z-before.z)<1e-5f,"body handles follow position/crouch while head pitch leaves the layout level");tracked=false;Controls(body);tracked=true;for(int i=0;i<2;++i){g_views[i].pose.position={i?.032f:-.032f,1.6f,0};g_views[i].pose.orientation={0,0,0,1};}}
 // Single mode previews inventory slots while held; it queues native equip only on release.
 {g_holsters297.mode=fc3holster297::Single;inv.equipped=0;g_singleSlot297=0;fc3pad::Controllers single={};single.active=true;Wrist({0,1.3f,0});Controls(single);Check(g_singleSlot297==0&&g_bodyReady297,"single holster starts at a visible owned weapon when native hands are empty");auto p=Palm292(RackHandle292(0));Wrist(testnotes::Sub(p,{.0871f,.0409f,-.1103f}));Controls(single);const int before=fc3select290::fixtureCalls;single.rightGrip=1;Controls(single);Check(g_singleHeld297&&!g_pick292.requested&&fc3select290::fixtureCalls==before,"single grip attaches preview without changing equipped weapon");
  const auto rawStep=[&](float x,float y){auto input=single;input.rightX=x;input.rightY=y;g_selectAllowed290=true;g_selectAllowedAt290=GetTickCount64();SelectorControls290(input);return input;};
  auto consumed=rawStep(.9f,0);Check(g_singleSlot297==1&&consumed.rightX==0&&consumed.rightY==0,"one right flick advances one slot and consumes camera input");rawStep(.9f,0);Check(g_singleSlot297==1,"holding a deflected stick cannot repeat through weapons");rawStep(0,0);rawStep(.9f,0);Check(g_singleSlot297==2,"neutral rearms the next flick");rawStep(0,0);rawStep(.9f,0);Check(g_singleSlot297==3,"single mode reaches the fourth inventory weapon");rawStep(0,0);rawStep(.9f,0);Check(g_singleSlot297==0,"single mode wraps fourth slot to first");rawStep(0,0);rawStep(-.9f,0);Check(g_singleSlot297==3&&fc3select290::fixtureCalls==before,"left flick goes backward without native intermediate switches");
  XrCompositionLayerProjection layer={};XrCompositionLayerProjectionView views[2]={};Check(SelectorLayer291(layer,views,source),"single selected native appearance and grip hand render while cycling");
  single.rightGrip=0;auto release=rawStep(-.9f,0);Check(fc3select290::fixtureCalls==before+1&&fc3select290::fixtureGrab==3&&!g_selectGesture290.open&&release.rightX==0,"release equips only the final preview and consumes the held stick");
  auto held=rawStep(-.9f,0);Check(held.rightX==0,"deflected selection stick stays blocked after equip");rawStep(0,0);auto turn=rawStep(.9f,0);Check(turn.rightX==.9f,"camera stick resumes after observed neutral");
 }
 // Cancelling a single-holster preview must never equip a different weapon.
 {g_holsters297.mode=fc3holster297::Single;fc3pad::Controllers single={};single.active=true;Wrist({0,1.3f,0});Controls(single);auto p=Palm292(RackHandle292(g_singleSlot297));Wrist(testnotes::Sub(p,{.0871f,.0409f,-.1103f}));Controls(single);single.rightGrip=1;Controls(single);const int before=fc3select290::fixtureCalls;single.a=true;Controls(single);Check(!g_selectGesture290.open&&!g_singleHeld297&&fc3select290::fixtureCalls==before,"A cancels single preview without native equip");}
 // Safety exits may not turn a held preview into an unintended native equip.
 for(int reason=0;reason<4;++reason){g_holsters297.mode=fc3holster297::Single;fc3pad::Controllers input={};input.active=true;Wrist({0,1.3f,0});Controls(input);auto palm=Palm292(RackHandle292(g_singleSlot297));Wrist(testnotes::Sub(palm,{.0871f,.0409f,-.1103f}));Controls(input);input.rightGrip=1;Controls(input);Check(g_singleHeld297,"safety fixture begins direct-grip single selection without A");const int before=fc3select290::fixtureCalls;
  if(reason==0)g_selectorWristAt292=GetTickCount64()-121;
  if(reason==1)fc3pad::fixtureMenu=true;
  if(reason==2)inv.weapons[1].entity+=1000;
  if(reason==3)g_bodyLast297=GetTickCount64()-121;
  Controls(input);Check(!g_selectGesture290.open&&!g_singleHeld297&&fc3select290::fixtureCalls==before,"stale wrist, menu, loadout change and hitch cancel without native equip");
  fc3pad::fixtureMenu=false;if(reason==2)inv.weapons[1].entity-=1000;input.rightGrip=0;Wrist({0,1.3f,0});Controls(input);
 }
 {g_bodyRenderFailed297=true;fc3pad::Controllers input={};input.active=true;Wrist({0,1.3f,0});Controls(input);const int before=fc3select290::fixtureCalls;auto p=Palm292(RackHandle292(g_singleSlot297));Wrist(testnotes::Sub(p,{.0871f,.0409f,-.1103f}));input.rightGrip=1;Controls(input);Check(!g_selectGesture290.open&&!g_bodyVisible297&&fc3select290::fixtureCalls==before,"failed body renderer cannot accept invisible grips until settings reset or restart");g_bodyVisible297=true;XrCompositionLayerProjection recovery358={};XrCompositionLayerProjectionView views358[2]={};Check(SelectorLayer291(recovery358,views358,source),"358 recovered body overlay actually renders");Check(!g_bodyRenderFailed297,"358 successful rendered holsters must clear the previous draw-denial latch");g_bodyRenderFailed297=false;}
 g_holsters297.mode=fc3holster297::Front;
 //304 visual targets are tested through actual shipping D3D rendering.
 {g_selectGesture290.Close();g_pick292.Cancel();g_bodyVisible297=false;g_warm292.done=true;g_selectShown290=inv;g_selectPose290={{0,0,0,1},{0,1.6f,0}};fc3holster297::Seated(g_holsters297);g_holsters297.mode=fc3holster297::Single;g_singleSlot297=1;g_zonesVisible304=false;
 auto& held=fc3hands303::fixtureSnapshot;held.enabled=held.engaged=held.active=true;held.empty=false;held.slot=1;held.entity=inv.weapons[1].entity;g_stowReady304=false;g_holsters297.stowOutline=true;
 XrCompositionLayerProjection target={};XrCompositionLayerProjectionView eyes[2]={};Check(SelectorLayer291(target,eyes,source),"304 held-slot ghost renders even when no other holster is visible");auto gold=Pixels291(images[(acquired-1)%2],0);int goldCount=0;for(UINT px:gold)if((px&255)>100&&((px>>8)&255)>60&&((px>>16)&255)<40)++goldCount;Check(goldCount>20,"304 real GPU pixels show a gold return outline rather than the equipped native material");
 for(auto& eye:g_views)eye.pose.orientation={-sinf(.45f),0,0,cosf(.45f)};Check(SelectorLayer291(target,eyes,source),"304 ghost target stays located while looking down");Save291(Pixels291(images[(acquired-1)%2],0));CopyFileW(L"evidence/native-materials292-fixture-left.bmp",L"evidence/holsters304-stow-left.bmp",FALSE);for(auto& eye:g_views)eye.pose.orientation={0,0,0,1};
 g_stowReady304=true;Check(SelectorLayer291(target,eyes,source),"304 ready target renders");auto ready=Pixels291(images[(acquired-1)%2],0);int greenCount=0;for(UINT px:ready)if(((px>>8)&255)>180&&(px&255)<60&&((px>>16)&255)<100)++greenCount;Check(greenCount>20&&ready!=gold,"304 actual GPU target changes from gold to green when stow-ready");
 g_holsters297.stowOutline=false;Check(!SelectorLayer291(target,eyes,source),"304 outline toggle removes idle held-slot overlay");held.empty=true;g_holsters297.mode=fc3holster297::Four;g_zonesVisible304=true;for(auto& v:fc3visual292::fixtureVisual)v.ready=false;
 fc3pad::fixtureMenu=true;Check(SelectorLayer291(target,eyes,source),"304 Insert menu can show all four trigger boundaries with no weapon cache");auto zones=Pixels291(images[(acquired-1)%2],0),zonesRight=Pixels291(images[(acquired-1)%2],1);int coloured=0;for(UINT px:zones)if(px>>24)++coloured;Check(coloured>100&&zones!=zonesRight,"304 cache-independent grab boundaries have visible stereo depth");for(auto& eye:g_views)eye.pose.orientation={-sinf(.45f),0,0,cosf(.45f)};Check(SelectorLayer291(target,eyes,source),"304 all four seated boundaries remain visible while looking down");Save291(Pixels291(images[(acquired-1)%2],0));for(auto& eye:g_views)eye.pose.orientation={0,0,0,1};CopyFileW(L"evidence/native-materials292-fixture-left.bmp",L"evidence/holsters304-zones-left.bmp",FALSE);
 g_holsters297.positions[0].x+=.18f;Check(SelectorLayer291(target,eyes,source),"304 adjusted holster boundary renders live while Insert is open");Check(Pixels291(images[(acquired-1)%2],0)!=zones,"304 actual boundary pixels move with the per-slot adjustment");
 fc3pad::fixtureMenu=false;g_zonesVisible304=false;g_holsters297={};g_holsters297.mode=fc3holster297::Front;held={};held.enabled=true;held.empty=true;for(auto& v:fc3visual292::fixtureVisual)v.ready=true;
 }
 // Destroy cached GPU resources and restore on a genuinely different device.
 SelectorGpuRelease291();for(auto& v:fc3visual292::fixtureVisual)v=fc3visual292::WeaponVisual{};
 ID3D11Device* previousDevice296=device;ID3D11DeviceContext* freshContext296=nullptr;ID3D11Device* freshDevice296=nullptr;
 Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&freshDevice296,nullptr,&freshContext296)),"new D3D device simulates another game launch");device=freshDevice296;
 fc3visual292::WeaponVisual refused296;auto corrupt296=diskArchives296[0];corrupt296.back()^=1;Check(!fc3cache296::Decode(device,corrupt296,100,refused296),"corrupted cache payload is rejected");Check(!fc3cache296::Decode(device,diskArchives296[0],999,refused296),"wrong owned weapon ID cannot load another appearance");corrupt296=diskArchives296[0];corrupt296.resize(20);Check(!fc3cache296::Decode(device,corrupt296,100,refused296),"truncated cache is rejected");
 inv.owner=900;for(int i=0;i<4;++i)inv.weapons[i].entity=300+i;inv.equipped=302;const int beforeDisk296=fc3select290::fixtureCalls;fc3visual292::diskPreload296=true;
 fc3pad::Controllers reopen296={};reopen296.active=true;Wrist({0,1.3f,0});Controls(reopen296);reopen296.a=true;reopen296.rightGrip=1;Controls(reopen296);reopen296.a=false;reopen296.rightGrip=0;Controls(reopen296);
 Check(g_selectGesture290.open&&g_warm292.done&&fc3select290::fixtureCalls==beforeDisk296,"persisted four-weapon rack opens on new entities without any native setup switch");
 ID3D11Texture2D* freshSource296=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&freshSource296)),"new process source image");XrCompositionLayerProjection diskLayer296={};XrCompositionLayerProjectionView diskViews296[2]={};Check(SelectorLayer291(diskLayer296,diskViews296,freshSource296),"persisted native previews render on the new device");const auto restoredPixels296=Pixels291(images[(acquired-1)%2],0);Check(restoredPixels296==referencePixels296,"restored native skins, UVs, attachments and stereo placement produce pixel-identical output");
 for(int kind=0;kind<2;++kind){ID3D11Resource* res=nullptr;ID3D11Texture2D* tex=nullptr;fc3visual292::fixtureVisual[0].parts[0]->textures[14+kind]->GetResource(&res);res->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&tex);D3D11_TEXTURE2D_DESC restored={};tex->GetDesc(&restored);Check(restored.MipLevels==3&&restored.ArraySize==(kind?2u:6u)&&restored.Format==(kind?DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:DXGI_FORMAT_BC1_UNORM),"compressed mip/cube and sRGB array resource descriptions survive restart");res->Release();tex->Release();}
 Check(fc3visual292::fixtureVisual[0].parts[0]->textures[3]==fc3visual292::fixtureVisual[0].parts[1]->textures[3],"shared native texture views remain shared after disk restore");
 SelectorGpuRelease291();for(auto& v:fc3visual292::fixtureVisual)v=fc3visual292::WeaponVisual{};freshContext296->ClearState();freshSource296->Release();freshContext296->Release();freshDevice296->Release();device=previousDevice296;
 immediate->ClearState();original->Release();source->Release();immediate->Release();device->Release();return 0;
}
