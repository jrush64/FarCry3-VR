namespace special253test {
ID3D11Buffer* material=nullptr;UINT total=0,tinted=0,next=0;
void STDMETHODCALLTYPE Draw(ID3D11DeviceContext* ctx,UINT count,UINT start,INT base){
 Check(start==next&&base==0,"253 split retains every triangle exactly once and native order");next+=count;total+=count;
 ID3D11Buffer* cb=nullptr;ctx->PSGetConstantBuffers(0,1,&cb);const bool cue=cb==g_1911TintCb49;
 Check(cue||cb==material,"253 native material restored on non-mechanism pieces");
 if(cue){if(g_weaponMechanicProfile==15)Check((start==3996&&count==690)||(start==9699&&count==276)||(start==20838&&count==1512)||(start==27759&&count==804)||(start==13590&&count==7248),"253 only verified flame crank/handle/tank triangles pulse");tinted+=count;}SafeRelease(cb);
}
void Test(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Buffer* source,NativeAmmoFixture219& ammo){
 const auto flame=g_flamer;const auto flare=g_flare;const auto gm=g_gm94;const auto clip=g_flamerClip;const auto gc=g_gm94Clip;
 const auto draw=g_oDrawIndexed;g_oDrawIndexed=Draw;material=source;
 auto mesh=[&](int profile,UINT vbBytes,UINT ibBytes,UINT stride,UINT offset,UINT start,UINT count,UINT expected){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame;
  auto* vb=Buffer(d,vbBytes,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(d,ibBytes,D3D11_BIND_INDEX_BUFFER);
  c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);DrawCall call={};call.b=start;call.a=count;
  total=tinted=0;for(int eye=0;eye<2;++eye){next=start;auto* original=Begin1911Tint49(c,call,1);Check(!original&&IssueMp5Hint229(c,call),"253 special mechanism uses shared stereo pulse path");End1911Tint49(c,original);}
  Check(total==count*2&&tinted==expected*2,"253 both eyes tint exact target without receiver tint or duplicate geometry");
  Check(!Begin1911Tint49(c,call,0)&&!Begin1911Tint49(c,call,2),"253 special cue rejects world/shadow/UI cameras");
  vb->Release();ib->Release();
 };
 g_weaponMechanicProfile=15;g_weaponMechanicFrame=g_frame;g_flamer={};g_flamerClip={};g_flamer.frameFrame=g_frame;g_flamer.ammoState=ammo.state;g_flamer.ammoWrapper=ammo.wrapper;ammo.count()=0;
 Check(ReloadActionRequired227()==8,"253 empty fuel starts lever pulse immediately");
 mesh(15,349160,68436,40,0,3630,8004,966);mesh(15,349160,68436,40,0,20838,13380,2316);
 g_flamer.leverDown=true;g_flamer.lever=1;Check(ReloadActionRequired227()==9,"253 open flame lever advances to canister guidance");
 mesh(15,349160,68436,40,0,13590,7248,7248);
 ammo.count()=100;g_flamer.replaced=true;Check(ReloadActionRequired227()==8,"253 fuel refill alone cannot stop cue before lever locks");
 g_flamer.leverDown=false;g_flamer.lever=0;Check(!ReloadActionRequired227(),"253 fully reloaded and locked flame stops cue");
 g_flamer.lever=.5;Check(ReloadActionRequired227()==8,"253 partly closed lever retains cue after release with fuel already loaded");g_flamer.lever=0;
 g_flamer.carrying=true;g_flamer.seated=false;Check(ReloadActionRequired227()==8,"253 interrupted flame reload retains guide");g_flamer.carrying=false;g_flamer.seated=true;
 g_weaponMechanicProfile=16;g_flare={};g_flare.frameFrame=g_frame;g_flare.ammoState=ammo.state;g_flare.ammoWrapper=ammo.wrapper;g_flare.chamber=2;ammo.count()=0;
 Check(ReloadActionRequired227()==10,"253 spent flare requests barrel opening");mesh(16,91584,18168,32,0,0,1590,1590);
 g_flare.open=true;Check(ReloadActionRequired227()==11,"253 open spent flare guides shell extraction/insertion");mesh(16,91584,18168,32,0,8616,138,138);
 g_flare.chamber=1;ammo.count()=1;Check(ReloadActionRequired227()==10,"253 loaded flare still guides closing");g_flare.open=false;Check(!ReloadActionRequired227(),"253 closed loaded flare stops cue");
 g_weaponMechanicProfile=18;g_gm94={};g_gm94Clip={};g_gm94.frameFrame=g_frame;g_gm94.ammoState=ammo.state;g_gm94.ammoWrapper=ammo.wrapper;g_gm94.spent=true;
 Check(ReloadActionRequired227()==13,"253 GM94 post-shot pump cue");mesh(18,243072,40776,32,640,9900,2952,2952);
 g_gm94.spent=false;g_gm94.chambered=false;ammo.count()=0;Check(ReloadActionRequired227()==12,"253 empty clear launcher requests cover rather than endless pump");mesh(18,243072,40776,32,640,14820,1638,1638);
 g_gm94.coverOpen=true;ammo.count()=4;Check(ReloadActionRequired227()==12,"253 filled launcher still needs cover closed");g_gm94.coverOpen=false;Check(ReloadActionRequired227()==13,"253 closed loaded launcher still needs pump");g_gm94.chambered=true;Check(!ReloadActionRequired227(),"253 ready launcher stops cue");
 g_handLeftValid=false;Check(!ReloadActionRequired227(),"253 tracking loss suppresses stale special guide");g_handLeftValid=true;
 g_flamer=flame;g_flare=flare;g_gm94=gm;g_flamerClip=clip;g_gm94Clip=gc;g_oDrawIndexed=draw;
 puts("PASS:253 flame empty-to-locked reload guide, flare open/shell/close and GM94 pump/cover states; exact native material splits across both eyes preserve every triangle and stop when ready");
}
}
