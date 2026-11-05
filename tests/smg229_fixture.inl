namespace smg229test {
UINT total=0,tinted=0,draws=0;ID3D11Buffer* source=nullptr;
void STDMETHODCALLTYPE Draw(ID3D11DeviceContext* context,UINT count,UINT start,INT base){
 Check(base==0,"229 MP5 base vertex preserved");ID3D11Buffer* cb=nullptr;context->PSGetConstantBuffers(0,1,&cb);
 bool on=cb==g_1911TintCb49;Check(on||cb==source,"229 MP5 retains original material on non-handle triangles");
 if(on){Check((g_weaponMechanicProfile==21&&start==3885&&count==858)||(g_weaponMechanicProfile==28&&((start==1647&&count==78)||(start==12240&&count==288)||(start==21507&&count==192))),"230 only asset-verified MP5/Vector handle runs receive tint");tinted+=count;}
 total+=count;++draws;SafeRelease(cb);
}
void Test(ID3D11Device* device,ID3D11DeviceContext* context){
 PistolReloadState states[20];memcpy(states,g_magazineStates,sizeof(states));PistolGuideSettings guides[20];memcpy(guides,g_magazineGuides,sizeof(guides));
 const LONG profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame;auto* ctx=g_ctx;const auto controls=fc3pad::Latest();const auto menu=g_pistolMenu;
 const bool motion=g_motionGun,guard=g_weaponMotionGuard,left=g_handLeftValid,right=g_handRightValid,two=g_twoHand,hint=g_reloadActionGuide227,loaded=g_1911TintLoaded49;
 const float strength=g_smgHintStrength230;g_smgHintStrength230=1;
 float hands[2][16];memcpy(hands,g_handBody,sizeof(hands));const auto support0=g_supportGrab214[7],support1=g_supportGrab214[28],support2=g_supportGrab214[9];
 g_ctx=context;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_twoHand=false;g_pistolMenu.open=false;g_reloadActionGuide227=g_1911TintLoaded49=true;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);const auto id=HandIdentity();for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=float(id.m[i]);
 for(int weapon:{7,28,9}){
  g_weaponMechanicProfile=weapon;g_weaponMechanicFrame=++g_frame;g_pistolReload={};g_pistolReload.enabled=true;g_supportGrab214[weapon].offset[0]=.03f;
  V3 normal={},palm={},preview={},expected={};Mat4 root={};Check(ControllerMagazineRoot124(root),"229 SMG support has controller weapon root");expected=Apply(Multiply(root,*CurrentRifle().support),{-.6781,.0519,1.0387});SupportGrabAdjustPoint214(expected);
  Check(GrabGizmoPoints({50,60,70},normal,palm,&id),"229 gameplay support contact available");
  g_pistolMenu.open=true;Check(GrabGizmoPoints({-50,-60,-70},preview,palm,&id),"229 Insert support contact available");g_pistolMenu.open=false;
  Check(Length(Sub(normal,preview))<1e-7&&Length(Sub(normal,expected))<1e-7,"229 gameplay and Insert share calibrated weapon-relative support point despite different native support poses");
 }
 NativeAmmoFixture219 ammo;ammo.count()=5;g_weaponMechanicProfile=39;g_weaponMechanicFrame=++g_frame;auto& p=g_pistolReload;p={};p.enabled=p.magazinePresent=true;p.rootBody=id;p.rootEpoch=g_armPoseEpoch;p.rootFrame=p.palmFrame=g_frame;p.palmBody=id;
 const Mat4 box=PistolGuideBox(p.rootBody,0);p.tip={box.m[3],box.m[7],box.m[11]};c.leftGrip=1;c.x=true;TickAkRemoval(c);
 Check(p.removalHeld&&p.magazinePresent,"229 Skorpion grip+X at magazine starts physical extraction");p.tip.z-=.04;p.tip.y-=.04;TickAkRemoval(c);Check(p.removalDetached&&!p.magazinePresent&&ammo.count()==5,"229 physical pull detaches without ammo write");
 c.leftGrip=0;TickAkRemoval(c);Check(!p.removalHeld&&p.dropActive&&ammo.count()==5,"229 grip release drops extracted Skorpion magazine");c.x=false;fc3pad::Publish(c);
 const auto begin=g_nativeReloadBegin;const auto end=g_nativeReloadEnd;const auto fill=g_nativeReload;const auto gate=g_reloadSignal;const auto reload=g_reload;const UINT hudOld=g_pistolFixtureHud;
 g_weaponMechanicProfile=28;g_weaponMechanicFrame=++g_frame;p={};g_pistolReload={};auto& mp=g_pistolReload;mp.enabled=mp.pending=true;mp.magazinePresent=false;mp.deadline=GetTickCount64()+3000;
 g_reloadSignal.enabled=g_reloadSignal.manualAk=true;g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=PistolFixtureTransfer;
 g_pistolFixtureHud=UINT(ammo.page)+16384;g_reload={};g_reload.widgets[0].wrapper=LONG(ammo.wrapper);g_reload.widgets[0].hud=LONG(g_pistolFixtureHud);
 const UINT hud[5]={0,8,0,100,100};memcpy(reinterpret_cast<void*>(g_pistolFixtureHud+0x134),hud,sizeof(hud));ammo.count()=0;
 ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(mp.committed&&mp.needsRack&&!mp.chamberReady&&PistolBusy(),"229 native MP5 empty seating remains fire locked until charging handle");
 Complete1911RackFixture50();Check(!mp.needsRack&&mp.chamberReady&&ammo.count()==8&&ReloadRead(g_pistolFixtureHud,0x140)==92,"229 actual MP5 controller stroke chambers without extra ammo/debit");
 g_nativeReloadBegin=begin;g_nativeReloadEnd=end;g_nativeReload=fill;g_reloadSignal=gate;g_reload=reload;g_pistolFixtureHud=hudOld;
 auto* material=Buffer(device,160,D3D11_BIND_CONSTANT_BUFFER);float values[40];for(float& x:values)x=.5f;material->SetPrivateData(kCbCopyGuid,sizeof(values),values);context->UpdateSubresource(material,0,nullptr,values,0,0);context->PSSetConstantBuffers(0,1,&material);
 auto* code=Native1911Compile48("float4 main():SV_Target{return 1;}","ps_5_0");ID3D11PixelShader* shader=nullptr;Check(SUCCEEDED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader)),"229 hint material fixture");code->Release();unsigned long long hash=0x7496D2BA1AA0B813ull;shader->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);context->PSSetShader(shader,nullptr,0);
 const auto nativeDraw=g_oDrawIndexed;g_oDrawIndexed=Draw;source=material;
 for(int weapon:{39,7,9,28,21}){
  g_weaponMechanicProfile=weapon;g_weaponMechanicFrame=++g_frame;auto& q=g_pistolReload;q={};q.enabled=q.magazinePresent=q.needsRack=true;q.chamberReady=false;
  UINT vbBytes=weapon==39?157984:CurrentRifle().vb,ibBytes=weapon==39?35322:CurrentRifle().ib,stride=(weapon==28||weapon==21)?40:32,offset=0;
  auto* vb=Buffer(device,vbBytes,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,ibBytes,D3D11_BIND_INDEX_BUFFER);context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
  DrawCall call={};call.b=weapon==39?1548:weapon==7?21066:weapon==9?23511:weapon==21?3615:0;call.a=weapon==39?468:weapon==7?528:weapon==9?519:weapon==21?10623:4149;
  Check(ReloadActionRequired227()==3,"229 all four SMGs publish required charging cue");
  if(weapon!=28&&weapon!=21){auto* original=Begin1911Tint49(context,call,1);Check(original==material,"229 exact rigid SMG charging mesh receives pulse");End1911Tint49(context,original);}
  else if(weapon==21){
   total=tinted=draws=0;
   for(int eye=0;eye<2;++eye){auto* original=Begin1911Tint49(context,call,1);Check(!original&&IssueMp5Hint229(context,call),"230 Vector shared draw splits while charging required");End1911Tint49(context,original);}
   Check(total==2*10623&&tinted==2*858&&draws==6,"230 both Vector eyes cover original triangles once and tint only SLIDE_HANDLE");
   DrawCall receiver=call;receiver.b=14238;receiver.a=3906;Check(!Begin1911Tint49(context,receiver,1)&&!IssueMp5Hint229(context,receiver),"230 Vector receiver-only draw is never split/tinted");
  }
  else{
   total=tinted=draws=0;
   for(int eye=0;eye<2;++eye)for(int part=0;part<2;++part){call.b=part?12234:0;call.a=part?15534:4149;auto* original=Begin1911Tint49(context,call,1);Check(!original&&IssueMp5Hint229(context,call),"229 native MP5 shared draw splits only while cue active");End1911Tint49(context,original);}
   Check(total==2*(4149+15534)&&tinted==2*(78+288+192)&&draws==16,"229 both eyes cover every original triangle once and tint only native handle triangles");
  }
  float weak[40]={},strong[40]={};g_smgHintStrength230=.25f;auto* original=BindActionMaterial229(context,700);Check(original==material,"230 strength uses real material binding");Read(device,context,g_1911TintCb49,weak);End1911Tint49(context,original);
  g_smgHintStrength230=3;original=BindActionMaterial229(context,700);Read(device,context,g_1911TintCb49,strong);End1911Tint49(context,original);
  Check(std::fabs(strong[0]-values[0])>std::fabs(weak[0]-values[0]),"230 strength visibly increases selected colour on each of five SMG GPU materials");
  for(int i=0;i<40;++i)if(i!=0&&i!=1&&i!=2&&i!=4&&i!=5&&i!=6)Check(strong[i]==values[i]&&weak[i]==values[i],"230 strength preserves alpha and noncolour material data");g_smgHintStrength230=1;
  q.needsRack=false;q.chamberReady=true;Check(!ReloadActionRequired227(),"229 SMG cue stops when charging completes");
  ID3D11Buffer* none=nullptr;context->IASetVertexBuffers(0,1,&none,&stride,&offset);
  vb->Release();ib->Release();
 }
 g_oDrawIndexed=nativeDraw;context->ClearState();shader->Release();material->Release();g_mp5HintContext229=nullptr;
 memcpy(g_magazineStates,states,sizeof(states));memcpy(g_magazineGuides,guides,sizeof(guides));g_supportGrab214[7]=support0;g_supportGrab214[28]=support1;g_supportGrab214[9]=support2;
 g_ctx=ctx;g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_pistolMenu=menu;g_motionGun=motion;g_weaponMotionGuard=guard;g_handLeftValid=left;g_handRightValid=right;g_twoHand=two;g_reloadActionGuide227=hint;g_1911TintLoaded49=loaded;memcpy(g_handBody,hands,sizeof(hands));fc3pad::Publish(controls);
 g_smgHintStrength230=strength;
 puts("PASS: smg229 gameplay/Insert support point consistency, physical Skorpion extraction, native MP5 empty-seating/controller-charge gates, four SMG action meshes, exact handle-only MP5 split coverage in both eyes and cue completion");
}
}
