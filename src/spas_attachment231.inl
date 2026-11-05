struct SpasAttachment231 {UINT vb=0,start=0,count=0,epoch=0;Mat4 native={},relative={};LONG frame=-100000;bool valid=false;};
SpasAttachment231 g_spasAttachments231[24];
Mat4 g_spasNative231={};LONG g_spasNativeFrame231=-100000;UINT g_spasNativeEpoch231=0;ID3D11DeviceContext* g_spasNativeContext231=nullptr;
void SpasNativeFrame231(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& world){
 if(!SpasModel()||!((start==4908&&count==7401)||(start==18&&count==3090)||(start==3108&&count==1800)||(start==0&&count==18)))return;
 DrawCall call={};call.a=count;call.b=start;if(NativeWeaponGeometry(context,call).vertexBytes!=127600)return;
 g_spasNative231=world;g_spasNativeFrame231=g_frame;g_spasNativeEpoch231=g_armPoseEpoch;g_spasNativeContext231=context;
 Mat4 inverse={};if(!Invert(world,inverse))return;
 for(auto& a:g_spasAttachments231)if(a.vb&&a.frame==g_frame&&a.epoch==g_armPoseEpoch){a.relative=Multiply(inverse,a.native);a.valid=true;}
}
bool SpasAttachmentMove231(ID3D11DeviceContext* context,UINT start,UINT count,const float* bones,const Mat4* toView,Mat4& move){
 if(!SpasActive())return false;
 UINT vb=0,ib=0;if(!AttachmentBuffers(context,vb,ib)||!RifleAttachmentModel(vb,ib))return false;
 CameraSummary camera={};Mat4 viewInverse={},target={},support=SpasKeyMat(kSpasLeftSupportFromFrame);
 if(!PistolGuideCamera(context,camera)||!Invert(HullView(camera),viewInverse)||!ControllerGripWorld124(context,SpasKeyMat(kSpasRightHandFromFrame),&support,target))return false;
 Mat4 native={};
 if(bones&&toView)native=Multiply(viewInverse,Multiply(*toView,BoneMatrix(bones,0)));
 else{ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);float data[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,data,sizeof(data));SafeRelease(cb);const int off=WeaponWorldOffset(context);if(bytes<UINT((off+15)*4))return false;native=HullWorld(data+off);}
 SpasAttachment231* slot=nullptr;for(auto& a:g_spasAttachments231)if(a.vb==vb&&a.start==start&&a.count==count){slot=&a;break;}
 if(!slot)for(auto& a:g_spasAttachments231)if(!a.vb){slot=&a;a.vb=vb;a.start=start;a.count=count;break;}if(!slot)return false;
 if(slot->epoch!=g_armPoseEpoch){slot->valid=false;slot->epoch=g_armPoseEpoch;}slot->native=native;slot->frame=g_frame;
 if(g_spasNativeFrame231==g_frame&&g_spasNativeEpoch231==g_armPoseEpoch&&g_spasNativeContext231==context){Mat4 inverse={};if(Invert(g_spasNative231,inverse)){slot->relative=Multiply(inverse,native);slot->valid=true;}}
 if(!slot->valid)return false;
 Mat4 inverseRel={},inverseNative={};if(!Invert(slot->relative,inverseRel)||!Invert(Multiply(native,inverseRel),inverseNative))return false;
 move=Multiply(HullView(camera),Multiply(Multiply(target,inverseNative),viewInverse));return true;
}
