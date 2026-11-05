// Optional action cue: exact required mechanism meshes, state-driven and scoped
// across both eyes. Existing material textures, alpha and noncolour fields stay native.
#include "action_diffuse231.inl"
#include "special_action253.inl"
bool g_reloadActionGuide227=true,g_1911TintLoaded49=false;
int g_reloadActionColour227=0;
float g_smgHintStrength230=1;
bool SmgActionProfile230(){return g_weaponMechanicProfile==39||g_weaponMechanicProfile==7||g_weaponMechanicProfile==28||g_weaponMechanicProfile==9||g_weaponMechanicProfile==21;}
bool RifleActionProfile236(){return RifleProfile(g_weaponMechanicProfile)&&!SmgActionProfile230()&&CurrentRifle().emptyNeedsCharge;}
float SmgHintStrength230(float value){return std::isfinite(value)?(std::max)(.25f,(std::min)(3.f,value)):1.f;}
void Load1911Tint49(const wchar_t* explicitPath=nullptr){
 if(g_1911TintLoaded49&&!explicitPath)return;wchar_t path[MAX_PATH];ManualSettingsPath(path);const auto* p=explicitPath?explicitPath:path;
 g_reloadActionGuide227=GetPrivateProfileIntW(L"ReloadHints",L"ActionGuide",1,p)!=0;
 g_reloadActionColour227=(std::max)(0,(std::min)(4,int(GetPrivateProfileIntW(L"ReloadHints",L"ActionColour",0,p))));g_1911TintLoaded49=true;
 wchar_t strength[64],legacy[64];GetPrivateProfileStringW(L"ReloadHints",L"SmgStrength",L"1",legacy,64,p);GetPrivateProfileStringW(L"ReloadHints",L"ActionStrength",legacy,strength,64,p);g_smgHintStrength230=SmgHintStrength230(float(wcstod(strength,nullptr)));
}
bool Save1911Tint49(const wchar_t* path){
 const wchar_t* values[]={L"0",L"1",L"2",L"3",L"4"};
 wchar_t strength[32];swprintf_s(strength,L"%.3f",SmgHintStrength230(g_smgHintStrength230));
 return WritePrivateProfileStringW(L"ReloadHints",L"ActionGuide",g_reloadActionGuide227?L"1":L"0",path)!=0&&
 WritePrivateProfileStringW(L"ReloadHints",L"ActionColour",values[(std::max)(0,(std::min)(4,g_reloadActionColour227))],path)!=0&&
 WritePrivateProfileStringW(L"ReloadHints",L"ActionStrength",strength,path)!=0;
}
#ifdef FC3_AMMO_TEST
ImVec2 g_actionCheckPos227={},g_actionComboPos227={};
ImVec2 g_smgStrengthPos230={};
void MenuTestPoint(ImVec2&);
#endif
void ReloadActionGuideMenu227(const wchar_t* path,bool smg=false){
 Load1911Tint49();const char* colours[]={"Yellow","Red","Green","Cyan","White"};
 bool changed=ImGui::Checkbox("Reload action colour guide",&g_reloadActionGuide227);
#ifdef FC3_AMMO_TEST
 MenuTestPoint(g_actionCheckPos227);
#endif
 ImGui::SameLine();ImGui::SetNextItemWidth(110*g_menuScale);
 changed=ImGui::Combo("Colour##ActionGuide",&g_reloadActionColour227,colours,5)||changed;
#ifdef FC3_AMMO_TEST
 MenuTestPoint(g_actionComboPos227);
#endif
 {
  ImGui::SetNextItemWidth(190*g_menuScale);changed=ImGui::SliderFloat("Action guide strength",&g_smgHintStrength230,.25f,3.f,"%.2fx")||changed;
#ifdef FC3_AMMO_TEST
  g_smgStrengthPos230=ImVec2(ImGui::GetItemRectMin().x+95*g_menuScale,(ImGui::GetItemRectMin().y+ImGui::GetItemRectMax().y)*.5f);
#endif
 }
 static bool failed=false;if(changed){wchar_t p[MAX_PATH];ManualSettingsPath(p);failed=!Save1911Tint49(path?path:p);}
 if(failed)ImGui::TextDisabled("Colour guide could not be saved");
}
int ReloadActionRequired227(){
 if(!g_reloadActionGuide227||g_pistolMenu.open||fc3pad::GameplayInputBlocked()||!MotionInteractionEnabled())return 0;
 if(g_weaponMechanicProfile==15||g_weaponMechanicProfile==16||g_weaponMechanicProfile==18)return SpecialActionRequired253();
 if(RevolverManual()){
  const auto& r=g_revolver;
  return r.open&&!r.empty&&!r.pending&&!r.carrying&&!r.dropping&&r.clip<0&&RevolverLoadedRounds()>0?2:0;
 }
 if(g_handLeftValid&&g_handRightValid&&fc3pad::Latest().active){
  if(g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6&&g_pumpEnabled&&!g_chambered&&!ReloadPoseActive()&&!g_reload.held&&!g_manualReload.pending&&!g_manualReload.cycle)return 4;
  if(Lever1887Active()&&!g_lever.simple&&!g_lever.chambered&&g_lever.clip<0&&!g_lever.pending&&!g_lever.shell)return 5;
  if(R700Active()&&!g_r700.chambered&&g_r700.clip<0&&!g_r700.native&&!g_pistolReload.held&&!g_pistolReload.removalHeld&&!g_pistolReload.pending&&!g_pistolReload.cycle&&!g_pistolReload.rackReturning&&!g_pistolReload.slideClosing)return 7;
  if(SpasManual()&&g_spas.needPump&&!g_spas.carrying&&!g_spas.pending&&!SpasClipPlaying())return 6;
 }
 if(R700Model())return 0;
 const bool smg=SmgActionProfile230(),assault=RifleActionProfile236();
 if(!smg&&!assault&&g_weaponMechanicProfile!=20&&g_weaponMechanicProfile!=1&&g_weaponMechanicProfile!=12)return 0;
 if(!PistolHandling())return 0;const auto& p=g_pistolReload;
 return p.magazinePresent&&!p.held&&!p.pending&&!p.cycle&&!p.slideClosing&&!p.rackReturning&&p.needsRack&&!p.chamberReady?(smg||assault?3:1):0;
}
struct ActionTintLayout227 {UINT size=0,slot=0,count=0,offset[2]={};};
const GUID kActionTintLayout227={0xb395bd8a,0x31bf,0x4c25,{0x91,0xbc,0x46,0x42,0x55,0x72,0x27,0x01}};
bool ActionMaterialLayout227(ID3D11PixelShader* shader,ActionTintLayout227& layout){
 if(!shader)return false;UINT bytes=sizeof(layout);
 if(SUCCEEDED(shader->GetPrivateData(kActionTintLayout227,&bytes,&layout))&&bytes==sizeof(layout))return layout.count>0;
 layout={};UINT length=0;
 if(SUCCEEDED(shader->GetPrivateData(kEjectionBytecodeGuid,&length,nullptr))&&length&&length<=65536){
  std::vector<BYTE> code(length);ID3D11ShaderReflection* reflection=nullptr;
  if(SUCCEEDED(shader->GetPrivateData(kEjectionBytecodeGuid,&length,code.data()))&&SUCCEEDED(D3DReflect(code.data(),length,IID_ID3D11ShaderReflection,reinterpret_cast<void**>(&reflection)))){
   D3D11_SHADER_DESC desc={};reflection->GetDesc(&desc);
   for(UINT n=0;n<desc.ConstantBuffers&&!layout.count;++n){
    auto* cb=reflection->GetConstantBufferByIndex(n);D3D11_SHADER_BUFFER_DESC buffer={};D3D11_SHADER_INPUT_BIND_DESC binding={};
    if(FAILED(cb->GetDesc(&buffer))||!buffer.Size||buffer.Size>1024||buffer.Size%16||FAILED(reflection->GetResourceBindingDescByName(buffer.Name,&binding))||binding.Type!=D3D_SIT_CBUFFER||binding.BindPoint>=14)continue;
    ActionTintLayout227 candidate;candidate.size=buffer.Size;candidate.slot=binding.BindPoint;
    for(const char* name:{"_DiffuseColor1","_DiffuseColor2","_DiffuseColor","_BaseDiffuseColor","_DetailDiffuseColor"}){
     D3D11_SHADER_VARIABLE_DESC v={};auto* variable=cb->GetVariableByName(name);D3D11_SHADER_TYPE_DESC type={};
     if(candidate.count<2&&SUCCEEDED(variable->GetDesc(&v))&&SUCCEEDED(variable->GetType()->GetDesc(&type))&&v.Size>=12&&v.StartOffset%4==0&&v.StartOffset+12<=buffer.Size&&type.Type==D3D_SVT_FLOAT&&type.Class==D3D_SVC_VECTOR&&type.Columns>=3&&(v.uFlags&D3D_SVF_USED))candidate.offset[candidate.count++]=v.StartOffset/4;
    }
    if(candidate.count)layout=candidate;
   }
   SafeRelease(reflection);
  }
 }
 // Captured native Metal PS has this independently checked layout even when
 // its bytecode metadata is unavailable. Never guess a layout for another PS.
 if(!layout.count&&ShaderHash(shader)==0x7496D2BA1AA0B813ull){layout.size=160;layout.count=2;layout.offset[0]=0;layout.offset[1]=4;}
 shader->SetPrivateData(kActionTintLayout227,sizeof(layout),&layout);return layout.count>0;
}
bool TintActionMaterial227(const float* source,float* tinted,const ActionTintLayout227& layout,ULONGLONG now){
 if(!layout.count||layout.count>2||layout.size>1024)return false;
 for(UINT n=0;n<layout.count;++n)for(int c=0;c<3;++c)if(!std::isfinite(source[layout.offset[n]+c])||source[layout.offset[n]+c]<0)return false;
 memcpy(tinted,source,layout.size);
 const float colours[5][3]={{1,.85f,.03f},{1,.04f,.02f},{.08f,1,.14f},{.03f,.85f,1},{1,1,1}};
 const auto* colour=colours[(std::max)(0,(std::min)(4,g_reloadActionColour227))];
 const float pulse=.25f+.65f*float(.5-.5*std::cos(double(now%1400)*6.283185307179586/1400));
 const float amount=(std::min)(1.f,pulse*SmgHintStrength230(g_smgHintStrength230));
 for(UINT n=0;n<layout.count;++n)for(int c=0;c<3;++c){const UINT i=layout.offset[n]+c;tinted[i]=source[i]*(1-amount)+colour[c]*amount;}
 return true;
}
ID3D11Buffer* g_1911TintCb49=nullptr;ID3D11Device* g_1911TintDevice49=nullptr;UINT g_actionTintBytes227=0;
ID3D11DeviceContext* g_mp5HintContext229=nullptr;LONG g_mp5HintFrame229=-1;UINT g_mp5HintStart229=0,g_mp5HintCount229=0;ULONGLONG g_mp5HintAt229=0;
LONG g_smgHintProfile230=-1;
ID3D11Buffer* BindActionMaterial229(ID3D11DeviceContext* context,ULONGLONG pulseAt=~0ULL){
 ID3D11PixelShader* shader=nullptr;context->PSGetShader(&shader,nullptr,nullptr);ActionTintLayout227 layout;const bool valid=ActionMaterialLayout227(shader,layout);
 if(g_weaponMechanicProfile==21){static unsigned long long seen[8]={};static UINT count=0;const auto hash=ShaderHash(shader);bool known=false;for(UINT i=0;i<count;++i)if(seen[i]==hash)known=true;
  if(!known&&count<8){seen[count++]=hash;Log("[ACTION231] Vector charging material PS %016llX supported=%d bytes=%u slot=%u colours=%u; exact handle triangle run 3885/858",hash,valid,layout.size,layout.slot,layout.count);}
 }
 SafeRelease(shader);if(!valid)return nullptr;
 ID3D11Buffer* original=nullptr;context->PSGetConstantBuffers(layout.slot,1,&original);D3D11_BUFFER_DESC desc={};if(original)original->GetDesc(&desc);float source[256]={},tinted[256]={};
 if(desc.ByteWidth!=layout.size||ReadCopy(original,source,layout.size)!=layout.size||!TintActionMaterial227(source,tinted,layout,pulseAt==~0ULL?GetTickCount64():pulseAt)){SafeRelease(original);return nullptr;}
 ID3D11Device* device=nullptr;context->GetDevice(&device);if(g_1911TintDevice49!=device||g_actionTintBytes227!=layout.size){SafeRelease(g_1911TintCb49);g_1911TintDevice49=nullptr;}
 if(!g_1911TintCb49){D3D11_BUFFER_DESC cb={};cb.ByteWidth=layout.size;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(FAILED(device->CreateBuffer(&cb,nullptr,&g_1911TintCb49))){SafeRelease(device);SafeRelease(original);return nullptr;}g_1911TintDevice49=device;g_actionTintBytes227=layout.size;}SafeRelease(device);
 context->UpdateSubresource(g_1911TintCb49,0,nullptr,tinted,0,0);context->PSSetConstantBuffers(layout.slot,1,&g_1911TintCb49);
 // Binding slot travels with this returned reference through the stereo draw.
 original->SetPrivateData(kActionTintLayout227,sizeof(layout.slot),&layout.slot);return original;
}
ID3D11Buffer* Begin1911Tint49(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 g_mp5HintContext229=nullptr;
 // This hook sees every stereo scene draw. Reject cameras and exact draw
 // ranges before querying any mechanism state/native ammo authority.
 if(variant!=1||call.kind!=0||context!=g_ctx)return nullptr;
 const int special=SpecialActionDraw253(call);
 const bool cylinder=g_weaponMechanicProfile==42&&call.b==9444&&call.a==2625;
 const bool slide=(g_weaponMechanicProfile==20||g_weaponMechanicProfile==1||g_weaponMechanicProfile==12)&&PistolMeshSlide(call.b,call.a);
 const bool rigidSmg=(g_weaponMechanicProfile==39&&call.b==1548&&call.a==468)||(g_weaponMechanicProfile==7&&call.b==21066&&call.a==528)||(g_weaponMechanicProfile==9&&call.b==23511&&call.a==519);
 const bool mp5=g_weaponMechanicProfile==28&&((call.b==0&&call.a==4149)||(call.b==12234&&call.a==15534));
 const bool vector=g_weaponMechanicProfile==21&&call.b==3615&&call.a==10623;
 const bool bolt=R700Model()&&(RiflePart(CurrentRifle().part[0],call.b,call.a)||(Bolt().partFollowsBolt&&RiflePart(CurrentRifle().part[1],call.b,call.a)));
 const bool rifleHandle=RifleActionProfile236()&&!CurrentRifle().skinned&&RiflePart(CurrentRifle().part[0],call.b,call.a);
 const bool ms16=g_weaponMechanicProfile==25&&call.b==11028&&call.a==13881;
 const bool pump=g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6&&((call.b==4557&&call.a==5076)||(call.b==9633&&call.a==6795));
 const bool lever=g_weaponMechanicProfile==0&&call.b==13626&&call.a==7170;
 const bool spas=g_weaponMechanicProfile==34&&call.b==12699&&call.a==1494;
 if(!special&&!cylinder&&!slide&&!rigidSmg&&!mp5&&!vector&&!pump&&!lever&&!spas&&!rifleHandle&&!ms16&&!bolt)return nullptr;
 Load1911Tint49();if(!g_reloadActionGuide227)return nullptr;
 const auto d=NativeWeaponGeometry(context,call);
 if(special){if(!SpecialActionGeometry253(special,d))return nullptr;}
 else if(slide){if(!PistolMeshMain(d))return nullptr;}
 else if(rigidSmg){if(g_weaponMechanicProfile==39){if(d.vertexBytes!=157984||d.indexBytes!=35322||d.stride!=32||d.offset||d.format!=57)return nullptr;}else if(!RifleMain(CurrentRifle(),d))return nullptr;}
 else if(mp5||vector||rifleHandle||ms16||bolt){if(!RifleMain(CurrentRifle(),d))return nullptr;}
 else if(pump){if(d.vertexBytes!=257896||d.indexBytes!=47478||d.stride!=40||d.offset!=336||d.format!=57)return nullptr;}
 else if(lever){if(d.vertexBytes!=217840||d.indexBytes!=47196||d.stride!=32||d.offset!=400||d.format!=57)return nullptr;}
 else if(spas){if(d.vertexBytes!=127600||d.stride!=32||d.offset!=240||d.format!=57)return nullptr;}
 else if(d.vertexBytes!=186104||d.indexBytes!=31002||d.format!=57||d.stride!=40||d.offset!=224)return nullptr;
 const int action=ReloadActionRequired227();if(special){if(action!=special&&!(special==8&&action==9))return nullptr;}else if(action!=(bolt?7:pump?4:lever?5:spas?6:cylinder?2:(rigidSmg||mp5||vector||rifleHandle||ms16||bolt)?3:1))return nullptr;
 if(special){g_mp5HintContext229=context;g_mp5HintFrame229=g_frame;g_mp5HintStart229=call.b;g_mp5HintCount229=call.a;g_mp5HintAt229=GetTickCount64();g_smgHintProfile230=g_weaponMechanicProfile;return nullptr;}
 if(mp5||vector||pump||rifleHandle||ms16||bolt){g_mp5HintContext229=context;g_mp5HintFrame229=g_frame;g_mp5HintStart229=call.b;g_mp5HintCount229=call.a;g_mp5HintAt229=GetTickCount64();g_smgHintProfile230=g_weaponMechanicProfile;return nullptr;}
 return BindActionMaterial229(context);
}
void End1911Tint49(ID3D11DeviceContext* context,ID3D11Buffer*& original){g_mp5HintContext229=nullptr;if(original){UINT slot=0,bytes=sizeof(slot);original->GetPrivateData(kActionTintLayout227,&bytes,&slot);context->PSSetConstantBuffers(slot,1,&original);SafeRelease(original);}}

// Asset-verified triangles weighted exclusively to MP5 HANDLE bone12 (palette2)
// or Vector SLIDE_HANDLE bone12 (palette4, kriss.xbg section1).
// Split the native draw in order; no overlay, duplicate triangles or new buffers.
bool IssueMp5Hint229(ID3D11DeviceContext* context,const DrawCall& call){
 if(g_mp5HintContext229!=context||g_mp5HintFrame229!=g_frame||g_smgHintProfile230!=g_weaponMechanicProfile||call.kind||call.b!=g_mp5HintStart229||call.a!=g_mp5HintCount229)return false;
 ID3D11Buffer* original=BindActionMaterial229(context,g_mp5HintAt229);if(!original)return false;
 UINT slot=0,bytes=sizeof(slot);original->GetPrivateData(kActionTintLayout227,&bytes,&slot);
 const UINT mp5Runs[3][2]={{1647,78},{12240,288},{21507,192}},vectorRuns[1][2]={{3885,858}},pumpRuns[3][2]={{4557,468},{6813,672},{10959,219}};
 const bool pump=g_smgHintProfile230>=3&&g_smgHintProfile230<=6;
 // MS16 BOLTSLIDE bone6/palette0: native xbg indices 11034..12017,
 // all 328 triangles exclusively weighted to the charging handle.
 const UINT ms16Runs[1][2]={{11034,984}},rigidRifleRun[1][2]={{call.b,call.a}};
 const bool assault=RifleActionProfile236()||R700Model();
 // Asset-verified exclusive CRANK/HYDRAULIC and HANDLE palette weights;
 // the remaining triangles in those shared draws retain their native material.
 const UINT flamerCrank[2][2]={{3996,690},{9699,276}},flamerHandle[2][2]={{20838,1512},{27759,804}};
 const bool special=g_smgHintProfile230==15||g_smgHintProfile230==16||g_smgHintProfile230==18;
 const bool flamerLever=g_smgHintProfile230==15&&(call.b==3630||call.b==20838);
 const UINT (*runs)[2]=special?(flamerLever?(call.b==3630?flamerCrank:flamerHandle):rigidRifleRun):pump?pumpRuns:assault?(g_smgHintProfile230==25?ms16Runs:rigidRifleRun):g_smgHintProfile230==21?vectorRuns:mp5Runs;const int count=flamerLever?2:special||assault||g_smgHintProfile230==21?1:3;UINT cursor=call.b;
 for(int n=0;n<count;++n){const auto& run=runs[n];if(run[0]<call.b||run[0]+run[1]>call.b+call.a)continue;
  if(run[0]>cursor){context->PSSetConstantBuffers(slot,1,&original);DrawCall part=call;part.b=cursor;part.a=run[0]-cursor;RawIssue(context,part);}
  context->PSSetConstantBuffers(slot,1,&g_1911TintCb49);DrawCall part=call;part.b=run[0];part.a=run[1];{VectorDiffuseCue231 visible(context,g_smgHintProfile230==21||assault||special);RawIssue(context,part);}cursor=run[0]+run[1];
 }
 context->PSSetConstantBuffers(slot,1,&original);
 if(cursor<call.b+call.a){DrawCall part=call;part.b=cursor;part.a=call.b+call.a-cursor;RawIssue(context,part);}
 SafeRelease(original);return true;
}
