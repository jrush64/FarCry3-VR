// Snapshot the completed solid FRAME upload and promote it only at its stereo draw.
// Predictors and attachment draws cannot replace this insertion parent.
struct PistolFrameAnchor45 {
 Mat4 world={},nativeView={},toBody={},body={};LONG frame=-100000;UINT epoch=0,start=0,count=0,weapon=0,wrapper=0;
 ID3D11DeviceContext* context=nullptr;ID3D11Buffer* buffer=nullptr;uintptr_t vertex=0;bool valid=false;
};
PistolFrameAnchor45 g_1911FramePending,g_1911FrameFinal;
void Stage1911Frame(ID3D11DeviceContext* context,UINT start,UINT count,const float* globals,ID3D11Buffer* buffer){
 if(g_weaponMechanicProfile!=20||!PistolModel()||!buffer||!(start==18&&count==276))return;
 DrawCall call={};call.a=count;call.b=start;if(!PistolMeshMain(NativeWeaponGeometry(context,call)))return;
 CameraSummary camera={};Mat4 toBody={};if(!PistolGuideCamera(context,camera)||!Invert(ArmBodyToCamera(),toBody))return;
 // The legitimate Kimber camera is 1.554/2.791 while the world camera is .368.
 // Exact geometry and the confirmed stereo variant own this draw, not world FOV.
 if(!std::isfinite(camera.xScale)||!std::isfinite(camera.yScale)||camera.xScale<=0||camera.yScale<=0)return;
 Mat4 world=HullWorld(globals);for(double v:world.m)if(!std::isfinite(v))return;
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);const uintptr_t vertex=reinterpret_cast<uintptr_t>(vb);SafeRelease(vb);
 auto& p=g_pistolReload;g_1911FramePending={};auto& a=g_1911FramePending;
 a.world=world;a.nativeView=HullView(camera);a.toBody=toBody;a.frame=g_frame;a.epoch=g_armPoseEpoch;a.context=context;a.buffer=buffer;a.vertex=vertex;a.start=start;a.count=count;a.weapon=p.weapon;a.wrapper=p.wrapper;a.valid=true;
}
void Confirm1911Frame(ID3D11DeviceContext* context,UINT start,UINT count,ID3D11Buffer* buffer,int variant){
 const auto& a=g_1911FramePending;
 if(g_weaponMechanicProfile!=20||variant!=1||!a.valid||a.frame!=g_frame||a.epoch!=g_armPoseEpoch||a.context!=context||a.buffer!=buffer||a.start!=start||a.count!=count)return;
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);const bool same=reinterpret_cast<uintptr_t>(vb)==a.vertex;SafeRelease(vb);if(!same)return;
 if(g_guideShaderPlacementFrame[1]>g_frame||g_frame-g_guideShaderPlacementFrame[1]>1)return;
 const Mat4 placement=g_guideShaderPlacement[1];
 const Mat4 body=Multiply(a.toBody,Multiply(placement,Multiply(a.nativeView,a.world)));Mat4 inverse={};if(!Invert(body,inverse))return;
 g_1911FrameFinal=a;g_1911FrameFinal.body=body;
 static LONG logs=0;if(logs++<16)Log("[FRAME47] 1911 insertion parent from final stereo FRAME draw %u/%u frame %ld",start,count,g_frame);
}
bool Resolve1911Insertion(const Mat4& source,Mat4& root,bool visual){
 root=PistolInteractionRoot(source);const auto& a=g_1911FrameFinal;
 if(g_weaponMechanicProfile!=20||!a.valid)return true; // existing startup path until first confirmed FRAME
 if((!visual&&g_pistolMenu.open)||a.epoch!=g_armPoseEpoch||a.frame>g_frame||(!visual&&a.frame!=g_frame))return false;
 const auto& p=g_pistolReload;
 if((a.weapon&&p.weapon&&a.weapon!=p.weapon)||(a.wrapper&&p.wrapper&&a.wrapper!=p.wrapper))return false;
 root=a.body;return true;
}
