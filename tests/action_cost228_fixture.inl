UINT g_actionAuthorityCalls228=0;
UINT __stdcall ActionAuthority228(UINT manager,UINT low,UINT high){++g_actionAuthorityCalls228;return SignalFixtureAuthority(manager,low,high);}
void TestActionCost228(ID3D11Device* device,ID3D11DeviceContext* context){
 const LONG profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame;auto* ctx=g_ctx;
 const auto revolver=g_revolver;const auto controls=fc3pad::Latest();const auto menu=g_pistolMenu;
 const bool enabled=g_reloadActionGuide227,loaded=g_1911TintLoaded49,motion=g_motionGun,guard=g_weaponMotionGuard,left=g_handLeftValid,right=g_handRightValid;
 NativeAmmoFixture219 ammo;ammo.count()=6;g_singleReloadAuthority=ActionAuthority228;
 g_ctx=context;g_weaponMechanicProfile=42;g_weaponMechanicFrame=++g_frame;g_reloadActionGuide227=g_1911TintLoaded49=true;
 g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 g_revolver={};g_revolver.open=true;g_revolver.ammoState=ammo.state;g_revolver.ammoWrapper=ammo.wrapper;
 g_actionAuthorityCalls228=0;Check(ReloadActionRequired227()==2&&g_actionAuthorityCalls228==1,"228 real open-cylinder context reaches native authority");
 g_actionAuthorityCalls228=0;
 DrawCall cylinder={};cylinder.a=2625;cylinder.b=9444;DrawCall scene={};scene.a=300;scene.b=12;
 for(int i=0;i<4096;++i){
  Check(!Begin1911Tint49(context,cylinder,0)&&!Begin1911Tint49(context,cylinder,2),"228 world/UI camera rejects before native query");
  Check(!Begin1911Tint49(context,scene,1),"228 unrelated weapon/scene range rejects before native query");
 }
 Check(!g_actionAuthorityCalls228,"228 twelve thousand unrelated draws incur zero native ammo/authority calls");
 // Even coincident cylinder index ranges require the actual weapon buffers.
 context->ClearState();ID3D11Buffer* vb=Buffer(device,4096,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,4096,D3D11_BIND_INDEX_BUFFER);UINT stride=40,offset=224;
 context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 for(int i=0;i<128;++i)Check(!Begin1911Tint49(context,cylinder,1),"228 coincident index range with other buffers excluded");
 Check(!g_actionAuthorityCalls228,"228 wrong geometry also incurs zero native ammo/authority calls");
 context->ClearState();vb->Release();ib->Release();
 g_ctx=ctx;g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_revolver=revolver;g_pistolMenu=menu;
 g_reloadActionGuide227=enabled;g_1911TintLoaded49=loaded;g_motionGun=motion;g_weaponMotionGuard=guard;g_handLeftValid=left;g_handRightValid=right;fc3pad::Publish(controls);
 puts("PASS: cost228 open loaded .44 remains eligible, 12288 world/UI/unrelated draws and 128 wrong-buffer draws make zero native ammo authority calls; actual mechanism colour is covered by action227 GPU tests");
}
