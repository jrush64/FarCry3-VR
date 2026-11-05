// Included after existing native weapon/arm math. The earlier draw hook only
// declares these observers; no working weapon/reload path is replaced.
} // existing anonymous engine implementation namespace
#include "selector_expected292.h"
#include "selector_wrist293.h"
#include "selector_passive295.h"
#include "selector_capture331.h"
namespace fc3visual292 {
fc3capture331::Gate g_captureGate331;
WeaponVisual g_visual[4];fc3select290::Inventory g_identity;int g_target=-1;
unsigned long long g_cacheTried296[4]={};bool g_refreshLive296[4]={};
std::vector<WeaponVisual> g_save296;fc3cache296::DiskWriter g_writer296;
std::wstring CachePath296(unsigned long long id){wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sselector296",fc3::DataFolder());CreateDirectoryW(path,nullptr);wchar_t name[40];_snwprintf_s(name,40,_TRUNCATE,L"\\%016llX.fcv296",id);return std::wstring(path)+name;}
fc3readback359::Batch g_readback359;WeaponVisual g_readbackVisual359;ULONGLONG g_readbackAt359=0,g_cacheQuiet359=0;LONG g_cacheFrame359=-100;
void SavePump296(){
 if(g_cacheFrame359==g_frame)return;g_cacheFrame359=g_frame;
 const auto now=GetTickCount64();const auto controls=fc3pad::Latest();
 const bool quiet=controls.active&&controls.leftTrigger<.1f&&controls.rightTrigger<.1f&&controls.leftGrip<.35f&&controls.rightGrip<.35f&&!controls.a&&!controls.b&&!controls.x&&!controls.y;
 if(!quiet)g_cacheQuiet359=0;else if(!g_cacheQuiet359)g_cacheQuiet359=now;
 const int result=g_writer296.TakeResult();if(result)Log("[SELECT296] persistent appearance disk write %s",result>0?"completed":"FAILED");
 if(g_writer296.Busy())return;
 if(g_readbackAt359){
  const int ready=g_readback359.Ready(g_ctx);
  if(!ready&&now>=g_readbackAt359&&now-g_readbackAt359<=3000)return;
  if(ready>0&&(!g_cacheQuiet359||now<g_cacheQuiet359||now-g_cacheQuiet359<750))return;
  fc3cache296::Writer bytes;bool ok=false;
  if(ready>0){fc3readback359::Scope snapshot(g_readback359);const bool own=g_own;g_own=true;ok=fc3cache296::Encode(g_dev,g_ctx,g_readbackVisual359,bytes);g_own=own;}
  const auto id=g_readbackVisual359.id;g_readback359.Clear();g_readbackVisual359={};g_readbackAt359=0;
  if(ok){const auto size=bytes.bytes.size();const bool queued=g_writer296.Start(CachePath296(id),std::move(bytes.bytes));Log("[CACHE359] completed GPU snapshot %s id=%016llX bytes=%u; nonwaiting maps, idle serialization",queued?"queued":"write start FAILED",id,unsigned(size));}
  else Log("[CACHE359] readback expired/unsupported id=%016llX; original in-memory appearance retained",id);
  const auto elapsed=GetTickCount64()-now;if(elapsed>8)Log("[CACHE359] idle CPU serialization elapsed=%llums GPUReady=%d",elapsed,ready);
  return;
 }
 if(g_save296.empty())return;
 g_readbackVisual359=std::move(g_save296.front());g_save296.erase(g_save296.begin());bool ok=true;
 const bool own=g_own;g_own=true;
 for(const auto& part:g_readbackVisual359.parts){
  if(!part){ok=false;break;}ok=ok&&g_readback359.Add(g_dev,g_ctx,part->vertices);
  for(auto* cb:part->constants)if(cb)ok=ok&&g_readback359.Add(g_dev,g_ctx,cb);
  for(auto* view:part->textures)if(view){ID3D11Resource* resource=nullptr;view->GetResource(&resource);ok=ok&&g_readback359.Add(g_dev,g_ctx,resource);Drop(resource);}
 }
 ok=ok&&g_readback359.End(g_dev,g_ctx);g_own=own;
 if(ok)g_readbackAt359=now;else{g_readback359.Clear();g_readbackVisual359={};Log("[CACHE359] staging snapshot refused; in-memory appearance retained");}
}
void LoadCache296(const fc3select290::Inventory& inv){
 if(!inv.valid||!g_dev)return;
 for(int i=0;i<4;++i){const auto& w=inv.weapons[i];if(!w.entity||g_visual[i].ready||g_cacheTried296[i]==w.id)continue;g_cacheTried296[i]=w.id;
  std::vector<BYTE> bytes;if(!fc3cache296::ReadFile(CachePath296(w.id).c_str(),bytes))continue;WeaponVisual loaded;
  const bool own=g_own;g_own=true;const bool ok=fc3cache296::Decode(g_dev,bytes,w.id,loaded);g_own=own;
  if(!ok){Log("[SELECT296] persistent appearance rejected id=%016llX",w.id);continue;}
  loaded.owner=inv.owner;loaded.entity=w.entity;g_visual[i]=std::move(loaded);g_refreshLive296[i]=true;
  Log("[SELECT296] persistent appearance loaded slot=%d id=%016llX native parts=%u; no weapon cycling",i+1,w.id,unsigned(g_visual[i].parts.size()));
 }
}
LONG g_captureFrame=-1;bool g_captureFailed=false,g_wristValid=false,g_coreSeen=false;Mat4 g_nativeWrist={};
std::vector<std::shared_ptr<Part>> g_pending;
std::shared_ptr<Part> g_handPart;
float g_captureFingers[25*12]={};ULONGLONG g_targetAt=0,g_contactAt=0;
Mat4 g_contactWrist={};int g_contactSlot=-1;float g_contactGrip=0;
void ClearContact(){g_contactAt=0;g_contactSlot=-1;}
void Contact(const float* wrist,int slot,float grip){
 if(!wrist||slot<0||slot>3||!g_visual[slot].ready){ClearContact();return;}
 for(int i=0;i<16;++i){if(!std::isfinite(wrist[i])){ClearContact();return;}g_contactWrist.m[i]=wrist[i];}
 g_contactSlot=slot;g_contactGrip=(std::max)(0.f,(std::min)(1.f,grip));g_contactAt=GetTickCount64();
}
volatile LONG g_preparing330=0;
bool Preparing330(){return InterlockedCompareExchange(&g_preparing330,0,0)!=0;}
void SetPreparing330(bool active){InterlockedExchange(&g_preparing330,active?1:0);}
bool Hidden(){return fc3xr::SelectorOpen291()||Preparing330();}
void Target(const fc3select290::Inventory& inv,int slot){
 SavePump296();
 bool changed=inv.owner!=g_identity.owner;
 for(int i=0;i<4;++i)changed=changed||inv.weapons[i].id!=g_identity.weapons[i].id||inv.weapons[i].entity!=g_identity.weapons[i].entity;
 if(changed){for(auto& visual:g_visual)visual=WeaponVisual{};memset(g_cacheTried296,0,sizeof(g_cacheTried296));memset(g_refreshLive296,0,sizeof(g_refreshLive296));g_pending.clear();g_captureFrame=-1;ClearContact();}
 const bool arrived=slot>=0&&slot<4&&g_identity.equipped!=inv.weapons[slot].entity&&inv.equipped==inv.weapons[slot].entity;
 const bool newTarget=changed||slot!=g_target||arrived;g_identity=inv;g_target=slot;
 LoadCache296(inv);
 if(newTarget){g_captureGate331.Reset();g_pending.clear();g_captureFrame=-1;g_captureFailed=false;g_wristValid=false;g_targetAt=GetTickCount64();}
}
const WeaponVisual* Read(const fc3select290::Inventory& inv,int slot){
 if(slot<0||slot>3||!inv.valid)return nullptr;const auto& v=g_visual[slot];
 return v.ready&&v.owner==inv.owner&&v.id==inv.weapons[slot].id&&v.entity==inv.weapons[slot].entity?&v:nullptr;
}
void Preload296(const fc3select290::Inventory& inv){
 if(!inv.valid)return;bool changed=inv.owner!=g_identity.owner;
 for(int i=0;i<4;++i)changed=changed||inv.weapons[i].id!=g_identity.weapons[i].id||inv.weapons[i].entity!=g_identity.weapons[i].entity;
 if(changed)Target(inv,-1);else LoadCache296(inv);
}
void LockerVisual329(const fc3select290::Inventory& inv){
 static fc3locker329::Visual last;static bool first=true;static fc3locker329::Budget budget;
 const auto native=fc3locker329::Read();fc3locker329::Visual v;v.nativeRevision=native.revision;v.owner=inv.owner;v.equipped=inv.equipped;v.valid=inv.valid;v.target=g_target;
 for(int i=0;i<4;++i){v.id[i]=inv.weapons[i].id;v.entity[i]=inv.weapons[i].entity;v.parts[i]=unsigned(g_visual[i].parts.size());v.rawId[i]=g_visual[i].id;v.rawEntity[i]=g_visual[i].entity;v.modelHash[i]=14695981039346656037ull;for(const auto& part:g_visual[i].parts)if(part){const UINT64 words[]={part->count,part->stride,part->hand,part->shader,part->materialKey};for(auto word:words)v.modelHash[i]=(v.modelHash[i]^word)*1099511628211ull;}
  if(Read(inv,i))v.ready|=1u<<i;if(g_visual[i].ready)v.rawReady|=1u<<i;if(g_refreshLive296[i])v.refresh|=1u<<i;if(g_cacheTried296[i]&&g_cacheTried296[i]==v.id[i])v.tried|=1u<<i;
 }
 if(!first&&v==last)return;first=false;last=v;if(!budget.Take(256))return;
 Log("[LOCKER329 VISUAL] ms=%llu nativeRevision=%u valid=%d owner=%016llX equipped=%08X target=%d readyMask=%X rawReadyMask=%X refreshMask=%X diskTriedMask=%X",GetTickCount64(),native.revision,v.valid,v.owner,v.equipped,v.target+1,v.ready,v.rawReady,v.refresh,v.tried);
 for(int i=0;i<4;++i)Log("[LOCKER329 CACHE] slot=%d shownName=%s expected=%08X/%016llX nativeName=%s native=%08X/%016llX cached=%08X/%016llX parts=%u shape=%016llX usable=%d",i+1,inv.weapons[i].name,v.entity[i],v.id[i],native.inv.weapons[i].name,native.inv.weapons[i].entity,native.inv.weapons[i].id,v.rawEntity[i],v.rawId[i],v.parts[i],v.modelHash[i],(v.ready>>i)&1);
}
void Passive295(const fc3select290::Inventory& inv){
 LockerVisual329(inv);
 if(!inv.valid){SavePump296();g_target=-1;g_pending.clear();g_captureFrame=-1;return;} // suspend; preserve cache
 bool ready[4];for(int i=0;i<4;++i)ready[i]=Read(inv,i)!=nullptr&&!g_refreshLive296[i];
 // Observation only: never request a native weapon switch in this path.
 Target(inv,fc3passive295::EquippedMissing(inv,ready));
}
bool Capturing(){
 if(g_target<0||g_target>=4||!g_identity.valid||GetTickCount64()-g_identity.at>150||g_identity.equipped!=g_identity.weapons[g_target].entity||(g_visual[g_target].ready&&!g_refreshLive296[g_target])||(!Preparing330()&&GetTickCount64()-g_targetAt<500))return false;
 unsigned wrapper=0,weapon=0,strategy=0;return fc3aim::EquippedWeapon206(wrapper,weapon,strategy)&&wrapper==g_identity.weapons[g_target].wrapper;
}
void LockerDraw329(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(call.kind!=0||!call.a||(variant!=0&&variant!=1))return;
 static fc3locker329::Budget budget;static unsigned revision=0;static UINT64 seen=0;static LONG frame=-1;static fc3locker329::Native n;static ULONGLONG now=0,changedAt=0;if(budget.used>=128)return;
 if(frame!=g_frame){frame=g_frame;now=GetTickCount64();n=fc3locker329::Read();if(revision!=n.revision){revision=n.revision;seen=0;changedAt=now;}}
 if(!n.revision||!n.inv.valid||now-n.inv.at>2000||now-changedAt>15000)return;
 bool potential=false;for(unsigned i=0;i<sizeof(fc3weapon::kProfiles)/sizeof(*fc3weapon::kProfiles);++i){const auto& p=fc3weapon::kProfiles[i];if(i<64&&!(seen&(UINT64(1)<<i))&&p.start==call.b&&p.count==call.a){potential=true;break;}}
 if(!potential)return;const auto geometry=NativeWeaponGeometry(context,call);const int profile=fc3weapon::ProfileIndex(geometry);if(profile<0||profile>=64||(seen&(UINT64(1)<<profile)))return;
 seen|=UINT64(1)<<profile;if(!budget.Take(128))return;unsigned matches=0;for(int i=0;i<4;++i){const char* expected=SelectorExpectedBody292(n.inv.weapons[i].name);if(expected&&!strcmp(expected,fc3weapon::kProfileNames[profile]))matches|=1u<<i;}
 Log("[LOCKER329 DRAW] ms=%llu revision=%u frame=%ld profile=%d body=%s variant=%d matchingSlotMask=%X equipped=%08X captureTarget=%d capturing=%d; native draw observed, not requested",GetTickCount64(),n.revision,g_frame,profile,fc3weapon::kProfileNames[profile],variant,matches,n.inv.equipped,g_target+1,Capturing());
}
void StartFrame(){if(g_captureFrame!=g_frame){g_pending.clear();g_handPart.reset();g_wristValid=false;g_coreSeen=false;g_captureFailed=false;g_captureFrame=g_frame;}}
void Finish(){
 if(g_target<0||g_captureFrame!=g_frame||g_pending.empty()||!g_wristValid||!g_coreSeen||!g_handPart||g_captureFailed){g_captureGate331.Reset();return;}
 if(Preparing330()){
  // Match the complete set of native geometry/material passes independent of
  // draw order, and require a consistent native grip over consecutive frames.
  std::vector<UINT64> keys;for(const auto& part:g_pending){UINT64 key=14695981039346656037ull;for(UINT64 word:{part->shader,part->materialKey,UINT64(part->count),UINT64(part->start),UINT64(part->vertexBytes),UINT64(part->indexBytes)})key=(key^word)*1099511628211ull;keys.push_back(key);}std::sort(keys.begin(),keys.end());UINT64 shape=14695981039346656037ull;for(auto key:keys)shape=(shape^key)*1099511628211ull;
  const bool settled=g_captureGate331.Frame(g_target,g_frame,GetTickCount64(),shape,g_captureFingers);static unsigned reports=0;if(reports++<48)Log("[LOCKER331] complete native capture slot=%d frame=%ld parts=%u stable=%d",g_target+1,g_frame,unsigned(keys.size()),settled);
  if(!settled)return;
 }
 g_pending.push_back(g_handPart);
 Mat4 inverse={};if(!Invert(g_nativeWrist,inverse))return;
 auto& v=g_visual[g_target];v=WeaponVisual{};v.owner=g_identity.owner;v.id=g_identity.weapons[g_target].id;v.entity=g_identity.weapons[g_target].entity;v.frame=g_frame;
 for(auto& part:g_pending){Mat4 inverseClip={};for(int i=0;i<16;++i)inverseClip.m[i]=part->clipToGrip[i];const auto local=Multiply(inverse,inverseClip);for(int i=0;i<16;++i)part->clipToGrip[i]=float(local.m[i]);}
 const bool own=g_own;g_own=true;bool geometry=true;for(const auto& part:g_pending)if(part->hand?!CropHand(g_dev,g_ctx,*part):!Geometry(g_dev,g_ctx,*part,v.collider.triangles)){geometry=false;break;}g_own=own;
 if(!geometry){v=WeaponVisual{};g_captureFailed=true;Log("[SELECT295] native collision geometry rejected; no invisible grab enabled");return;}v.collider.Build();
 v.parts=std::move(g_pending);memcpy(v.fingers,g_captureFingers,sizeof(v.fingers));v.ready=true;
 g_refreshLive296[g_target]=false;bool replaced=false;for(auto& queued:g_save296)if(queued.id==v.id){queued=v;replaced=true;break;}if(!replaced&&g_save296.size()<4)g_save296.push_back(v);
 Log("[SELECT295] owned visual ready slot=%d entity=%08X id=%016llX native material parts=%u; scale preserved; native hand grip",g_target+1,v.entity,v.id,unsigned(v.parts.size()));
}
void Release(){g_readback359.Clear();g_readbackVisual359={};g_readbackAt359=0;g_save296.clear();g_pending.clear();g_handPart.reset();for(auto& v:g_visual)v=WeaponVisual{};memset(g_cacheTried296,0,sizeof(g_cacheTried296));memset(g_refreshLive296,0,sizeof(g_refreshLive296));g_target=-1;g_captureFrame=-1;ClearContact();}
}
namespace {
void ObserveSelectorHand292(ID3D11DeviceContext* context,const DrawCall& call,int side){
 using namespace fc3visual292;if(side!=0||!Capturing())return;
 ID3D11RenderTargetView* rt[8]={};context->OMGetRenderTargets(8,rt,nullptr);UINT targets=0;for(auto*& p:rt){if(p)++targets;SafeRelease(p);}if(targets<2)return;
 StartFrame();
 float bones[48*12]={};Mat4 toView={},fromView={};if(!DrawSpace(context,bones,toView,fromView,3,true))return;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);CameraSummary camera={};UINT bytes=sizeof(camera);const bool ok=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&camera.mainView;SafeRelease(cb);if(!ok)return;
 Mat4 viewInverse={},wrist=BoneMatrix(bones,2),bind=HandIdentity();bind.m[3]=.591;bind.m[7]=.011;bind.m[11]=1.149;
 if(!Invert(HullView(camera),viewInverse))return;g_nativeWrist=Multiply(viewInverse,Multiply(toView,Multiply(wrist,bind)));g_wristValid=true;
 Mat4 inverseWrist={};if(!Invert(wrist,inverseWrist)){g_wristValid=false;return;}
 for(int b=0;b<25;++b)StoreBone(g_captureFingers,b,Multiply(inverseWrist,BoneMatrix(bones,b)));
 if(!g_handPart){auto part=std::make_shared<Part>();std::string why;const bool own=g_own;g_own=true;const bool captured=Capture(g_dev,context,call.a,call.b,call.d,kEjectionBytecodeGuid,*part,why);g_own=own;if(!captured){static UINT lines=0;if(lines++<8)Log("[SELECT295] native grip-hand capture refused: %s",why.c_str());g_captureFailed=true;return;}
  Mat4 projection={};projection.m[0]=camera.xScale;projection.m[5]=camera.yScale;projection.m[10]=camera.zScale;projection.m[11]=camera.zOffset;projection.m[14]=1;Mat4 inverse={};if(!Invert(Multiply(projection,HullView(camera)),inverse)){g_captureFailed=true;return;}
  for(int i=0;i<16;++i)part->clipToGrip[i]=float(inverse.m[i]);part->hand=true;g_handPart=part;
 }
}
void ObserveSelectorDraw292(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 fc3visual292::LockerDraw329(context,call,variant);
 using namespace fc3visual292;if(variant!=1||!Capturing()||call.kind!=0||!call.a||call.a%3)return;
 ID3D11VertexShader* shader=nullptr;context->VSGetShader(&shader,nullptr,nullptr);const auto hash=ShaderHash(shader);SafeRelease(shader);if(hash==kArmVsHash)return;
 ID3D11Buffer* vb=nullptr,*ib=nullptr;UINT stride=0,offset=0;DXGI_FORMAT format;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);context->IAGetIndexBuffer(&ib,&format,&offset);
 const bool body=vb&&HasFlag(vb,kBodyVbGuid);D3D11_BUFFER_DESC vd={},id={};if(vb)vb->GetDesc(&vd);if(ib)ib->GetDesc(&id);SafeRelease(vb);SafeRelease(ib);if(body||!vd.ByteWidth||!id.ByteWidth)return;
 ID3D11RenderTargetView* rt[8]={};context->OMGetRenderTargets(8,rt,nullptr);UINT targets=0;for(auto*& p:rt){if(p)++targets;SafeRelease(p);}if(targets<2)return; // exclude shadows, scope-image and single-target effects
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);CameraSummary camera={};UINT bytes=sizeof(camera);const bool ok=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&camera.mainView&&camera.xScale>.05&&camera.yScale>.05;SafeRelease(cb);if(!ok)return;
 StartFrame();
 const auto geometry=NativeWeaponGeometry(context,call);bool expectedCore=false;
 for(unsigned profile=0;profile<sizeof(fc3weapon::kProfiles)/sizeof(*fc3weapon::kProfiles);++profile){const auto& p=fc3weapon::kProfiles[profile];
  if(geometry.vertexBytes==p.vertexBytes&&geometry.indexBytes==p.indexBytes&&geometry.stride==p.stride&&geometry.offset==p.offset&&geometry.start==p.start&&geometry.count==p.count&&geometry.format==p.format){
   const char* expected=SelectorExpectedBody292(g_identity.weapons[g_target].name);if(!expected||strcmp(expected,fc3weapon::kProfileNames[profile])){g_captureFailed=true;return;}expectedCore=true;
  }
 }
 const auto materialKey=MaterialKey294(context);
 for(const auto& p:g_pending)if(p->shader==hash&&p->materialKey==materialKey&&p->start==call.b&&p->count==call.a&&p->vertexBytes==vd.ByteWidth&&p->indexBytes==id.ByteWidth)return;
 UINT64 total=0;for(const auto& p:g_pending)total+=UINT64(p->stride)*p->count;if(g_pending.size()>=32||total>64*1024*1024){g_captureFailed=true;return;}
 auto part=std::make_shared<Part>();std::string why;const bool own=g_own;g_own=true;const bool captured=Capture(g_dev,context,call.a,call.b,call.d,kEjectionBytecodeGuid,*part,why);g_own=own;
 if(!captured){if(why=="material is not a supported colour pass")return;static UINT lines=0;if(lines++<32)Log("[SELECT295] native part refused: %s",why.c_str());g_captureFailed=true;return;}
 Mat4 projection={};projection.m[0]=camera.xScale;projection.m[5]=camera.yScale;projection.m[10]=camera.zScale;projection.m[11]=camera.zOffset;projection.m[14]=1;
 Mat4 inverse={};if(!Invert(Multiply(projection,HullView(camera)),inverse)){g_captureFailed=true;return;}
 for(int i=0;i<16;++i)part->clipToGrip[i]=float(inverse.m[i]);part->shader=hash;part->materialKey=materialKey;part->vertexBytes=vd.ByteWidth;part->indexBytes=id.ByteWidth;g_pending.push_back(part);g_coreSeen=g_coreSeen||expectedCore;
 Log("[SELECT295] native material slot=%d part=%u key=%016llX linear-albedo=%d",g_target+1,unsigned(g_pending.size()),materialKey,part->linearAlbedo);
}
void SelectorContactMove292(Mat4& move,const Mat4& nativeWrist){
 using namespace fc3visual292;if(!g_contactAt||GetTickCount64()-g_contactAt>120||!fc3xr::SelectorOpen291())return;
 Mat4 bind=HandIdentity();bind.m[3]=.591;bind.m[7]=.011;bind.m[11]=1.149;
 Mat4 target=g_contactWrist;for(int r=0;r<3;++r)target.m[r*4+2]*=-1; // pose-local frame back to native bind-local frame
 Mat4 inverse={};if(Invert(Multiply(move,Multiply(nativeWrist,bind)),inverse))move=Multiply(Multiply(ArmBodyToCamera(),target),Multiply(inverse,move));
}
void SelectorFingers292(float* bones){
 using namespace fc3visual292;if(!fc3xr::SelectorOpen291())return;
 const bool contact=g_contactAt&&GetTickCount64()-g_contactAt<=120&&g_contactSlot>=0&&g_visual[g_contactSlot].ready;
 const Mat4 wrist=BoneMatrix(bones,2);const float amount=contact?g_contactGrip:0;
 for(int b=3;b<20;++b){Mat4 closed=wrist;if(contact)closed=Multiply(wrist,BoneMatrix(g_visual[g_contactSlot].fingers,b));for(int i=0;i<16;++i)closed.m[i]=wrist.m[i]+(closed.m[i]-wrist.m[i])*amount;StoreBone(bones,b,closed);}
}
