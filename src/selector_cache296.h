#pragma once
// Private local GPU snapshots. No pointers or game assets enter the repository.
#include <cstdio>
#include <map>
#include <atomic>
#include <limits>
#include "selector_readback359.h"
namespace fc3cache296 {
constexpr size_t Limit=192u*1024*1024;
inline unsigned long long Hash(const BYTE* p,size_t n){unsigned long long h=14695981039346656037ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}
struct Writer {
 std::vector<BYTE> bytes;bool ok=true;
 void Raw(const void* p,size_t n){if(!ok||n>Limit||bytes.size()>Limit-n){ok=false;return;}const auto* a=(const BYTE*)p;if(n)bytes.insert(bytes.end(),a,a+n);}
 template<class T>void Put(const T& v){Raw(&v,sizeof(v));}
 void Blob(const std::vector<BYTE>& b){UINT n=UINT(b.size());Put(n);Raw(b.data(),n);}
};
struct Reader {
 const std::vector<BYTE>& bytes;size_t at=0;bool ok=true;
 void Raw(void* p,size_t n){if(!ok||at>bytes.size()||n>bytes.size()-at){ok=false;return;}if(n)memcpy(p,bytes.data()+at,n);at+=n;}
 template<class T>T Get(){T v={};Raw(&v,sizeof(v));return v;}
 std::vector<BYTE> Blob(size_t max=Limit){const UINT n=Get<UINT>();if(!ok||n>max||n>bytes.size()-at){ok=false;return {};}std::vector<BYTE> v(n);Raw(v.data(),n);return v;}
};
inline bool Buffer(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Buffer* src,std::vector<BYTE>& out,UINT wanted=0){
 if(!src){out.clear();return true;}D3D11_BUFFER_DESC desc={};src->GetDesc(&desc);if(!desc.ByteWidth||desc.ByteWidth>32*1024*1024)return false;
 if(!wanted)wanted=desc.ByteWidth;if(wanted>desc.ByteWidth)return false;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=desc.StructureByteStride=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ID3D11Buffer* stage=nullptr;if(auto* batch=fc3readback359::Current()){auto* frozen=batch->Find(src);if(!frozen||FAILED(frozen->QueryInterface(__uuidof(ID3D11Buffer),(void**)&stage)))return false;}else{if(FAILED(d->CreateBuffer(&desc,nullptr,&stage)))return false;c->CopyResource(stage,src);}D3D11_MAPPED_SUBRESOURCE m={};bool ok=SUCCEEDED(c->Map(stage,0,D3D11_MAP_READ,fc3readback359::MapFlags(),&m));if(ok){out.assign((BYTE*)m.pData,(BYTE*)m.pData+wanted);c->Unmap(stage,0);}fc3visual292::Drop(stage);return ok;
}
inline UINT Rows(DXGI_FORMAT f,UINT height){return (f>=DXGI_FORMAT_BC1_TYPELESS&&f<=DXGI_FORMAT_BC5_SNORM)||(f>=DXGI_FORMAT_BC6H_TYPELESS&&f<=DXGI_FORMAT_BC7_UNORM_SRGB)?(height+3)/4:height;}
inline UINT MinPitch(DXGI_FORMAT f,UINT width){const UINT n=UINT(f);if((n>=70&&n<=84)||(n>=94&&n<=99))return ((width+3)/4)*((n<=72||(n>=79&&n<=81))?8:16);
 UINT pixel=n>=1&&n<=4?16:n>=5&&n<=8?12:n>=9&&n<=22?8:n>=23&&n<=47?4:n>=48&&n<=59?2:n>=60&&n<=65?1:n==67?4:n==85||n==86?2:n>=87&&n<=93?4:0;return pixel*width;}
inline bool SaveTexture(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11ShaderResourceView* view,Writer& w){
 ID3D11Resource* res=nullptr;ID3D11Texture2D* tex=nullptr;view->GetResource(&res);const HRESULT qi=res?res->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&tex):E_FAIL;fc3visual292::Drop(res);if(FAILED(qi))return false;
 D3D11_TEXTURE2D_DESC desc={};tex->GetDesc(&desc);D3D11_SHADER_RESOURCE_VIEW_DESC srv={};view->GetDesc(&srv);
 if(desc.SampleDesc.Count!=1||!desc.MipLevels||desc.MipLevels>15||!desc.ArraySize||desc.ArraySize>12||!desc.Width||!desc.Height||desc.Width>16384||desc.Height>16384||!MinPitch(desc.Format,desc.Width)){fc3visual292::Drop(tex);return false;}
 w.Put(desc);w.Put(srv);auto staging=desc;staging.Usage=D3D11_USAGE_STAGING;staging.BindFlags=0;staging.CPUAccessFlags=D3D11_CPU_ACCESS_READ;staging.MiscFlags=0;ID3D11Texture2D* stage=nullptr;
 bool ok=false;if(auto* batch=fc3readback359::Current()){auto* frozen=batch->Find(tex);ok=frozen&&SUCCEEDED(frozen->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&stage));}else{ok=SUCCEEDED(d->CreateTexture2D(&staging,nullptr,&stage));if(ok)c->CopyResource(stage,tex);}fc3visual292::Drop(tex);
 for(UINT a=0;a<desc.ArraySize&&ok;++a)for(UINT mip=0;mip<desc.MipLevels&&ok;++mip){const UINT sub=D3D11CalcSubresource(mip,a,desc.MipLevels);D3D11_MAPPED_SUBRESOURCE map={};ok=SUCCEEDED(c->Map(stage,sub,D3D11_MAP_READ,fc3readback359::MapFlags(),&map));if(!ok)break;
  const UINT height=(std::max)(1u,desc.Height>>mip),rows=Rows(desc.Format,height);const UINT pitch=map.RowPitch;
  if(!pitch||UINT64(pitch)*rows>Limit)ok=false;else{w.Put(pitch);w.Put(rows);w.Raw(map.pData,size_t(pitch)*rows);ok=w.ok;}c->Unmap(stage,sub);
 }fc3visual292::Drop(stage);return ok&&w.ok;
}
inline bool LoadTexture(ID3D11Device* d,Reader& r,ID3D11ShaderResourceView** out){
 auto desc=r.Get<D3D11_TEXTURE2D_DESC>();const auto srv=r.Get<D3D11_SHADER_RESOURCE_VIEW_DESC>();
 if(!r.ok||desc.SampleDesc.Count!=1||!desc.Width||!desc.Height||desc.Width>16384||desc.Height>16384||!desc.MipLevels||desc.MipLevels>15||!desc.ArraySize||desc.ArraySize>12||!MinPitch(desc.Format,desc.Width))return false;
 std::vector<D3D11_SUBRESOURCE_DATA> data;for(UINT a=0;a<desc.ArraySize;++a)for(UINT m=0;m<desc.MipLevels;++m){const UINT pitch=r.Get<UINT>(),rows=r.Get<UINT>();
  if(!r.ok||pitch<MinPitch(desc.Format,(std::max)(1u,desc.Width>>m))||pitch>262144||rows!=Rows(desc.Format,(std::max)(1u,desc.Height>>m))||UINT64(pitch)*rows>r.bytes.size()-r.at)return false;
  data.push_back({r.bytes.data()+r.at,pitch,0});r.at+=size_t(pitch)*rows;
 }
 desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=0;desc.MiscFlags&=D3D11_RESOURCE_MISC_TEXTURECUBE;ID3D11Texture2D* tex=nullptr;
 const bool ok=SUCCEEDED(d->CreateTexture2D(&desc,data.data(),&tex))&&SUCCEEDED(d->CreateShaderResourceView(tex,&srv,out));fc3visual292::Drop(tex);return ok;
}
inline bool Encode(ID3D11Device* d,ID3D11DeviceContext* c,const fc3visual292::WeaponVisual& v,Writer& out){
 if(!v.ready||!v.id||v.parts.empty()||v.parts.size()>32)return false;Writer w;
 std::map<ID3D11ShaderResourceView*,UINT> indices;std::vector<ID3D11ShaderResourceView*> views;
 for(const auto& p:v.parts)for(auto* srv:p->textures)if(srv&&!indices.count(srv)){indices[srv]=UINT(views.size());views.push_back(srv);}
 if(views.size()>128)return false;w.Put(UINT(views.size()));for(auto* srv:views)if(!SaveTexture(d,c,srv,w))return false;
 w.Put(UINT(v.parts.size()));w.Raw(v.fingers,sizeof(v.fingers));
 for(const auto& p:v.parts){if(!p->ready||!p->count||p->count>300000||!p->stride||p->stride>512||p->placeCode.empty()||p->materialCode.empty()||p->inputs.empty()||p->inputs.size()>32)return false;
  w.Put(p->count);w.Put(p->stride);w.Put(p->positionOffset);w.Put(UINT(p->hand));w.Put(UINT(p->linearAlbedo));w.Raw(p->clipToGrip,64);w.Blob(p->placeCode);w.Blob(p->materialCode);w.Put(UINT(p->inputs.size()));for(const auto& input:p->inputs)w.Put(input);
  std::vector<BYTE> bytes;if(!Buffer(d,c,p->vertices,bytes,p->count*p->stride))return false;w.Blob(bytes);
  for(auto* cb:p->constants){if(!Buffer(d,c,cb,bytes))return false;w.Blob(bytes);}
  for(auto* srv:p->textures)w.Put(srv?indices[srv]:~0u);
  for(auto* sampler:p->samplers){w.Put(UINT(sampler!=nullptr));if(sampler){D3D11_SAMPLER_DESC desc={};sampler->GetDesc(&desc);w.Put(desc);}}
 }
 if(!w.ok)return false;out.Put(UINT(0x36393253));out.Put(UINT(1));out.Put(v.id);out.Put(Hash(w.bytes.data(),w.bytes.size()));out.Raw(w.bytes.data(),w.bytes.size());return out.ok;
}
inline bool Decode(ID3D11Device* d,const std::vector<BYTE>& bytes,unsigned long long id,fc3visual292::WeaponVisual& out){
 if(!d||bytes.size()>Limit)return false;Reader r{bytes};if(r.Get<UINT>()!=0x36393253||r.Get<UINT>()!=1||r.Get<unsigned long long>()!=id)return false;const auto hash=r.Get<unsigned long long>();if(!r.ok||hash!=Hash(bytes.data()+r.at,bytes.size()-r.at))return false;
 const UINT textures=r.Get<UINT>();if(!r.ok||textures>128)return false;
 struct Pool{std::vector<ID3D11ShaderResourceView*> views;~Pool(){for(auto*& p:views)fc3visual292::Drop(p);}}pool;
 for(UINT i=0;i<textures;++i){ID3D11ShaderResourceView* srv=nullptr;if(!LoadTexture(d,r,&srv)){fc3visual292::Drop(srv);return false;}pool.views.push_back(srv);}
 fc3visual292::WeaponVisual v;const UINT parts=r.Get<UINT>();if(!r.ok||!parts||parts>32)return false;r.Raw(v.fingers,sizeof(v.fingers));for(float f:v.fingers)if(!std::isfinite(f))return false;
 for(UINT i=0;i<parts&&r.ok;++i){auto p=std::make_shared<fc3visual292::Part>();p->count=r.Get<UINT>();p->stride=r.Get<UINT>();p->positionOffset=r.Get<UINT>();p->hand=r.Get<UINT>()!=0;p->linearAlbedo=r.Get<UINT>()!=0;r.Raw(p->clipToGrip,64);
  if(!r.ok||!p->count||p->count>300000||!p->stride||p->stride>512||p->positionOffset>p->stride||p->stride-p->positionOffset<16)return false;
  for(float f:p->clipToGrip)if(!std::isfinite(f))return false;
  p->placeCode=r.Blob(65536);p->materialCode=r.Blob(65536);const UINT inputs=r.Get<UINT>();if(!r.ok||!inputs||inputs>32)return false;
  std::vector<D3D11_INPUT_ELEMENT_DESC> elements;for(UINT n=0;n<inputs;++n){const auto input=r.Get<fc3visual292::Part::Input>();if(!r.ok||input.index>=32||input.offset>=p->stride)return false;p->inputs.push_back(input);elements.push_back({"CAPTURE",input.index,DXGI_FORMAT(input.format),0,input.offset,D3D11_INPUT_PER_VERTEX_DATA,0});}
  auto vertex=r.Blob(32*1024*1024);if(!r.ok||vertex.size()!=size_t(p->count)*p->stride)return false;
  D3D11_BUFFER_DESC bd={};bd.ByteWidth=UINT(vertex.size());bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.Usage=D3D11_USAGE_IMMUTABLE;D3D11_SUBRESOURCE_DATA init={vertex.data(),0,0};
  if(FAILED(d->CreateBuffer(&bd,&init,&p->vertices))||FAILED(d->CreateVertexShader(p->placeCode.data(),p->placeCode.size(),nullptr,&p->place))||FAILED(d->CreatePixelShader(p->materialCode.data(),p->materialCode.size(),nullptr,&p->material))||FAILED(d->CreateInputLayout(elements.data(),UINT(elements.size()),p->placeCode.data(),p->placeCode.size(),&p->layout)))return false;
  for(auto*& cb:p->constants){auto blob=r.Blob(65536);if(!r.ok)return false;if(blob.empty())continue;if(blob.size()%16)return false;bd.ByteWidth=UINT(blob.size());bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;init.pSysMem=blob.data();if(FAILED(d->CreateBuffer(&bd,&init,&cb)))return false;}
  for(auto*& srv:p->textures){const UINT index=r.Get<UINT>();if(!r.ok)return false;if(index==~0u)continue;if(index>=pool.views.size())return false;srv=pool.views[index];srv->AddRef();}
  for(auto*& sampler:p->samplers){const UINT present=r.Get<UINT>();if(!r.ok||present>1)return false;if(present){const auto desc=r.Get<D3D11_SAMPLER_DESC>();if(!r.ok||FAILED(d->CreateSamplerState(&desc,&sampler)))return false;}}
  p->ready=true;v.parts.push_back(p);
 }
 if(!r.ok||r.at!=bytes.size())return false;v.id=id;v.ready=true;out=std::move(v);return true;
}
inline bool ReadFile(const wchar_t* path,std::vector<BYTE>& bytes){FILE* f=nullptr;if(_wfopen_s(&f,path,L"rb")||!f)return false;bool ok=!_fseeki64(f,0,SEEK_END);const auto n=_ftelli64(f);ok=ok&&n>24&&n<=Limit&&!_fseeki64(f,0,SEEK_SET);if(ok){bytes.resize(size_t(n));ok=fread(bytes.data(),1,bytes.size(),f)==bytes.size();}fclose(f);return ok;}
inline bool WriteFile(const wchar_t* path,const std::vector<BYTE>& bytes){if(bytes.empty()||bytes.size()>Limit)return false;std::wstring temp=std::wstring(path)+L".partial";FILE* f=nullptr;if(_wfopen_s(&f,temp.c_str(),L"wb")||!f)return false;const bool wrote=fwrite(bytes.data(),1,bytes.size(),f)==bytes.size();const bool closed=fclose(f)==0;return wrote&&closed&&MoveFileExW(temp.c_str(),path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}
// Serialize GPU state on its owning thread; file I/O alone runs on a worker.
// Pin the DLL for that worker and release with FreeLibraryAndExitThread, so
// neither shutdown nor a hot unload can execute its code after module removal.
class DiskWriter {
 HANDLE worker=nullptr;std::atomic<bool> busy{false};std::atomic<int> result{0};
 struct Job{DiskWriter* owner;HMODULE module;std::wstring path;std::vector<BYTE> bytes;};
 static DWORD WINAPI Run(void* raw){auto* job=(Job*)raw;job->owner->result=WriteFile(job->path.c_str(),job->bytes)?1:-1;job->owner->busy=false;const auto module=job->module;delete job;FreeLibraryAndExitThread(module,0);return 0;}
 public:~DiskWriter(){if(worker)CloseHandle(worker);}
 bool Busy()const{return busy.load();}
 int TakeResult(){return result.exchange(0);}
 bool Start(std::wstring path,std::vector<BYTE> bytes){if(busy.load())return false;HMODULE module=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(&Run),&module))return false;
  if(worker){CloseHandle(worker);worker=nullptr;}auto* job=new Job{this,module,std::move(path),std::move(bytes)};result=0;busy=true;worker=CreateThread(nullptr,0,Run,job,0,nullptr);if(!worker){busy=false;delete job;FreeLibrary(module);return false;}return true;}
 void Wait(){if(worker)WaitForSingleObject(worker,INFINITE);}
};
}
