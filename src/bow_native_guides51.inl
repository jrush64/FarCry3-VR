// Bow guides use the actual rigid handle's uploaded palette and eye camera.
// This preserves the existing nock/contact calibration and bow input path.
NativeGuideEye48 g_bowGuideEye51[2];
NativeGuidePacket50 g_bowGuidePackets51[2];UINT g_bowGuidePacketCount51=0;
bool BowGuideReady51(){
 if(!BowModel()&&!(g_pistolMenu.open&&g_weaponMechanicProfile==10))return false;
 for(const auto& eye:g_bowGuideEye51)if(!eye.valid||eye.frame>g_frame||(!g_pistolMenu.open&&eye.frame!=g_frame)||eye.anchor.epoch!=g_armPoseEpoch)return false;return true;
}
void CaptureBowGuides51(ID3D11DeviceContext* context,const DrawCall& call,int variant,int eye){
 if(g_weaponMechanicProfile!=10||variant!=1||call.kind!=0||call.b!=14982||call.a!=7140||eye<0||eye>1||!BowGeometry(NativeWeaponGeometry(context,call)))return;
 auto& out=g_bowGuideEye51[eye];if(out.valid&&out.frame==g_frame&&out.anchor.epoch==g_armPoseEpoch)return;
 ID3D11Buffer* cb[4]={};context->VSGetConstantBuffers(0,4,cb);float world[kSmallCbMax/4]={},camera[224]={},bones[576]={},stereo[64]={};
 bool valid=ReadCopy(cb[0],world,sizeof(world))>=60&&ReadCopy(cb[1],camera,sizeof(camera))==sizeof(camera)&&ReadCopy(cb[3],bones,sizeof(bones))==sizeof(bones);for(auto*& buffer:cb)SafeRelease(buffer);
 ID3D11Buffer* eyeBuffer=nullptr;context->VSGetConstantBuffers(dxbc::kEyeCbSlot,1,&eyeBuffer);valid=valid&&ReadCopy(eyeBuffer,stereo,sizeof(stereo))==sizeof(stereo);SafeRelease(eyeBuffer);
 CameraSummary summary={};Mat4 inverse={};valid=valid&&PistolGuideCamera(context,summary)&&Invert(ArmBodyToCamera(),inverse);if(!valid)return;
 const Mat4 frame=Multiply(HullWorld(world),BoneMatrix(bones,0));const Mat4 body=Multiply(inverse,Multiply(g_guideShaderPlacement[variant],Multiply(HullView(summary),frame)));Mat4 bodyInverse={};if(!Invert(body,bodyInverse))return;
 out=NativeGuideEye48{};out.anchor.world=frame;out.anchor.body=body;out.anchor.frame=g_frame;out.anchor.epoch=g_armPoseEpoch;out.anchor.context=context;out.anchor.valid=true;memcpy(out.camera,camera,sizeof(camera));for(int i=0;i<16;++i)out.eye[i]=stereo[i]+(i%5==0?1.f:0.f);out.frame=g_frame;out.valid=true;
}
void BowGuideShape51(const Mat4& shape,const V3* points,UINT count,int colour){
 if(!BowGuideReady51()||g_bowGuidePacketCount51>=2||!count||count>192)return;auto& packet=g_bowGuidePackets51[g_bowGuidePacketCount51];packet.count=count;packet.colour=colour;packet.holding=false;
 for(int eye=0;eye<2;++eye){const auto& draw=g_bowGuideEye51[eye];Mat4 inverse={};if(!Invert(draw.anchor.body,inverse))return;const Mat4 local=Multiply(inverse,shape);for(UINT i=0;i<count;++i)if(!Native1911Clip48(draw,Apply(local,points[i]),packet.clip[eye][i]))return;}++g_bowGuidePacketCount51;
}
void BowNativeGuideMarkers51(){
 g_bowGuidePacketCount51=0;fc3xr::HideReloadMarkers();if(!g_showWeaponReloadGuides||!PhysicalReloadGuidesEnabled()||!BowGuideReady51()||!BowHandsReady())return;
 // Weapon-local nock offsets use the same final handle pose as the bow pixels.
 const double scale=(std::max)(.1f,g_worldScale);const V3 half=BowStringHalfSize58();Mat4 box=g_bowGuideEye51[0].anchor.body;const V3 nock=BowNockPoint(box);box.m[3]=nock.x;box.m[7]=nock.y;box.m[11]=nock.z;
 const int edges[24]={0,1,2,3,4,5,6,7,0,2,1,3,4,6,5,7,0,4,1,5,2,6,3,7};V3 corners[24];for(int i=0;i<24;++i){const int c=edges[i];corners[i]={half.x*(c&1?1:-1),half.y*(c&2?1:-1),half.z*(c&4?1:-1)};}BowGuideShape51(box,corners,24,0);
 // Existing arrow-tail detector, rather than a newly invented palm offset.
 const V3 tail=BowArrowTail(Multiply(g_bow.right,BowHeldArrow()));Mat4 hand=HandIdentity();hand.m[3]=tail.x;hand.m[7]=tail.y;hand.m[11]=tail.z;V3 ring[192];UINT at=0;for(int axis=0;axis<3;++axis)for(int segment=0;segment<32;++segment)for(int end=0;end<2;++end){const double a=(segment+end)*(6.283185307179586/32);double v[3]={};v[(axis+1)%3]=cos(a)*.05*scale;v[(axis+2)%3]=sin(a)*.05*scale;ring[at++]={v[0],v[1],v[2]};}BowGuideShape51(hand,ring,192,1);
}
void RenderBowGuides51(ID3D11Texture2D* left,ID3D11Texture2D* right){
 if(g_weaponMechanicProfile!=10||!left||!right||fc3pad::NativeMenuOpen())return;fc3xr::HideReloadMarkers();if(!BowGuideReady51()||!g_showWeaponReloadGuides||!PhysicalReloadGuidesEnabled())return;
 for(UINT i=0;i<g_bowGuidePacketCount51;++i){const auto& packet=g_bowGuidePackets51[i];DrawNative1911Guide48(g_ctx,left,&packet.clip[0][0][0],packet.count,packet.colour);DrawNative1911Guide48(g_ctx,right,&packet.clip[1][0][0],packet.count,packet.colour);}
}
