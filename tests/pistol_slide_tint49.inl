#include "special_action253_fixture.inl"
void TestSlideTint49(ID3D11Device* d,ID3D11DeviceContext* c){
 const LONG oldFrame=g_frame,oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;auto* oldContext=g_ctx;
 const bool oldEnabled=g_reloadActionGuide227,oldLoaded=g_1911TintLoaded49;const int oldColour=g_reloadActionColour227;const auto oldClock=g_fixtureClock171;const float oldStrength=g_smgHintStrength230;
 const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;const auto oldRevolver=g_revolver;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid;
 PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 NativeAmmoFixture219 ammo;ammo.count()=6;
 g_ctx=c;g_reloadActionGuide227=g_1911TintLoaded49=true;g_pistolMenu.open=false;g_fixtureClock171=700;
 g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
 c->ClearState();ID3D11Buffer* material=Buffer(d,160,D3D11_BIND_CONSTANT_BUFFER);
 float source[40];for(int i=0;i<40;++i)source[i]=.65f;source[3]=source[7]=.8f;for(int i=8;i<40;++i)source[i]=float(i)/40;
 material->SetPrivateData(kCbCopyGuid,sizeof(source),source);c->UpdateSubresource(material,0,nullptr,source,0,0);c->PSSetConstantBuffers(0,1,&material);
 auto* code=Native1911Compile48("cbuffer Metal:register(b0){float4 _DiffuseColor2;float4 _DiffuseColor1;float4 other[8];};float4 main():SV_Target{return (_DiffuseColor2+_DiffuseColor1)*.5;}","ps_5_0");
 ID3D11PixelShader* ps=nullptr;Check(code&&SUCCEEDED(d->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&ps)),"227 actual material shader");
 ps->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());SafeRelease(code);unsigned long long hash=123;ps->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);c->PSSetShader(ps,nullptr,0);
 ActionTintLayout227 reflected;Check(ActionMaterialLayout227(ps,reflected)&&reflected.size==160&&reflected.slot==0&&reflected.count==2,"227 reflected material names/types/offsets instead of assuming all weapon shaders equal");
 code=Native1911Compile48("float4 main(uint id:SV_VertexID):SV_Position{float2 p=float2((id<<1)&2,id&2);return float4(p*float2(2,-2)+float2(-1,1),0,1);}","vs_5_0");ID3D11VertexShader* vs=nullptr;
 Check(code&&SUCCEEDED(d->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs)),"227 GPU vertex fixture");SafeRelease(code);c->VSSetShader(vs,nullptr,0);c->IASetInputLayout(nullptr);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=8;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
 ID3D11Texture2D* target=nullptr;d->CreateTexture2D(&td,nullptr,&target);ID3D11RenderTargetView* rt=nullptr;d->CreateRenderTargetView(target,nullptr,&rt);c->OMSetRenderTargets(1,&rt,nullptr);D3D11_VIEWPORT vp={0,0,8,8,0,1};c->RSSetViewports(1,&vp);
 td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* staging=nullptr;d->CreateTexture2D(&td,nullptr,&staging);
 for(int profile:{20,1,12,42}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;
  auto& p=g_pistolReload;p={};p.enabled=p.magazinePresent=p.needsRack=true;p.chamberReady=false;
  g_revolver={};g_revolver.open=true;g_revolver.ammoState=ammo.state;g_revolver.ammoWrapper=ammo.wrapper;
  const UINT vbytes=profile==42?186104:CurrentPistolMesh().vb,ibytes=profile==42?31002:CurrentPistolMesh().ib;
  ID3D11Buffer* vb=Buffer(d,vbytes,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(d,ibytes,D3D11_BIND_INDEX_BUFFER);UINT stride=profile==42?40:32,offset=profile==42?224:CurrentPistolMesh().offset;
  c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
  DrawCall call={};call.a=profile==42?2625:CurrentPistolMesh().slide[0][1];call.b=profile==42?9444:CurrentPistolMesh().slide[0][0];
  for(int colour=0;colour<5;++colour){
   g_reloadActionColour227=colour;ID3D11Buffer* original=Begin1911Tint49(c,call,1);Check(original==material,"227 all four exact handgun mechanisms receive required-action cue");
   ID3D11Buffer* tint=nullptr;c->PSGetConstantBuffers(0,1,&tint);float result[40]={},expected[40]={};Read(d,c,tint,result);SafeRelease(tint);
   Check(TintActionMaterial227(source,expected,reflected,g_fixtureClock171),"227 pulse reference");Compare(result,expected,40,"227 actual GPU material equals selected pulse");
   for(int i=0;i<40;++i)if(i!=0&&i!=1&&i!=2&&i!=4&&i!=5&&i!=6)Check(result[i]==source[i],"227 preserves alpha, normals, gloss, masks and all noncolour material values");
   c->Draw(3,0);c->CopyResource(staging,target);D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(c->Map(staging,0,D3D11_MAP_READ,0,&mapped)),"227 colour pixel readback");
   const BYTE* pixel=static_cast<BYTE*>(mapped.pData)+4*mapped.RowPitch+16;
   for(int channel=0;channel<4;++channel)Check(abs(int(pixel[channel])-int(expected[channel]*255+.5f))<=1,"227 each preset reaches actual rendered GPU colour with native alpha");c->Unmap(staging,0);
   End1911Tint49(c,original);c->PSGetConstantBuffers(0,1,&tint);Check(tint==material&&!original,"227 restores binding after stereo scope");SafeRelease(tint);Read(d,c,material,result);Compare(result,source,40,"227 source GPU material unchanged");
  }
  g_reloadActionGuide227=false;Check(!Begin1911Tint49(c,call,1),"227 toggle off restores normal material immediately");g_reloadActionGuide227=true;
  Check(!Begin1911Tint49(c,call,0)&&!Begin1911Tint49(c,call,2),"227 never changes world, shadow or UI camera variants");
  DrawCall receiver=call;receiver.a=profile==42?9432:CurrentPistolMesh().frame[0][1];receiver.b=profile==42?12:CurrentPistolMesh().frame[0][0];Check(!Begin1911Tint49(c,receiver,1),"227 never tints receiver or mixed receiver/crane range");
  if(profile!=42){
   for(const auto& part:CurrentPistolMesh().slide){call.a=part[1];call.b=part[0];ID3D11Buffer* original=Begin1911Tint49(c,call,1);Check(original==material,"227 both main slide pieces qualify");End1911Tint49(c,original);}
   p.needsRack=false;p.chamberReady=true;Check(!Begin1911Tint49(c,call,1),"227 completed/ready and automatic partial reloads have no permanent tint");p.needsRack=true;p.chamberReady=false;
   p.pending=true;Check(!ReloadActionRequired227(),"227 pending seating is not a slide instruction");p.pending=false;p.cycle=true;Check(!ReloadActionRequired227(),"227 animated reload does not ask for a blocked slide");p.cycle=false;
   p.slideClosing=true;Check(!ReloadActionRequired227(),"227 automatic closure does not request a manual pull");p.slideClosing=false;p.rackReturning=true;Check(!ReloadActionRequired227(),"227 completed physical release ends the cue");p.rackReturning=false;
   p.magazinePresent=false;Check(!ReloadActionRequired227(),"227 no slide instruction while magazine missing");p.magazinePresent=true;p.enabled=false;Check(!ReloadActionRequired227(),"227 Simple mode excluded");p.enabled=true;
  }else{
   g_revolver.open=false;Check(!ReloadActionRequired227(),"227 cylinder closure ends cue");g_revolver.open=true;g_revolver.empty=true;Check(!ReloadActionRequired227(),"227 empty cylinder asks for ammo, not closure");g_revolver.empty=false;g_revolver.pending=true;Check(!ReloadActionRequired227(),"227 fill pending excluded");g_revolver.pending=false;g_revolver.simple=true;Check(!ReloadActionRequired227(),"227 Simple revolver excluded");g_revolver.simple=false;
  }
  g_pistolMenu.open=true;Check(!ReloadActionRequired227(),"227 Insert preview pauses gameplay cue");g_pistolMenu.open=false;
  g_handLeftValid=false;Check(!ReloadActionRequired227(),"227 tracking loss suppresses stale instruction");g_handLeftValid=true;
  vb->Release();ib->Release();
 }
 special253test::Test(d,c,material,ammo);
 g_weaponMechanicProfile=8;g_weaponMechanicFrame=g_frame;Check(!ReloadActionRequired227(),"227 other weapon families unchanged");
 float low[40]={},high[40]={};g_reloadActionColour227=0;TintActionMaterial227(source,low,reflected,0);TintActionMaterial227(source,high,reflected,700);Check(high[0]>low[0]&&high[2]<low[2],"227 slow pulse changes intensity, not permanent colour");
 wchar_t folder[MAX_PATH],path[MAX_PATH],value[64];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"a27",0,path);
 WritePrivateProfileStringW(L"PistolReload",L"PortWidth",L"0.12345678",path);WritePrivateProfileStringW(L"ReloadHints",L"Red1911Slide",L"1",path);
 Load1911Tint49(path);Check(g_reloadActionGuide227&&g_reloadActionColour227==0,"227 new defaults yellow/on ignore obsolete always-red setting");
 for(int colour=0;colour<5;++colour){g_reloadActionColour227=colour;g_reloadActionGuide227=colour%2!=0;Check(Save1911Tint49(path),"227 independent settings save");g_reloadActionColour227=-1;g_reloadActionGuide227=true;Load1911Tint49(path);Check(g_reloadActionColour227==colour&&g_reloadActionGuide227==(colour%2!=0),"227 all presets and toggle persist");}
 WritePrivateProfileStringW(L"ReloadHints",L"ActionColour",L"99",path);Load1911Tint49(path);Check(g_reloadActionColour227==4,"227 invalid preset clamps safely");GetPrivateProfileStringW(L"PistolReload",L"PortWidth",L"",value,64,path);Check(wcscmp(value,L"0.12345678")==0,"227 settings never rewrite calibration");DeleteFileW(path);
 GetTempFileNameW(folder,L"s30",0,path);g_smgHintStrength230=2.125f;Check(Save1911Tint49(path),"230 SMG strength save");g_smgHintStrength230=1;Load1911Tint49(path);Check(g_smgHintStrength230==2.125f,"230 shared SMG strength survives restart");
 for(const wchar_t* invalid:{L"nan",L"inf",L"-4",L"100"}){WritePrivateProfileStringW(L"ReloadHints",L"ActionStrength",invalid,path);Load1911Tint49(path);Check(std::isfinite(g_smgHintStrength230)&&g_smgHintStrength230>=.25f&&g_smgHintStrength230<=3,"231 invalid global strength bounded");}DeleteFileW(path);
 c->ClearState();SafeRelease(material);SafeRelease(ps);SafeRelease(vs);SafeRelease(rt);SafeRelease(target);SafeRelease(staging);SafeRelease(g_1911TintCb49);g_1911TintDevice49=nullptr;g_actionTintBytes227=0;
 memcpy(g_magazineStates,oldStates,sizeof(oldStates));g_revolver=oldRevolver;g_ctx=oldContext;g_frame=oldFrame;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_fixtureClock171=oldClock;
 g_reloadActionGuide227=oldEnabled;g_1911TintLoaded49=oldLoaded;g_reloadActionColour227=oldColour;g_smgHintStrength230=oldStrength;g_pistolMenu=oldMenu;g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;fc3pad::Publish(oldControls);
 puts("PASS: action227 four handgun mechanism meshes, five real GPU colour presets, slow state-driven pulse/stop, reflected material layout, unchanged noncolour/source data, stereo binding restoration, ready/pending/auto/Simple/tracking guards and independent persistence");
}
