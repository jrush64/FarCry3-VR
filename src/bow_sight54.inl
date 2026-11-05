// Optics on the bow follow its rigid handle, never a string/limb palette slot.
// Exact existing optic assets only. Each housing/glass/glow draw keeps its
// native local offset; drawing before the handle cannot reuse an old view move.
struct BowSightSlot54 {UINT vb=0,ib=0,stride=0,offset=0,indexOffset=0,start=0;Mat4 native={},relative={};LONG frame=-100000;UINT epoch=0;ID3D11DeviceContext* context=nullptr;bool valid=false;};
BowSightSlot54 g_bowSights54[32];
struct BowSightHandle54 {Mat4 native={};LONG frame=-100000;UINT epoch=0;ID3D11DeviceContext* context=nullptr;bool valid=false;}g_bowSightHandle54;
// PID72936, Oct13: 2,304 housing/handle same-frame pairs, maximum
// matrix spread2.87e-7. Standard-world glass/glow draws share the same asset
// frame; their local vertex positions provide each part's position.
const Mat4 kBowReflexAFromHandle58={{1.00000002,-.00004321,.00004313,-.03703417,.00004318,1.00000009,-.00004319,.00016503,-.00004310,.00004319,.99999999,.13337271,0,0,0,1}};
UINT g_bowSightEpoch54=~0u;
void ResetBowSights54(){if(g_bowSightEpoch54==g_armPoseEpoch)return;for(auto& slot:g_bowSights54)slot=BowSightSlot54{};g_bowSightHandle54=BowSightHandle54{};g_bowSightEpoch54=g_armPoseEpoch;}
bool BowSightAsset54(UINT vb,UINT ib){return (vb==54456&&ib==9066)||(vb==98144&&ib==21168)||(vb==120816&&ib==24948)||(vb==48520&&ib==7032)||(vb==67448&&ib==11280);}
struct BowSightUpload55 {ID3D11DeviceContext* context=nullptr;LONG frame=-1;UINT start=0,count=0;bool ready=false,rigid=false;Mat4 world={};}g_bowSightUpload55;
void BowSightFinalizeWorld55(ID3D11DeviceContext* context,UINT start,UINT count,float* worldRows){
 const auto& draw=g_bowSightUpload55;if(worldRows&&draw.ready&&draw.rigid&&draw.frame==g_frame&&draw.context==context&&draw.start==start&&draw.count==count)WritePistolSplitWorld(worldRows,draw.world);
}
void BowSightNote55(ID3D11DeviceContext* context,UINT start,UINT count,const char* status,UINT bytes=0){
 static unsigned long long keys[32]={};static UINT seen=0;ID3D11VertexShader* shader=nullptr;context->VSGetShader(&shader,nullptr,nullptr);const auto hash=ShaderHash(shader);SafeRelease(shader);unsigned long long key=hash^(static_cast<unsigned long long>(start)<<32)^count;for(const char* s=status;*s;++s)key=key*31+static_cast<unsigned char>(*s);for(UINT i=0;i<seen;++i)if(keys[i]==key)return;if(seen>=32)return;keys[seen++]=key;
 Log("[BOW SIGHT55] %s frame %ld draw %u/%u VS %016llX CB0 %u bytes; handle frame %ld epoch %u/%u; hand frames %ld/%ld tracked %d/%d",status,static_cast<long>(g_frame),start,count,hash,bytes,static_cast<long>(g_bowSightHandle54.frame),g_bowSightEpoch54,g_armPoseEpoch,static_cast<long>(g_bow.handFrame[0]),static_cast<long>(g_bow.handFrame[1]),g_handRightValid,g_handLeftValid);
}
// Called only after the production UpdateSubresource of the corrected world.
// The diagnostic distinguishes reaching the helper from actually uploading it.
void BowSightWorldUploaded55(ID3D11DeviceContext* context,UINT start,UINT count,UINT bytes){
 if(g_bowSightUpload55.ready&&g_bowSightUpload55.frame==g_frame&&g_bowSightUpload55.context==context&&g_bowSightUpload55.start==start&&g_bowSightUpload55.count==count){BowSightNote55(context,start,count,"corrected world uploaded",bytes);g_bowSightUpload55.ready=false;}
}
void BowSightLearn54(BowSightSlot54& slot){
 const auto& handle=g_bowSightHandle54;Mat4 inverse={};if(slot.valid||!handle.valid||handle.frame!=g_frame||slot.frame!=g_frame||slot.epoch!=g_armPoseEpoch||handle.epoch!=g_armPoseEpoch||handle.context!=slot.context||!Invert(handle.native,inverse))return;
 slot.relative=Multiply(inverse,slot.native);slot.valid=true;
}
void ObserveBowSightHandle54(ID3D11DeviceContext* context,UINT start,UINT count,const float* bones,const Mat4& toView){
 if(!BowActive()||start!=14982||count!=7140)return;DrawCall call={};call.a=count;call.b=start;if(!BowGeometry(NativeWeaponGeometry(context,call)))return;ResetBowSights54();CameraSummary camera={};Mat4 inverse={};if(!PistolGuideCamera(context,camera)||!camera.mainView||!Invert(HullView(camera),inverse))return;
 auto& handle=g_bowSightHandle54;handle.native=Multiply(inverse,Multiply(toView,BoneMatrix(bones,0)));handle.frame=g_frame;handle.epoch=g_armPoseEpoch;handle.context=context;handle.valid=true;
 for(auto& slot:g_bowSights54)if(slot.vb)BowSightLearn54(slot);
}
bool BowAttachmentMove54(ID3D11DeviceContext* context,UINT start,UINT count,const float* bones,const Mat4* toView,Mat4& move){
 g_bowSightUpload55.ready=false;if(!BowModel())return false;UINT vbBytes=0,ibBytes=0;if(!AttachmentBuffers(context,vbBytes,ibBytes)||!BowSightAsset54(vbBytes,ibBytes))return false;if(!BowActive()){BowSightNote55(context,start,count,"skipped inactive motion/tracking");return false;}ResetBowSights54();
 ID3D11Buffer* vb=nullptr,*ib=nullptr;UINT stride=0,offset=0,indexOffset=0;DXGI_FORMAT format={};context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);context->IAGetIndexBuffer(&ib,&format,&indexOffset);SafeRelease(vb);SafeRelease(ib);
 BowSightSlot54* slot=nullptr;for(auto& candidate:g_bowSights54)if(candidate.vb==vbBytes&&candidate.ib==ibBytes&&candidate.stride==stride&&candidate.offset==offset&&candidate.indexOffset==indexOffset&&candidate.start==start){slot=&candidate;break;}if(!slot)for(auto& candidate:g_bowSights54)if(!candidate.vb){slot=&candidate;candidate.vb=vbBytes;candidate.ib=ibBytes;candidate.stride=stride;candidate.offset=offset;candidate.indexOffset=indexOffset;candidate.start=start;break;}if(!slot)return false;
 CameraSummary camera={};Mat4 viewInverse={};if(!PistolGuideCamera(context,camera)||!camera.mainView||!Invert(HullView(camera),viewInverse)){BowSightNote55(context,start,count,"skipped camera");return false;}Mat4 nativeView={};UINT worldBytes=0;
 if(bones&&toView)nativeView=Multiply(*toView,BoneMatrix(bones,0));
 else {ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float rows[kSmallCbMax/4]={};worldBytes=ReadCopy(cb,rows,sizeof(rows));SafeRelease(cb);if(worldBytes<60){BowSightNote55(context,start,count,"skipped world shadow",worldBytes);return false;}nativeView=Multiply(HullView(camera),HullWorld(rows));}
 slot->native=Multiply(viewInverse,nativeView);slot->frame=g_frame;slot->epoch=g_armPoseEpoch;slot->context=context;if(vbBytes==54456&&ibBytes==9066&&!bones){slot->relative=kBowReflexAFromHandle58;slot->valid=true;}else BowSightLearn54(*slot);
 Mat4 root={},inverse={};if(!slot->valid||!BowRenderRoot58(root)||!Invert(nativeView,inverse)){BowSightNote55(context,start,count,slot->valid?"skipped current root":"skipped awaiting same-frame handle",worldBytes);return false;}
 const Mat4 desired=Multiply(ArmBodyToCamera(),Multiply(root,slot->relative));move=Multiply(desired,inverse);g_bowSightUpload55={context,g_frame,start,count,true,bones==nullptr,Multiply(viewInverse,desired)};BowSightNote55(context,start,count,"current-handle move selected",worldBytes);return true;
}
