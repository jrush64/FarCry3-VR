// .44 color/depth/lens passes use either b3 or b4 for BlendMatrices. Pin the
// actual palette consumed by each shader and retain each attachment's authored
// offset. Native same-frame captures supply these rigid relations, not UI aim.
void ObserveRevolverAssembly247(ID3D11DeviceContext* context,const DrawCall& call){
    if(!MotionInteractionEnabled()||MacheteHeld()||call.kind||call.d||RevolverModel())return;
    // Unique .44 parts may precede its main receiver on equip. Shared optics
    // cannot acquire ownership, but its own short/long barrel can, immediately.
    if(call.a!=9432&&call.a!=1338&&call.a!=1080)return;
    const auto d=NativeWeaponGeometry(context,call);
    const bool main=d.vertexBytes==186104&&d.indexBytes==31002&&d.stride==40&&d.offset==224&&call.b==12&&call.a==9432;
    if(d.format!=57||(!main&&!RevolverBarrelMaterial217(d,call.b,call.a)))return;
    CameraSummary camera={};if(!PistolGuideCamera(context,camera)||!camera.mainView)return;
    g_weaponMechanicProfile=42;g_weaponMechanicFrame=g_frame;
}
int RevolverAssemblyPart217(ID3D11DeviceContext* context,UINT start,UINT count,Mat4& relative)
{
    if(!RevolverModel()||!MotionInteractionEnabled()||MacheteHeld())return 0;
    DrawCall call={};call.a=count;call.b=start;
    const auto d=NativeWeaponGeometry(context,call);
    if(d.format!=57)return 0;
    relative=HandIdentity();
    if(d.vertexBytes==186104&&d.indexBytes==31002)return 1;
    if((d.vertexBytes==14768&&d.indexBytes==2172)||(d.vertexBytes==18032&&d.indexBytes==2688)){
        relative.m[3]=-.00000160;relative.m[7]=.12262147;relative.m[11]=.08685834;return 2;
    }
    if(d.vertexBytes==120816&&d.indexBytes==24948){
        // Cannon's optical attachment uses the authored ATTACH_SCOPE node.
        // It must share the receiver root just like the ordinary reflex sight.
        relative.m[7]=.1456338912;relative.m[11]=.0946501866;return 4;
    }
    if(d.vertexBytes==48520&&d.indexBytes==7032){
        relative.m[3]=-.00000227;relative.m[7]=.14557447;relative.m[11]=.09465650;return 3;
    }
    return 0;
}
bool RevolverAssemblyDraw217(ID3D11DeviceContext* context,UINT start,UINT count)
{
    Mat4 relative={};return RevolverAssemblyPart217(context,start,count,relative)!=0;
}
ID3D11Buffer* BuildRevolverPalette217(ID3D11DeviceContext* context,UINT start,UINT count,UINT& bindingSlot)
{
 ClearMirrorPalette338();
    MuzzleDiscardStage58();
    Mat4 relative={},toView={},fromView={},root={},grip={},anchorInverse={};
    if(!RevolverAssemblyPart217(context,start,count,relative)||!g_weaponPaletteCb)return nullptr;
    const int slot=MuzzlePaletteSlot58(context);
    if(slot!=3&&slot!=4)return nullptr; // rigid draws belong to the world path
    ID3D11Buffer* source=nullptr;context->VSGetConstantBuffers(UINT(slot),1,&source);
    float bones[576]={};const UINT bytes=ReadCopy(source,bones,sizeof(bones));SafeRelease(source);
    if(bytes!=sizeof(bones)||!DrawSpace(context,nullptr,toView,fromView)||
       !ControllerCurrentRoot124(root,grip)||!Invert(BoneMatrix(bones,0),anchorInverse))return nullptr;
    // The passive timeline must pair this shader's source palette with its
    // upload, including material passes whose palette is b4 rather than b3.
    RecordAnimationPaletteSource217(context,bones,UINT(slot));
    Mat4 native={};int profile=-1,anchor=0;
    const bool muzzle=MuzzleReadNative58(context,start,count,nullptr,native,profile,anchor);
    const Mat4 target=Multiply(ArmBodyToCamera(),Multiply(root,relative));
    MoveBones(bones,0,47,Multiply(fromView,Multiply(target,anchorInverse)));
    RevolverPalette(context,bones,toView,start,count);
    RecordReloadPort(context,bones,start,count);
    if(muzzle){ID3D11Buffer* globals=nullptr;context->VSGetConstantBuffers(0,1,&globals);
        float rows[kSmallCbMax/4]={};const UINT got=ReadCopy(globals,rows,sizeof(rows));SafeRelease(globals);
        if(got>=60)MuzzleStage58(context,start,count,g_weaponPaletteCb,native,
            Multiply(HullWorld(rows),BoneMatrix(bones,anchor)),profile);
    }
    RecordAnimationFinal(bones,2);
    const bool own=g_own;g_own=true;
    g_oUpdateSubresource(context,g_weaponPaletteCb,0,nullptr,bones,0,0);
    g_weaponPaletteCb->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
    g_own=own;bindingSlot=UINT(slot);
    static UINT seen247[5][2]={};const int part247=RevolverAssemblyPart217(context,start,count,relative);
    if(part247>0&&part247<5&&seen247[part247-1][slot-3]++<2)Log("[CANNON247] part%d draw%u/%u b%d pinned to receiver/controller assembly; attachment %.5f/%.5f/%.5f",part247,start,count,slot,relative.m[3],relative.m[7],relative.m[11]);
    return g_weaponPaletteCb;
}
bool RevolverAssemblyWorld217(ID3D11DeviceContext* context,UINT start,UINT count,float* rows)
{
    Mat4 relative={},root={};
    if(!RevolverAssemblyPart217(context,start,count,relative))return false;
    // A skinned material must go through BuildRevolverPalette217. Do not move
    // its world as well or overwrite a failed/untrusted palette with a guess.
    if(MuzzlePaletteSlot58(context)!=-1||
       !ControllerGripWorld124(context,kRevolverRightHandFromFrame,nullptr,root))return false;
    WritePistolWorld(rows,Multiply(root,relative));return true;
}
