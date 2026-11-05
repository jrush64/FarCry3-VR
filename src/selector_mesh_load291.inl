#include "selector_mesh291.h"
#pragma pack(push,1)
struct SelectorMeshRecord291 {char name[80];unsigned long long offset;UINT count;float bounds[6];};
#pragma pack(pop)
struct SelectorMesh291 {char name[80]={};float bounds[6]={};std::vector<fc3select291::Vertex> vertices;ID3D11Buffer* buffer=nullptr;bool valid=false;};
SelectorMesh291 g_selectMesh291[4];
bool LoadSelectorMesh291(SelectorMesh291& mesh,const char* name){
 if(!strcmp(mesh.name,name))return mesh.valid;
 if(mesh.buffer)mesh.buffer->Release();mesh.buffer=nullptr;mesh.vertices.clear();mesh.valid=false;strcpy_s(mesh.name,name);
 wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_selector291.mesh",fc3::DataFolder());
 FILE* file=nullptr;_wfopen_s(&file,path,L"rb");if(!file)return false;
 struct Close {FILE* f;~Close(){fclose(f);}} close={file};
 _fseeki64(file,0,SEEK_END);const auto length=_ftelli64(file);rewind(file);char magic[8];UINT count=0;
 if(length<12||length>128*1024*1024||fread(magic,8,1,file)!=1||memcmp(magic,"FC3G291\0",8)||fread(&count,4,1,file)!=1||count>2048)return false;
 std::vector<SelectorMeshRecord291> records(count);if(count&&fread(records.data(),116,count,file)!=count)return false;
 for(const auto& r:records){if(!memchr(r.name,0,80)||strcmp(r.name,name))continue;
  if(r.count<3||r.count>300000||r.count%3||r.offset<12+count*116||r.offset+24ull*r.count>static_cast<unsigned long long>(length))return false;
  mesh.vertices.resize(r.count);_fseeki64(file,r.offset,SEEK_SET);if(fread(mesh.vertices.data(),24,r.count,file)!=r.count){mesh.vertices.clear();return false;}
  memcpy(mesh.bounds,r.bounds,24);for(const auto& v:mesh.vertices)for(float p:v.p)if(!std::isfinite(p)||std::fabs(p)>.6f){mesh.vertices.clear();return false;}
  for(float b:mesh.bounds)if(!std::isfinite(b)||std::fabs(b)>.6f)return false;
  mesh.valid=true;Log("[SELECT291] actual3D mesh ready name=%s triangles=%u",name,r.count/3);return true;
 }
 Log("[SELECT291] actual3D mesh missing name=%s; this slot cannot be grabbed",name);return false;
}
void SelectorMeshes291(const fc3select290::Inventory& inventory){for(int i=0;i<4;++i)if(inventory.weapons[i].entity)LoadSelectorMesh291(g_selectMesh291[i],inventory.weapons[i].name);}
int SelectorTouch291(fc3select290::Point hand){
 int hit=-1;float nearest=.06f*.06f;
 for(int i=0;i<4;++i){const auto& mesh=g_selectMesh291[i];if(!mesh.valid||!g_selectShown290.weapons[i].entity)continue;const auto p=fc3select291::Sub(hand,fc3select290::Centre(i));const float distance=fc3select291::Distance(mesh.vertices,mesh.bounds,p);if(distance<=nearest){nearest=distance;hit=i;}}
 return hit;
}
