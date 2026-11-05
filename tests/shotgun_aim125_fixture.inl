// Run after the controller124 fixture in its isolated mode: baseline neutral
// wrist/grip is seeded and native walking animation has already been rejected.
void TestShotgunAim125(ID3D11Device* device,ID3D11DeviceContext* context)
{
    context->ClearState();g_dev=device;g_oUpdateSubresource=Update;g_aimActive=false;
    g_manualReloadEnabled=false;g_machete.held=false;g_twoHand=false;
    auto* previous=g_weaponPaletteCb;g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    auto* vb=Buffer(device,257896,D3D11_BIND_VERTEX_BUFFER);auto* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);auto* camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);auto* palette=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
    UINT stride=40,offset=336;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);ID3D11Buffer* cb[4]={globals,camera,nullptr,palette};context->VSSetConstantBuffers(0,4,cb);
    CameraSummary cam={};memcpy(cam.right,kM133Camera57,12);memcpy(cam.up,kM133Camera57+3,12);memcpy(cam.forward,kM133Camera57+6,12);memcpy(cam.position,kM133Camera57+9,12);cam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
    globals->SetPrivateData(kCbCopyGuid,sizeof(kM133GunGlobals57),kM133GunGlobals57);
    const Mat4 view=Multiply(HullView(cam),HullWorld(kM133GunGlobals57));
    for(int profile:{3,4,5,6}){
        g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_reload.shotgunFrame=g_frame;
        Mat4 initial={},grip={};Check(ControllerCurrentRoot124(initial,grip),"one-hand shotgun root available");
        const V3 forward=Unit({initial.m[1],initial.m[5],initial.m[9]});
        Mat4 turn=TurnAndMove(forward,{0,0,1},{0,0,0},{0,0,0}),controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];controller=Multiply(turn,controller);
        for(int i=0;i<16;++i)g_handBody[0][i]=g_handRight[i]=float(controller.m[i]);
        for(int step=0;step<12;++step){
            ++g_frame;g_weaponMechanicFrame=g_reload.shotgunFrame=g_frame;
            Mat4 animation=TurnAndMove({0,0,1},Unit({.1*step,.03*step,1}),{0,0,0},{.01*step,.02*step,0});float native[576]={};for(int b=0;b<48;++b)StoreBone(native,b,Multiply(animation,BoneMatrix(kM133GunBone57,0)));palette->SetPrivateData(kCbCopyGuid,sizeof(native),native);
            auto* uploaded=BuildWeaponPalette(context,16428,7311);Check(uploaded!=nullptr,"actual M133 receiver GPU override");float result[576];Read(device,context,uploaded,result);const Mat4 rendered=Multiply(view,BoneMatrix(result,0));
            const V3 barrel=Unit({rendered.m[1],rendered.m[5],rendered.m[9]});V3 aim={};Check(OneHandShotgunAim125(aim),"one-handed shotgun uses captured receiver barrel for aim");Check(Length(Sub(aim,barrel))<.001,"one-handed cursor/native aim equals actual uploaded M133 barrel under native animation");
            PublishReticleAim73(aim);g_aimActive=true;V3 target={};Check(ReticleTarget73(0,HandIdentity(),target),"forward shotgun cursor projects in front of eye");const V3 ray=Unit(Sub(target,ControllerWrist(g_handBody[0],0)));Check(Length(Sub(ray,barrel))<.001,"projected cursor follows displayed barrel instead of unrelated controller axis");g_aimActive=false;
            const Mat4 before=BoneMatrix(result,0);g_twoHand=true;V3 untouched={7,8,9};Check(!OneHandShotgunAim125(untouched)&&untouched.x==7&&untouched.y==8&&untouched.z==9,"two-hand accepted aim path is excluded and output untouched");g_twoHand=false;Read(device,context,uploaded,result);for(int i=0;i<12;++i)Check(fabs(before.m[i]-BoneMatrix(result,0).m[i])<1e-10,"aim selection never modifies the weapon GPU palette");
        }
    }
    g_weaponMechanicProfile=34;g_weaponMechanicFrame=g_frame;V3 aim={};Mat4 root={},grip={};Check(OneHandShotgunAim125(aim)&&ControllerCurrentRoot124(root,grip)&&Length(Sub(aim,Unit({root.m[1],root.m[5],root.m[9]})))<1e-8,"SPAS one-hand shares its own captured barrel axis");
    g_weaponMechanicProfile=12;Check(!OneHandShotgunAim125(aim),"D50 and other firearm aim ownership unchanged");g_weaponMechanicProfile=3;g_handRightValid=false;Check(!OneHandShotgunAim125(aim),"tracking loss rejects shotgun override");g_handRightValid=true;g_weaponMechanicFrame=g_frame-3;Check(!OneHandShotgunAim125(aim),"old shotgun classification rejects override");
    context->ClearState();g_weaponPaletteCb->Release();g_weaponPaletteCb=previous;vb->Release();globals->Release();camera->Release();palette->Release();
    puts("PASS: shotgun125 one-handed aim and projected cursor agree with actual M133 receiver GPU barrel across4 variants and12 native poses; SPAS uses own barrel; two-handed and other firearms unchanged; stale/tracking guards");
}
