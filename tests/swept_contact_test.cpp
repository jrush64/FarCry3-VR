#define FC3_AMMO_TEST 1
#include "../src/stereo.cpp"
#include <d3dcompiler.h>
#include <cstdlib>
void Check(bool ok,const char* what) { if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);} }
void STDMETHODCALLTYPE Update(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s,const D3D11_BOX* box,const void* p,UINT rp,UINT dp)
{ c->UpdateSubresource(r,s,box,p,rp,dp); }
ID3D11Buffer* Buffer(ID3D11Device* d,UINT size,UINT bind)
{
    D3D11_BUFFER_DESC desc={};desc.ByteWidth=size;desc.BindFlags=bind;ID3D11Buffer* b=nullptr;
    Check(SUCCEEDED(d->CreateBuffer(&desc,nullptr,&b)),"buffer creation");return b;
}
void Read(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Buffer* source,float* out)
{
    D3D11_BUFFER_DESC desc={};source->GetDesc(&desc);desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* stage=nullptr;Check(SUCCEEDED(d->CreateBuffer(&desc,nullptr,&stage)),"staging buffer");c->CopyResource(stage,source);
    D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(c->Map(stage,0,D3D11_MAP_READ,0,&m)),"read upload");std::memcpy(out,m.pData,desc.ByteWidth);c->Unmap(stage,0);stage->Release();
}
void Compare(const float* a,const float* b,int n,const char* what)
{for(int i=0;i<n;++i)Check(std::isfinite(a[i]) && std::fabs(a[i]-b[i])<2e-5f,what);}
void STDMETHODCALLTYPE ProbeCopy(ID3D11DeviceContext* c,ID3D11Resource* d,ID3D11Resource* s){c->CopyResource(d,s);}
HRESULT STDMETHODCALLTYPE ProbeMap(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s,D3D11_MAP t,UINT f,D3D11_MAPPED_SUBRESOURCE* m){return c->Map(r,s,t,f,m);}
void STDMETHODCALLTYPE ProbeUnmap(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s){c->Unmap(r,s);}
void STDMETHODCALLTYPE EjectCopyRange(ID3D11DeviceContext* c,ID3D11Resource* d,UINT ds,UINT x,UINT y,UINT z,ID3D11Resource* s,UINT ss,const D3D11_BOX* b){c->CopySubresourceRegion(d,ds,x,y,z,s,ss,b);}
#include "physical_ejection_fixture.inl"
#include "shell_visual_fixture.inl"
#include "ammo_fixture.inl"
#include "ammo_owner_fixture.inl"
#include "auto_ammo_fixture.inl"
#include "physical_reload_fixture.inl"
#include "animation_probe_fixture.inl"
#include "captured_hand_fixture.inl"
#include "pump_lock_fixture.inl"
#include "control_ownership_fixture.inl"
#include "reload_pose_fixture.inl"
#include "reload_count_trace_fixture.inl"
#include "one_shell_x_fixture.inl"
#include "machete_hold_fixture.inl"
#include "machete_melee_fixture.inl"
#include "machete_hold_mode_fixture.inl"
#include "machete_native_fixture.inl"
#include "machete_routing_fixture.inl"
#include "machete_quick_fixture.inl"
 #include "view_ownership_fixture.inl"
#include "machete_ownership_fixture.inl"
#include "machete_swept_fixture.inl"
int main(){wchar_t path[MAX_PATH];GetSystemDirectoryW(path,MAX_PATH);wcscat_s(path,L"\\d3d11.dll");auto create=reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(LoadLibraryW(path),"D3D11CreateDevice"));ID3D11Device* d=nullptr;ID3D11DeviceContext* c=nullptr;Check(SUCCEEDED(create(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)),"WARP");g_oUpdateSubresource=Update;TestSweptBlade();TestMacheteOwnership(d);TestViewOwnership(d,c);c->Release();d->Release();}