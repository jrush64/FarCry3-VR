// SVD ocular centre/edge/parallax layers share the native receiver FRAME.
// PID121436: 98 lens/frame and203 illuminated/frame pairs, relative identity
// to <2e-12 m. These omitted draws must use the same fixed root as the housing.
ID3D11Buffer* g_svdLensBuffers236[8]={};UINT g_svdLensSizes236[8]={};ID3D11Device* g_svdLensDevice236=nullptr;
void ReleaseSvdLens236(){for(auto*& b:g_svdLensBuffers236)SafeRelease(b);for(auto& n:g_svdLensSizes236)n=0;g_svdLensDevice236=nullptr;}
bool SvdLensGeometry236(const fc3weapon::Geometry& d){
 if(d.vertexBytes!=268464||d.indexBytes!=59526||d.format!=57)return false;
 if(d.stride==20&&d.offset==0)return(d.start==0&&d.count==6)||(d.start==6&&d.count==432)||(d.start==438&&d.count==432);
 return d.stride==32&&d.offset==10160&&((d.start==1488&&d.count==288)||(d.start==1776&&d.count==192)||(d.start==2946&&d.count==87));
}
ID3D11Buffer* BuildSvdLens236(ID3D11DeviceContext* context,const DrawCall& call){
 if(context!=g_ctx||g_weaponMechanicProfile!=35||!AkModel()||!MotionInteractionEnabled()||!SvdLensGeometry236(NativeWeaponGeometry(context,call)))return nullptr;
 CameraSummary camera={};Mat4 cameraInverse={},root={};if(!PistolGuideCamera(context,camera)||!camera.mainView||!Invert(HullView(camera),cameraInverse)||!ControllerMagazineRoot124(root))return nullptr;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);D3D11_BUFFER_DESC desc={};if(cb)cb->GetDesc(&desc);float data[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,data,sizeof(data));SafeRelease(cb);
 const int offset=WeaponWorldOffset(context);if(bytes!=desc.ByteWidth||bytes<UINT((offset+15)*4))return nullptr;
 Mat4 lensWorld338=Multiply(cameraInverse,Multiply(ArmBodyToCamera(),root));const BOOL mirrored338=A2000LeftCharge336()?TRUE:FALSE;if(mirrored338){Mat4 mirror=HandIdentity();mirror.m[0]=-1;lensWorld338=Multiply(lensWorld338,mirror);}WritePistolWorld(data+offset,lensWorld338);
 if(g_svdLensDevice236!=g_dev){ReleaseSvdLens236();g_svdLensDevice236=g_dev;}
 int slot=-1;for(int i=0;i<8;++i)if(g_svdLensSizes236[i]==bytes){slot=i;break;}
 if(slot<0)for(int i=0;i<8;++i)if(!g_svdLensBuffers236[i]){D3D11_BUFFER_DESC out={};out.ByteWidth=bytes;out.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(SUCCEEDED(g_oCreateBuffer(g_dev,&out,nullptr,&g_svdLensBuffers236[i]))){g_svdLensSizes236[i]=bytes;slot=i;}break;}
 if(slot<0)return nullptr;const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_svdLensBuffers236[slot],0,nullptr,data,0,0);g_own=own;
 g_svdLensBuffers236[slot]->SetPrivateData(kA2000Mirror337,sizeof(mirrored338),&mirrored338);g_svdLensBuffers236[slot]->SetPrivateData(kCbCopyGuid,bytes,data);RecordAnimationSolid(context,data,bytes);return g_svdLensBuffers236[slot];
}
