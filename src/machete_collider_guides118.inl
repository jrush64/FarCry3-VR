struct MacheteColliderGuide118 {float clip[2][192][4]={};UINT count=0;LONG frame=-1;UINT epoch=0;}g_macheteColliderGuide118;
void CaptureMacheteColliderGuide118(ID3D11DeviceContext* context)
{
    if(!MacheteHeld()||!g_macheteCollider118.show||!g_machete.handAttachment116)return;
    ID3D11Buffer* cb[2]={};context->VSGetConstantBuffers(0,2,cb);float globals[kSmallCbMax/4]={},camera[224]={};
    const bool read=ReadCopy(cb[0],globals,sizeof(globals))>=80&&ReadCopy(cb[1],camera,sizeof(camera))==sizeof(camera);for(auto*& b:cb)SafeRelease(b);
    CameraSummary summary={};if(!read||!PistolGuideCamera(context,summary))return;
    Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
    const Mat4 shape=Multiply(controller,Multiply(g_machete.controllerHand116,Multiply(kMacheteFromHand[0],MacheteColliderLocal118(g_macheteCollider118))));
    const double radius=g_macheteCollider118.radius,half=MacheteColliderHalf118(g_macheteCollider118);
    V3 points[192];UINT n=0;
    for(int side:{-1,1})for(int segment=0;segment<16;++segment)for(int end=0;end<2;++end){double t=(segment+end)*6.283185307179586/16;points[n++]={radius*cos(t),radius*sin(t),side*half};}
    for(int segment=0;segment<4;++segment)for(int side:{-1,1}){double t=segment*6.283185307179586/4;points[n++]={radius*cos(t),radius*sin(t),side*half};}
    for(int axis=0;axis<2;++axis)for(int side:{-1,1})for(int segment=0;segment<8;++segment)for(int end=0;end<2;++end){double t=(segment+end)*3.141592653589793/8;V3 v={0,0,side*(half+radius*sin(t))};if(axis)v.y=radius*cos(t);else v.x=radius*cos(t);points[n++]=v;}
    for(int eye=0;eye<2;++eye){NativeGuideEye48 draw;draw.anchor.world=HullWorld(globals);draw.anchor.body=Multiply(PistolGuideDrawToBody(summary,1),draw.anchor.world);Mat4 inverse={};if(!Invert(draw.anchor.body,inverse))return;memcpy(draw.camera,camera,sizeof(camera));float stereo[64]={};if(ReadCopy(g_eyeCb[1][eye],stereo,sizeof(stereo))!=sizeof(stereo))return;for(int i=0;i<16;++i)draw.eye[i]=stereo[i]+(i%5==0?1.f:0.f);const Mat4 local=Multiply(inverse,shape);for(UINT i=0;i<n;++i)if(!Native1911Clip48(draw,Apply(local,points[i]),g_macheteColliderGuide118.clip[eye][i]))return;}
    g_macheteColliderGuide118.count=n;g_macheteColliderGuide118.frame=g_frame;g_macheteColliderGuide118.epoch=g_armPoseEpoch;
}
void RenderMacheteColliderGuide118(ID3D11Texture2D* left,ID3D11Texture2D* right)
{
    const auto& g=g_macheteColliderGuide118;if(!MacheteHeld()||!g_macheteCollider118.show||g.frame!=g_frame||g.epoch!=g_armPoseEpoch||fc3pad::NativeMenuOpen()||!left||!right)return;
    const auto now=GetTickCount64();const int colour=now-g_macheteActor119.dispatchMs<200?1:now-g_macheteActor119.contactMs<150?2:0;
    DrawNative1911Guide48(g_ctx,left,&g.clip[0][0][0],g.count,colour);DrawNative1911Guide48(g_ctx,right,&g.clip[1][0][0],g.count,colour);
}
