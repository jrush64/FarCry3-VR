//Reflect actual emitted vertices once; leave all native/cached roots canonical.
bool A2000MirrorDrawModel337(ID3D11DeviceContext* context,UINT start,UINT count){
 if(!A2000LeftCharge336())return false;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 return (RifleProfile(g_weaponMechanicProfile)&&RifleMain(CurrentRifle(),d))||(g_weaponMechanicProfile==39&&d.vertexBytes==157984&&d.indexBytes==35322&&d.stride==32&&d.offset==0&&d.format==57)||RifleAttachmentModel(d.vertexBytes,d.indexBytes);
}
Mat4 A2000MirrorOutput337(const Mat4& world,const Mat4& rootWorld,bool detached){
 Mat4 mirror=HandIdentity();mirror.m[0]=-1;
 if(detached)return Multiply(world,mirror);
 Mat4 inverse={};if(!Invert(rootWorld,inverse))return world;
 return Multiply(rootWorld,Multiply(mirror,Multiply(inverse,world)));
}
bool A2000MirrorWorld337(ID3D11DeviceContext* context,UINT start,UINT count,float* rows){
 if(!A2000MirrorDrawModel337(context,start,count))return false;
 Mat4 rootWorld={};if(RefreshWeaponDrawRoot51(context))rootWorld=g_pistolReload.rootWorld;else{CameraSummary camera={};Mat4 vi={};if(!g_pistolReload.parentValid||g_pistolReload.parentEpoch!=g_armPoseEpoch||!PistolGuideCamera(context,camera)||!Invert(HullView(camera),vi))return false;rootWorld=Multiply(vi,Multiply(ArmBodyToCamera(),AkParentBody()));}
 const auto& p=g_pistolReload;DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 const bool clip=g_weaponMechanicProfile==39?(d.vertexBytes==157984&&d.indexBytes==35322&&start==0&&count==1548):(RifleMain(CurrentRifle(),d)&&RifleClip(CurrentRifle(),start,count));
 const bool inserting=p.insertActive&&(p.pending||p.cycle||p.magazinePresent);
 const bool detached=clip&&(p.held||p.removalHeld||PistolAmmoPreviewActive()||p.looseFlying||(!inserting&&p.dropActive));
 WritePistolWorld(rows,A2000MirrorOutput337(HullWorld(rows),rootWorld,detached));
 static LONG logs=0;if(InterlockedIncrement(&logs)<=16)Log("[SIDEMIRROR338] full rigid mesh profile=%d frame=%ld draw=%u/%u detached=%d",g_weaponMechanicProfile,g_frame,start,count,detached);
 return true;
}
// A skinned lever can also have an ordinary rigid world override. Its mixed
// palette winding must not be hidden by that unrelated world buffer's tag.
ID3D11Buffer* MirrorRasterSource349(ID3D11Buffer* world,ID3D11Buffer* palette){
 BOOL kind=FALSE;UINT bytes=sizeof(kind);
 if(palette&&SUCCEEDED(palette->GetPrivateData(kA2000Mirror337,&bytes,&kind))&&kind==2)return palette;
 return world?world:palette;
}
bool BeginA2000Raster337(ID3D11DeviceContext* context,ID3D11Buffer* world,ID3D11RasterizerState*& original,bool& changed){
 changed=false;original=nullptr;BOOL mirrored=FALSE;UINT bytes=sizeof(mirrored);
 if(!world||FAILED(world->GetPrivateData(kA2000Mirror337,&bytes,&mirrored))||!mirrored)return true;
 context->RSGetState(&original);D3D11_RASTERIZER_DESC desc={};
 if(original)original->GetDesc(&desc);else{desc.FillMode=D3D11_FILL_SOLID;desc.CullMode=D3D11_CULL_BACK;desc.DepthClipEnable=TRUE;}
 // Mixed flamer lever draws contain native receiver bones as well as reflected
 // articulated bones. Both windings must survive this draw; restore immediately.
 if(mirrored==2)desc.CullMode=D3D11_CULL_NONE;else desc.FrontCounterClockwise=!desc.FrontCounterClockwise;
 struct Entry{D3D11_RASTERIZER_DESC desc={};ID3D11RasterizerState* state=nullptr;};static Entry cache[8];static unsigned next=0;static ID3D11Device* device=nullptr;
 if(device!=g_dev){for(auto& c:cache)SafeRelease(c.state);device=g_dev;}
 ID3D11RasterizerState* replacement=nullptr;for(const auto& c:cache)if(c.state&&!memcmp(&c.desc,&desc,sizeof(desc))){replacement=c.state;break;}
 if(!replacement){auto& c=cache[next++%8];SafeRelease(c.state);c.desc=desc;if(FAILED(g_dev->CreateRasterizerState(&desc,&c.state))){SafeRelease(original);return false;}replacement=c.state;}
 context->RSSetState(replacement);changed=true;return true;
}
void EndA2000Raster337(ID3D11DeviceContext* context,ID3D11RasterizerState*& original,bool changed){if(changed)context->RSSetState(original);SafeRelease(original);}

bool SideAttachment338(ID3D11DeviceContext* context,UINT start,UINT count){if(!A2000LeftCharge336())return false;DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);return RifleAttachmentModel(d.vertexBytes,d.indexBytes);}
Mat4 CanonicalPhysical338(const Mat4& physical,const Mat4& frame){if(!A2000LeftCharge336())return physical;Mat4 m=HandIdentity();m.m[0]=-1;return Multiply(A2000MirrorOutput337(physical,frame,false),m);}
bool MirrorSkinned338(float* bones,const Mat4& frame){
 if(!A2000LeftCharge336())return false;Mat4 inverse={},m=HandIdentity();m.m[0]=-1;if(!Invert(frame,inverse))return false;
 const auto reflection=Multiply(frame,Multiply(m,inverse));for(int b=0;b<48;++b)if(bones[b*12+7]!=-100.f)StoreBone(bones,b,Multiply(reflection,BoneMatrix(bones,b)));return true;
}

//A shared palette buffer must not carry a previous rifle's winding into .44/tools.
void ClearMirrorPalette338(){if(g_weaponPaletteCb){const BOOL normal=FALSE;g_weaponPaletteCb->SetPrivateData(kA2000Mirror337,sizeof(normal),&normal);}}

//Retain native skinned-sight anchor correction while reflecting the whole output.
//337's accepted A2000 path remains unchanged; other rifles keep their prior reload alignment.
bool SideSkinnedAttachmentMove338(ID3D11DeviceContext* context,UINT start,UINT count,Mat4& move){
 if(g_weaponMechanicProfile==7||!SideAttachment338(context,start,count))return false;
 const int slot=MuzzlePaletteSlot58(context);if(slot<0)return false;
 float bones[576]={};Mat4 toView={},fromView={};if(!DrawSpace(context,bones,toView,fromView,UINT(slot),true))return false;
 return RifleSkinnedAttachmentMove(context,start,count,bones,toView,move);
}
